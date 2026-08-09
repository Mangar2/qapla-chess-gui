"""LoRA fine-tune on CPU with PyTorch -- the portable counterpart to train_lora.py.

Why a second trainer: MLX runs the GPU, and on a 16GB machine this workload (7.8k tokens per
record) overruns the driver's recommended working set. Two kernel panics and two swap
stalls, none of which a shorter context could avoid, because the context IS the tool schemas.
MLX's CPU backend is not an escape -- it is a fallback that runs single-threaded (measured:
99% of one core out of ten).

PyTorch uses every core, and CPU memory pressure degrades into ordinary paging rather than a
driver panic. It also runs unchanged on the 32GB Linux and 64GB Windows machines, where the
memory simply fits.

Two things are simpler here than under MLX:
  * masking is `labels = -100`, which CrossEntropyLoss ignores natively -- no custom loss,
    no custom batch iterator, none of the machinery train_lora.py needed
  * attn_implementation="sdpa" never materialises the attention matrix, which at 7800 tokens
    is 3.63GB per layer across 6 attention layers -- the actual driver of the MLX peak

Usage:
    python3 train_torch.py --iters 5 --bench     # time a few steps, then stop
    python3 train_torch.py --iters 400
    python3 train_torch.py --iters 400 --resume
"""
import argparse
import json
import pathlib
import time

import torch
from peft import LoraConfig, get_peft_model, PeftModel
from transformers import AutoModelForCausalLM, AutoTokenizer

HERE = pathlib.Path(__file__).parent
MODEL = "LiquidAI/LFM2.5-1.2B-Instruct"        # PyTorch weights, not the MLX conversion
TEMPLATE = (HERE / "chat_template.jinja").read_text(encoding="utf-8")


def build_examples(path, tokenizer, mask_mode):
    """(input_ids, labels) per record; labels are -100 everywhere except the trained spans."""
    examples = []
    for line in path.open(encoding="utf-8"):
        if not line.strip():
            continue
        record = json.loads(line)
        messages, tools = record["messages"], record.get("tools")
        encoded = tokenizer.apply_chat_template(
            messages, tools=tools, chat_template=TEMPLATE, tokenize=True,
            return_dict=True, return_assistant_tokens_mask=True)
        input_ids = encoded["input_ids"]

        if mask_mode == "assistant":
            keep = encoded["assistant_masks"]
            if not any(keep):
                raise SystemExit("assistant mask empty -- template lost its {% generation %} "
                                 "markers, every record would contribute zero loss")
        elif mask_mode == "prefix":
            prefix = len(tokenizer.apply_chat_template(
                messages[:1], tools=tools, chat_template=TEMPLATE, tokenize=True))
            keep = [0] * prefix + [1] * (len(input_ids) - prefix)
        else:
            keep = [1] * len(input_ids)

        labels = [tid if k else -100 for tid, k in zip(input_ids, keep)]
        examples.append((input_ids, labels))
    return examples


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--mask", choices=["assistant", "prefix", "none"], default="assistant")
    p.add_argument("--iters", type=int, default=400)
    p.add_argument("--rank", type=int, default=16)
    p.add_argument("--lora-alpha", type=int, default=32)
    p.add_argument("--learning-rate", type=float, default=1e-4)
    p.add_argument("--threads", type=int, default=0, help="0 = all cores")
    p.add_argument("--adapter-path", default="adapters-torch")
    p.add_argument("--resume", action="store_true")
    p.add_argument("--save-every", type=int, default=25)
    p.add_argument("--eval-every", type=int, default=50)
    p.add_argument("--eval-batches", type=int, default=8)
    p.add_argument("--milestones", type=int, nargs="*", default=[150],
                   help="steps to also keep as a separate numbered adapter")
    p.add_argument("--dtype", choices=["float32", "bfloat16"], default="bfloat16",
                   help="float32 doubles weight and activation memory")
    p.add_argument("--bench", action="store_true", help="report per-step timing and exit")
    args = p.parse_args()

    torch.set_num_threads(args.threads or _all_cores())
    print(f"torch {torch.__version__}, {torch.get_num_threads()} threads")

    tokenizer = AutoTokenizer.from_pretrained(MODEL)
    train = build_examples(HERE / "data" / "train.jsonl", tokenizer, args.mask)
    valid = build_examples(HERE / "data" / "valid.jsonl", tokenizer, args.mask)
    trained = sum(sum(1 for x in lab if x != -100) for _, lab in train)
    total = sum(len(ids) for ids, _ in train)
    print(f"mask={args.mask}: {trained:,} of {total:,} tokens ({trained/total:.1%})")

    model = AutoModelForCausalLM.from_pretrained(
        MODEL, dtype=getattr(torch, args.dtype), attn_implementation="sdpa")
    model.gradient_checkpointing_enable()
    model.enable_input_require_grads()

    adapter_path = HERE / args.adapter_path
    if args.resume:
        if not (adapter_path / "adapter_config.json").exists():
            raise SystemExit(f"--resume, but no adapter at {adapter_path}")
        model = PeftModel.from_pretrained(model, str(adapter_path), is_trainable=True)
        print(f"resumed from {adapter_path.name}")
    else:
        model = get_peft_model(model, LoraConfig(
            r=args.rank, lora_alpha=args.lora_alpha, lora_dropout=0.0,
            target_modules="all-linear", task_type="CAUSAL_LM"))
    model.print_trainable_parameters()
    model.train()

    optimizer = torch.optim.AdamW(
        [p for p in model.parameters() if p.requires_grad], lr=args.learning_rate)

    def run(example):
        ids, labels = example
        out = model(input_ids=torch.tensor([ids]), labels=torch.tensor([labels]))
        return out.loss

    @torch.no_grad()
    def evaluate():
        model.eval()
        losses = [run(v).item() for v in valid[:args.eval_batches]]
        model.train()
        return sum(losses) / len(losses)

    started = time.time()
    order = list(range(len(train)))
    for step in range(1, args.iters + 1):
        t0 = time.time()
        loss = run(train[order[(step - 1) % len(order)]])
        loss.backward()
        optimizer.step()
        optimizer.zero_grad(set_to_none=True)
        took = time.time() - t0

        if args.bench or step % 10 == 0:
            elapsed = time.time() - started
            per_it = elapsed / step
            remaining = per_it * (args.iters - step)
            print(f"Iter {step}/{args.iters} ({step/args.iters:5.1%})  loss {loss.item():.3f}  "
                  f"{took:.0f}s/it  laeuft {elapsed/3600:.1f}h  "
                  f"Rest ~{remaining/3600:.1f}h (fertig ~{_clock(remaining)})  "
                  f"RSS {_rss_gb():.1f} GB", flush=True)
        if args.bench and step >= 3:
            per_it = (time.time() - started) / step
            print(f"\n{per_it:.1f}s pro Iteration -> eine Epoche (381) = {per_it*381/3600:.1f} h")
            return
        if step % args.eval_every == 0:
            print(f"Iter {step}: val loss {evaluate():.3f}", flush=True)
        if step % args.save_every == 0:
            # The rolling copy is what --resume picks up; the numbered ones are kept so a
            # short run and a long run can be compared without training twice.
            model.save_pretrained(str(adapter_path))
            if step in args.milestones:
                model.save_pretrained(str(adapter_path / f"step-{step}"))
                print(f"Iter {step}: milestone -> step-{step}", flush=True)
            print(f"Iter {step}: saved -> {adapter_path.name}", flush=True)

    model.save_pretrained(str(adapter_path))
    print(f"\ndone in {(time.time()-started)/60:.1f} min -> {adapter_path}")


def _all_cores():
    import os
    return os.cpu_count() or 8


def _clock(seconds_from_now):
    """Wall-clock time this run is expected to finish at."""
    import datetime
    return (datetime.datetime.now()
            + datetime.timedelta(seconds=seconds_from_now)).strftime("%H:%M")


def _rss_gb():
    import resource
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 2**30


if __name__ == "__main__":
    main()
