"""Fuses the LoRA adapter into the base model and installs it for LM Studio.

The install step is not a copy -- it exists because of chat_template.jinja. mlx_lm.fuse writes
out whatever template the base model repo carried, and LFM2.5's MLX repo ships one that has no
handling for tool_calls at all: every assistant turn renders as empty. Serving the fused model
with it would make the finetuned behaviour unreachable, and it would look like the training
failed rather than like the template dropped the output.

So after fusing, the working template is written over it and then verified by rendering a real
record and checking the tool call actually appears. A deploy that cannot prove that fails here
rather than in the GUI.

Usage:
    python3 fuse_deploy.py                     # fuse + install as qapla/<name>
    python3 fuse_deploy.py --no-install        # fuse only, leave LM Studio alone
"""
import argparse
import json
import pathlib
import shutil
import subprocess
import sys

HERE = pathlib.Path(__file__).parent
# Must match train_lora.py's base -- an adapter is only valid against the weights it saw.
MODEL = "LiquidAI/LFM2.5-1.2B-Instruct-MLX-4bit"
LMSTUDIO_MODELS = pathlib.Path.home() / ".lmstudio" / "models"
TEMPLATE = HERE / "chat_template.jinja"


def verify_template(model_dir):
    """Render a real record and assert its tool calls survive."""
    from transformers import AutoTokenizer
    tokenizer = AutoTokenizer.from_pretrained(str(model_dir))
    record = json.loads((HERE / "data" / "test.jsonl").read_text(encoding="utf-8").splitlines()[0])
    expected = [c["function"]["name"]
                for m in record["messages"] for c in (m.get("tool_calls") or [])]
    if not expected:
        raise SystemExit("first test record has no tool calls -- cannot verify the template")
    text = tokenizer.apply_chat_template(record["messages"], tools=record["tools"], tokenize=False)
    missing = [n for n in expected if f"{n}(" not in text]
    if missing:
        raise SystemExit(
            f"template in {model_dir} drops tool calls {missing} -- refusing to deploy. "
            "This is the MLX repo's template, not chat_template.jinja.")
    print(f"template check: all {len(expected)} tool calls render ({', '.join(expected)})")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--adapter-path", default="adapters")
    p.add_argument("--save-path", default="fused")
    p.add_argument("--name", default="lfm2.5-1.2b-qapla")
    p.add_argument("--publisher", default="qapla")
    p.add_argument("--no-install", action="store_true")
    args = p.parse_args()

    adapters = HERE / args.adapter_path
    if not (adapters / "adapters.safetensors").exists():
        raise SystemExit(f"no adapter at {adapters}")

    fused = HERE / args.save_path
    print(f"fusing {MODEL} + {adapters.name} -> {fused.name}")
    subprocess.run([sys.executable, "-m", "mlx_lm", "fuse",
                    "--model", MODEL,
                    "--adapter-path", str(adapters),
                    "--save-path", str(fused)], check=True)

    # fuse copies the base repo's template, which is the broken one -- overwrite it
    shutil.copy(TEMPLATE, fused / "chat_template.jinja")
    config_path = fused / "tokenizer_config.json"
    if config_path.exists():
        config = json.loads(config_path.read_text(encoding="utf-8"))
        if "chat_template" in config:
            # a template embedded here wins over the .jinja file, so it has to go too
            config["chat_template"] = TEMPLATE.read_text(encoding="utf-8")
            config_path.write_text(json.dumps(config, indent=2, ensure_ascii=False),
                                   encoding="utf-8")
            print("replaced the template embedded in tokenizer_config.json as well")
    verify_template(fused)

    if args.no_install:
        print(f"\nfused model at {fused} (not installed)")
        return

    target = LMSTUDIO_MODELS / args.publisher / args.name
    if target.exists():
        shutil.rmtree(target)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(fused, target)
    verify_template(target)
    print(f"\ninstalled -> {target}")
    print(f"load it in LM Studio as {args.publisher}/{args.name}, then:")
    print(f"  python3 eval_lmstudio.py --split test --out tuned.json")


if __name__ == "__main__":
    main()
