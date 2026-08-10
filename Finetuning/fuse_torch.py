"""Merges a PyTorch/PEFT adapter into the base model and converts it for LM Studio.

train_torch.py produces PEFT adapters against the PyTorch weights, which LM Studio cannot
load: it serves GGUF or MLX, not plain safetensors. So this merges the adapter, then hands
the result to mlx_lm.convert, which reads a HuggingFace checkpoint and writes MLX -- the same
format the GPU-trained models used, and one LM Studio already loads.

The chat template is written over the result for the same reason fuse_deploy.py does it: the
template that ships with the model drops tool_calls entirely, and a model that renders every
assistant turn as empty looks like a failed training rather than a broken template.

Usage:
    python3 fuse_torch.py --adapter adapters-torch/step-150 --name lfm2.5-1.2b-qapla-150
    python3 fuse_torch.py --adapter adapters-torch --name lfm2.5-1.2b-qapla-381
"""
import argparse
import json
import pathlib
import shutil
import subprocess
import sys

HERE = pathlib.Path(__file__).parent
BASE = "LiquidAI/LFM2.5-1.2B-Instruct"
TEMPLATE = HERE / "chat_template.jinja"


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--adapter", required=True, help="path to the PEFT adapter directory")
    p.add_argument("--name", required=True, help="model name for LM Studio")
    p.add_argument("--publisher", default="qapla")
    p.add_argument("--quantize", action="store_true", help="4bit instead of full precision")
    args = p.parse_args()

    import torch
    from peft import PeftModel
    from transformers import AutoModelForCausalLM, AutoTokenizer

    adapter = HERE / args.adapter
    if not (adapter / "adapter_config.json").exists():
        raise SystemExit(f"no adapter at {adapter}")

    merged = HERE / "merged" / args.name
    print(f"merging {adapter.relative_to(HERE)} into {BASE}")
    model = AutoModelForCausalLM.from_pretrained(BASE, dtype=torch.float32)
    model = PeftModel.from_pretrained(model, str(adapter))
    model = model.merge_and_unload()
    merged.mkdir(parents=True, exist_ok=True)
    model.save_pretrained(str(merged))
    AutoTokenizer.from_pretrained(BASE).save_pretrained(str(merged))
    del model

    # LiquidAI's PyTorch config and the MLX one disagree on field names, and mlx-lm reads the
    # MLX spelling: it wants block_ff_dim (absent here), rope_theta (called rope_parameters)
    # and tie_embedding (called tie_word_embeddings). Without this the conversion dies on
    # "ModelArgs.__init__() missing 1 required positional argument: 'block_ff_dim'".
    config_file = merged / "config.json"
    config = json.loads(config_file.read_text(encoding="utf-8"))
    config.setdefault("rope_theta", (config.get("rope_parameters") or {}).get("rope_theta", 1000000.0))
    config.setdefault("tie_embedding", config.get("tie_word_embeddings", True))
    if "block_ff_dim" not in config:
        # LFM2's conv blocks: multiple_of-aligned expansion of block_dim, 12288 for the 1.2B
        config["block_ff_dim"] = 12288
    config_file.write_text(json.dumps(config, indent=2), encoding="utf-8")

    mlx_out = HERE / "mlx" / args.name
    if mlx_out.exists():
        shutil.rmtree(mlx_out)
    cmd = [sys.executable, "-m", "mlx_lm", "convert", "--hf-path", str(merged),
           "--mlx-path", str(mlx_out)]
    if args.quantize:
        cmd += ["-q"]
    print("converting to MLX ...")
    subprocess.run(cmd, check=True)

    # the converted copy carries the base repo's template, which loses tool_calls
    shutil.copy(TEMPLATE, mlx_out / "chat_template.jinja")
    config_path = mlx_out / "tokenizer_config.json"
    if config_path.exists():
        config = json.loads(config_path.read_text(encoding="utf-8"))
        if "chat_template" in config:
            config["chat_template"] = TEMPLATE.read_text(encoding="utf-8")
            config_path.write_text(json.dumps(config, indent=2, ensure_ascii=False),
                                   encoding="utf-8")

    # prove the tool calls survive rendering before anyone tries to serve this
    from transformers import AutoTokenizer as T
    tok = T.from_pretrained(str(mlx_out))
    record = json.loads((HERE / "data" / "test.jsonl").read_text(encoding="utf-8").splitlines()[0])
    want = [c["function"]["name"]
            for m in record["messages"] for c in (m.get("tool_calls") or [])]
    text = tok.apply_chat_template(record["messages"], tools=record["tools"], tokenize=False)
    missing = [n for n in want if f"{n}(" not in text]
    if missing:
        raise SystemExit(f"template drops tool calls {missing} -- refusing to hand this over")
    print(f"template check: {len(want)} tool calls render ({', '.join(want)})")

    print(f"\nfertig: {mlx_out}")
    print(f"Nach /Volumes/T7/models/{args.publisher}/{args.name} kopieren, dann:")
    print(f"  lms load {args.name} -c 12288 -y")
    print(f"  python3 eval_lmstudio.py --split test --out tuned-{args.name}.json")


if __name__ == "__main__":
    main()
