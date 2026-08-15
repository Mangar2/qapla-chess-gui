"""LoRA-Finetuning eines Chat-Modells auf der CPU.

Warum CPU: Auf Geraeten, deren GPU-Speicher knapp ist, bricht MLX-Training mit
Treiber-Panics ab (siehe docs/finetuning-training.md). PyTorch nutzt alle Kerne, und
knapper Hauptspeicher fuehrt zu Auslagerung statt zum Absturz.

Trainiert wird nur auf den Assistenz-Antworten (--mask assistant): Bei langen Prompts
-- Systemprompt plus Werkzeugbeschreibungen -- sind das oft nur ein Prozent der Token.
Ohne Maskierung ginge fast das gesamte Lernsignal in das Nachsprechen des Prompts.

Speicherbedarf grob: 7 GB je Milliarde Parameter plus 2-3 GB.

Beispiele:
    python3 train.py --data Finetuning/data --adapter-path Finetuning/adapters --iters 200
    python3 train.py --data d/ --adapter-path a/ --model Qwen/Qwen3-1.7B --iters 400
    python3 train.py --data d/ --adapter-path a/ --iters 3 --bench    # nur Zeitmessung
    python3 train.py --data d/ --adapter-path a/ --iters 400 --resume # nach Abbruch
"""
import argparse
import json
import pathlib
import time

import torch
from peft import LoraConfig, get_peft_model, PeftModel
from transformers import AutoModelForCausalLM, AutoTokenizer



def build_examples(path, tokenizer, mask_mode, template):
    """(input_ids, labels) per record; labels are -100 everywhere except the trained spans."""
    examples = []
    for line in path.open(encoding="utf-8"):
        if not line.strip():
            continue
        record = json.loads(line)
        messages, tools = record["messages"], record.get("tools")
        encoded = tokenizer.apply_chat_template(
            messages, tools=tools, chat_template=template, tokenize=True,
            return_dict=True, return_assistant_tokens_mask=True)
        input_ids = encoded["input_ids"]

        if mask_mode == "assistant":
            keep = encoded["assistant_masks"]
            if not any(keep):
                raise SystemExit("assistant mask empty -- template lost its {% generation %} "
                                 "markers, every record would contribute zero loss")
        elif mask_mode == "prefix":
            prefix = len(tokenizer.apply_chat_template(
                messages[:1], tools=tools, chat_template=template, tokenize=True))
            keep = [0] * prefix + [1] * (len(input_ids) - prefix)
        else:
            keep = [1] * len(input_ids)

        labels = [tid if k else -100 for tid, k in zip(input_ids, keep)]
        examples.append((input_ids, labels))
    return examples


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--model", default="LiquidAI/LFM2.5-1.2B-Instruct",
                   help="HuggingFace-Kennung oder lokaler Pfad des Basismodells")
    p.add_argument("--data", type=pathlib.Path, required=True,
                   help="Verzeichnis mit train.jsonl und valid.jsonl (aus split.py)")
    p.add_argument("--template", type=pathlib.Path,
                   help="Chat-Vorlage (.jinja). Ohne Angabe die des Modells -- pruefen, "
                        "ob sie tool_calls rendert, sonst wird ohne Wirkung trainiert")
    p.add_argument("--mask", choices=["assistant", "prefix", "none"], default="assistant",
                   help="assistant: nur die Antworten des Modells (Vorgabe). prefix: alles nach dem Systemprompt. none: alles -- nur zum Vergleichen")
    p.add_argument("--iters", type=int, default=400,
                   help="Trainingsschritte, je ein Record")
    p.add_argument("--rank", type=int, default=16,
                   help="LoRA-Rang: mehr Rang, mehr Kapazitaet")
    p.add_argument("--lora-alpha", type=int, default=32,
                   help="Skalierung des Adapters. alpha/rank ist die effektive Schrittweite; zu hoch laesst den Verlust divergieren")
    p.add_argument("--learning-rate", type=float, default=1e-4,
                   help="Lernrate")
    p.add_argument("--threads", type=int, default=0, help="Rechenkerne, 0 = alle")
    p.add_argument("--adapter-path", default="adapters-torch")
    p.add_argument("--resume", action="store_true",
                   help="auf vorhandenem Adapter aufsetzen, etwa nach Abbruch")
    p.add_argument("--save-every", type=int, default=25,
                   help="Adapter alle N Schritte sichern")
    p.add_argument("--eval-every", type=int, default=50,
                   help="alle N Schritte gegen valid.jsonl pruefen. Kostet Zeit, deshalb sparsam")
    p.add_argument("--eval-batches", type=int, default=8,
                   help="wie viele Validierungsrecords je Pruefung")
    p.add_argument("--milestones", type=int, nargs="*", default=[150],
                   help="Schritte, deren Stand zusaetzlich separat aufgehoben wird -- so lassen sich kurzer und langer Lauf vergleichen, ohne zweimal zu trainieren")
    p.add_argument("--dtype", choices=["float32", "bfloat16"], default="bfloat16",
                   help="float32 ist auf CPU der einzige brauchbare Weg; bfloat16 ist dort um Groessenordnungen langsamer (gemessen)")
    p.add_argument("--bench", action="store_true", help="nur die Zeit je Schritt messen und beenden")
    args = p.parse_args()

    torch.set_num_threads(args.threads or _all_cores())
    print(f"torch {torch.__version__}, {torch.get_num_threads()} threads")

    template = args.template.read_text(encoding="utf-8") if args.template else None
    tokenizer = AutoTokenizer.from_pretrained(args.model)
    train = build_examples(args.data / "train.jsonl", tokenizer, args.mask, template)
    valid = build_examples(args.data / "valid.jsonl", tokenizer, args.mask, template)
    trained = sum(sum(1 for x in lab if x != -100) for _, lab in train)
    total = sum(len(ids) for ids, _ in train)
    print(f"mask={args.mask}: {trained:,} of {total:,} tokens ({trained/total:.1%})")

    model = AutoModelForCausalLM.from_pretrained(
        args.model, dtype=getattr(torch, args.dtype), attn_implementation="sdpa")
    model.gradient_checkpointing_enable()
    model.enable_input_require_grads()

    adapter_path = args.adapter_path
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
