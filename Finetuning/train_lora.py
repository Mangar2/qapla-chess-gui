"""LoRA fine-tune of LFM2.5 on the dataset, with the loss restricted to assistant tokens.

Why this exists instead of a plain `mlx_lm.lora` call:

Every record is ~7.4k tokens, of which ~7.1k are the system prompt plus the 17 tool schemas
-- byte-identical across all 453 records. Roughly 1% of the tokens are the assistant output
that the dataset was written to teach. Trained unmasked, ~99% of the gradient signal goes
into reproducing the tool schemas, which the model never needs to emit.

mlx-lm can mask, but only a single prefix per sample (see default_loss in tuner/trainer.py:
`steps >= lengths[:, 0]`). That cannot express "the four separate assistant spans in this
record" -- a multi-step tool chain has the assistant speaking, a tool answering, the
assistant speaking again. So loss and batching are supplied here; `--mask` picks between:

    assistant   loss only on assistant spans, via the template's {% generation %} markers
    prefix      loss on everything after the system turn (tool results included)
    none        mlx-lm's default, everything -- for comparison, not for real use

The chat template comes from chat_template.jinja rather than the model repo: the MLX repo's
own template silently drops tool_calls entirely, rendering every assistant turn as empty.

Usage:
    python3 train_lora.py --iters 10          # smoke test, prints timing
    python3 train_lora.py                     # full run
    python3 train_lora.py --mask prefix       # ablation
"""
import argparse
import json
import pathlib
import time

import mlx.core as mx
import mlx.nn as nn
import mlx.optimizers as optim
import numpy as np
from mlx.utils import tree_flatten
from mlx_lm.tuner.trainer import TrainingArgs, train
from mlx_lm.tuner.utils import linear_to_lora_layers
from mlx_lm.utils import load
from transformers import AutoTokenizer

HERE = pathlib.Path(__file__).parent
# bf16 peaks at 16.9GB on this 16GB machine -- it trains, but through swap, at 2.2 tok/s.
# The 4bit weights are 0.62GB against 2.4GB, which is what keeps a run in RAM.
MODEL = "LiquidAI/LFM2.5-1.2B-Instruct-MLX-4bit"
TEMPLATE = (HERE / "chat_template.jinja").read_text(encoding="utf-8")


class MaskedChatDataset:
    """Renders each record once and keeps (tokens, per-token loss mask)."""

    def __init__(self, path, tokenizer, mask_mode):
        self.items = []
        for line in path.open(encoding="utf-8"):
            if not line.strip():
                continue
            record = json.loads(line)
            messages, tools = record["messages"], record.get("tools")
            encoded = tokenizer.apply_chat_template(
                messages, tools=tools, chat_template=TEMPLATE, tokenize=True,
                return_dict=True, return_assistant_tokens_mask=True)
            tokens = encoded["input_ids"]

            if mask_mode == "assistant":
                mask = list(encoded["assistant_masks"])
                if not any(mask):
                    raise SystemExit(
                        "assistant mask is empty -- the template lost its {% generation %} "
                        "markers, every record would contribute zero loss")
            elif mask_mode == "prefix":
                # everything after the system turn: user text, tool calls, tool results, reply
                prefix = len(tokenizer.apply_chat_template(
                    messages[:1], tools=tools, chat_template=TEMPLATE, tokenize=True))
                mask = [0] * prefix + [1] * (len(tokens) - prefix)
            else:
                mask = [1] * len(tokens)
            self.items.append((tokens, mask))

    def __len__(self):
        return len(self.items)

    def __getitem__(self, i):
        return self.items[i]


def iterate_batches(dataset, batch_size, max_seq_length, loop=False, seed=None,
                    comm_group=None):
    """Same shape as mlx-lm's, but yields the indices of the loss-carrying positions.

    Indices rather than a mask, because masked_loss runs under value_and_grad and MLX forbids
    evaluating an array inside a function transformation -- so `np.flatnonzero(mask)` has to
    happen out here, where the mask is still plain batch data.

    Indices point into the flattened (batch, width-1) target grid, i.e. they are already
    shifted: target position i is predicted from input i.
    """
    order = sorted(range(len(dataset)), key=lambda i: len(dataset[i][0]))
    batches = [order[i:i + batch_size]
               for i in range(0, len(order) - batch_size + 1, batch_size)]
    if seed:
        np.random.seed(seed)
    while True:
        # Shuffled while training, in order while validating: an evaluation over a subset of
        # batches is only comparable across runs if it keeps drawing the same subset.
        sequence = np.random.permutation(len(batches)) if loop else range(len(batches))
        for b in sequence:
            items = [dataset[j] for j in batches[b]]
            lengths = [min(len(t), max_seq_length) for t, _ in items]
            pad_to = 32
            width = min(1 + pad_to * ((max(lengths) + pad_to - 1) // pad_to), max_seq_length)
            tokens = np.zeros((len(items), width), np.int32)
            masks = np.zeros((len(items), width), np.int32)
            for j, (tok, msk) in enumerate(items):
                n = min(len(tok), width)
                tokens[j, :n] = tok[:n]
                masks[j, :n] = msk[:n]
            indices = np.flatnonzero(masks[:, 1:].reshape(-1))
            if indices.size == 0:
                continue  # nothing to learn from this batch; skip rather than divide by zero
            yield mx.array(tokens), mx.array(indices)
        if not loop:
            break


def masked_loss(model, batch, indices):
    """Cross entropy over the masked positions only.

    An earlier version gathered the hidden states before the vocabulary projection, on the
    theory that materialising 7800 x 65536 logits (1.9GB, plus its gradient) to use ~85 rows
    was the memory problem. Measured, it was not: peak went from 14.65GB to 14.75GB, i.e.
    marginally worse. MLX evaluates lazily and fuses, so those logits were apparently never
    held whole. The gather is gone again -- it changed the hot path for no benefit.

    The peak is dominated by forward/backward activations over ~7800 positions, which is not
    reachable without shortening the context, and the context is the tool schemas.
    """
    inputs, targets = batch[:, :-1], batch[:, 1:]
    logits = model(inputs)
    ce = nn.losses.cross_entropy(logits.reshape(-1, logits.shape[-1])[indices],
                                 targets.reshape(-1)[indices])
    ntoks = indices.shape[0]
    return ce.astype(mx.float32).sum() / ntoks, mx.array(ntoks)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--mask", choices=["assistant", "prefix", "none"], default="assistant")
    p.add_argument("--iters", type=int, default=None, help="default: 3 epochs")
    p.add_argument("--batch-size", type=int, default=1)
    # 16 layers peaked at 15.9GB, well past the GPU's 11.8GB recommended working set, and took
    # the machine down with a driver panic ("completeMemory() prepare count underflow" in
    # IOGPUMemory.cpp). 8 layers halves the adapter's backward-pass footprint; with 151 base
    # records there is no shortage of capacity at this size.
    p.add_argument("--lora-layers", type=int, default=8)
    p.add_argument("--rank", type=int, default=16)
    # mlx-lm defaults to scale 20.0, which is meant for rank 8. Keeping it at rank 16 made the
    # validation loss climb 1.23 -> 2.85 within 20 iterations. scale/rank is the effective step
    # size on the adapter, so it is scaled down here to match.
    p.add_argument("--scale", type=float, default=2.0)
    p.add_argument("--learning-rate", type=float, default=1e-4)
    p.add_argument("--model", default=MODEL)
    p.add_argument("--max-seq-length", type=int, default=8192)
    p.add_argument("--adapter-path", default="adapters")
    # The GPU driver panics on this workload roughly every 40-50 iterations and takes the
    # machine with it (see TRAINING.md). Three attempts at avoiding it failed, so the run is
    # built to survive instead: save often, and pick up where it stopped.
    p.add_argument("--resume", action="store_true",
                   help="continue from adapters.safetensors in --adapter-path")
    p.add_argument("--steps-per-save", type=int, default=10)
    # A validation pass costs ~11s per record because every record carries the full ~7.4k-token
    # prefix. Over all 36 valid records that is ~7 minutes, so validating often costs far more
    # wall-clock than the training it is meant to supervise. 8 batches is ~90s and still tracks
    # the curve; the batches are drawn in the same order every time, so the number is comparable
    # across evals even though it is a subset.
    p.add_argument("--val-batches", type=int, default=8)
    # Hard ceiling below the GPU's max_recommended_working_set_size. Past that point the driver
    # is outside what it is tested for, and on this machine it panicked rather than refusing the
    # allocation. MLX blocks on scheduled work instead of allocating past the limit, so hitting
    # it costs speed, not correctness.
    p.add_argument("--memory-limit-gb", type=float, default=0.0,
                   help="0 = no limit; a value below actual demand starves the run")
    p.add_argument("--cache-limit-mb", type=int, default=0,
                   help="0 = MLX default. Shrinking the cache forces constant re-allocation")
    p.add_argument("--steps-per-eval", type=int, default=None,
                   help="default: 8 evals over the run, but never more often than every 25 iters")
    args = p.parse_args()

    recommended = mx.device_info()["max_recommended_working_set_size"]
    # Both limits default to off, and that is a correction, not an oversight. Capping memory at
    # 90% of the recommended working set (10.66GB) while the run actually needs ~14.2GB made
    # MLX wait on scheduled work at every allocation: the GPU sat at 100% utilisation and the
    # run stopped advancing entirely -- 68 minutes without completing a single iteration. The
    # cap never prevented the driver panic either, so it cost throughput and bought nothing.
    if args.memory_limit_gb:
        mx.set_memory_limit(int(args.memory_limit_gb * 2**30))
    if args.cache_limit_mb:
        mx.set_cache_limit(args.cache_limit_mb * 2**20)
    print(f"GPU recommends at most {recommended/2**30:.2f} GB; "
          f"limit {'%.2f GB' % args.memory_limit_gb if args.memory_limit_gb else 'off'}, "
          f"cache {'%d MB' % args.cache_limit_mb if args.cache_limit_mb else 'default'}")

    tokenizer = AutoTokenizer.from_pretrained(args.model)
    data = HERE / "data"
    train_set = MaskedChatDataset(data / "train.jsonl", tokenizer, args.mask)
    valid_set = MaskedChatDataset(data / "valid.jsonl", tokenizer, args.mask)

    kept = sum(sum(m) for _, m in train_set.items)
    total = sum(len(t) for t, _ in train_set.items)
    print(f"mask={args.mask}: training on {kept:,} of {total:,} tokens ({kept/total:.1%})")

    iters = args.iters or (len(train_set) * 3 // args.batch_size)
    print(f"{len(train_set)} train / {len(valid_set)} valid records, {iters} iterations")

    model, _ = load(args.model)
    model.freeze()
    linear_to_lora_layers(model, args.lora_layers,
                          {"rank": args.rank, "scale": args.scale, "dropout": 0.0})
    trainable = sum(v.size for _, v in tree_flatten(model.trainable_parameters()))
    print(f"trainable parameters: {trainable/1e6:.2f}M")

    adapter_path = HERE / args.adapter_path
    adapter_path.mkdir(parents=True, exist_ok=True)
    if args.resume:
        weights = adapter_path / "adapters.safetensors"
        if not weights.exists():
            raise SystemExit(f"--resume, but {weights} does not exist")
        model.load_weights(str(weights), strict=False)
        # Adam's moment estimates are not restored -- they rebuild within a few steps and are
        # not worth persisting for a crash-recovery path.
        print(f"resumed from {weights.name}")
    (adapter_path / "adapter_config.json").write_text(json.dumps({
        "num_layers": args.lora_layers, "fine_tune_type": "lora",
        "lora_parameters": {"rank": args.rank, "scale": args.scale, "dropout": 0.0},
    }, indent=2), encoding="utf-8")

    started = time.time()
    model.train()
    train(
        model=model,
        optimizer=optim.Adam(learning_rate=args.learning_rate),
        train_dataset=train_set,
        val_dataset=valid_set,
        args=TrainingArgs(
            batch_size=args.batch_size,
            iters=iters,
            val_batches=min(args.val_batches, len(valid_set) // args.batch_size),
            steps_per_report=10,
            steps_per_eval=args.steps_per_eval or max(iters // 8, 25),
            steps_per_save=args.steps_per_save,
            max_seq_length=args.max_seq_length,
            adapter_file=str(adapter_path / "adapters.safetensors"),
            grad_checkpoint=True,
        ),
        loss=masked_loss,
        iterate_batches=iterate_batches,
    )
    print(f"\ndone in {(time.time()-started)/60:.1f} min -> {adapter_path}")


if __name__ == "__main__":
    main()
