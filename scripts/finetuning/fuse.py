"""Rechnet einen LoRA-Adapter ins Basismodell ein und legt das Ergebnis fuer LM Studio ab.

    Basismodell + Adapter  ->  zusammengefuehrt  ->  MLX  ->  Modellordner von LM Studio

Die Umwandlung nach MLX ist noetig, weil LM Studio GGUF oder MLX bedient, aber keine
blanken PyTorch-Gewichte.

Die Chat-Vorlage wird ueber das Ergebnis geschrieben und danach geprueft: Manche Modelle
liefern eine Vorlage aus, die tool_calls verschluckt und jeden Assistenz-Turn leer rendert.
Ein so bedientes Modell wirkt wie ein misslungenes Training, obwohl nur die Vorlage falsch
ist. Schlaegt die Pruefung fehl, bricht das Skript ab, statt so etwas auszuliefern.

Der Modellordner von LM Studio ist einstellbar und liegt nicht immer am Standardort; ohne
--lmstudio-dir wird er aus dessen settings.json gelesen. Liegt er auf einem externen
Volume, verweigert macOS unter Umstaenden den Zugriff -- dann bricht das Skript mit einem
Hinweis ab und der Ordner muss von Hand kopiert werden.

Beispiele:
    python3 fuse.py --adapter Finetuning/adapters --name mein-modell \\
                    --template data/qapla-chatbot/chat_template.jinja
    python3 fuse.py --adapter a/ --name m --bits 4          # kleiner, ungenauer
    python3 fuse.py --adapter a/ --name m --no-install      # nur bauen
"""
import argparse
import json
import pathlib
import shutil
import subprocess
import sys


def lmstudio_models_dir():
    """Der eingestellte Modellordner, nicht der Standardort.

    LM Studio laesst ihn verschieben ("downloadsFolder"). Wer in den Standardpfad
    installiert, obwohl er verschoben wurde, bekommt keinen Fehler: Das Modell taucht
    einfach nirgends auf.
    """
    settings = pathlib.Path.home() / ".lmstudio" / "settings.json"
    try:
        configured = json.loads(settings.read_text(encoding="utf-8")).get("downloadsFolder")
        if configured:
            return pathlib.Path(configured)
    except Exception:
        pass
    return pathlib.Path.home() / ".lmstudio" / "models"


def align_config(config_file):
    """Feldnamen angleichen, die zwischen PyTorch- und MLX-Fassung abweichen.

    LFM2.5 fuehrt in der PyTorch-Fassung rope_parameters und tie_word_embeddings, waehrend
    mlx-lm rope_theta, tie_embedding und block_ff_dim erwartet. Ohne das bricht die
    Umwandlung mit "missing 1 required positional argument: 'block_ff_dim'" ab.
    """
    config = json.loads(config_file.read_text(encoding="utf-8"))
    changed = False
    if "rope_theta" not in config and "rope_parameters" in config:
        config["rope_theta"] = config["rope_parameters"].get("rope_theta", 1000000.0)
        changed = True
    if "tie_embedding" not in config and "tie_word_embeddings" in config:
        config["tie_embedding"] = config["tie_word_embeddings"]
        changed = True
    if "block_ff_dim" not in config and config.get("model_type") == "lfm2":
        config["block_ff_dim"] = 12288
        changed = True
    if changed:
        config_file.write_text(json.dumps(config, indent=2), encoding="utf-8")
    return changed


def check_template(model_dir, sample_file):
    """Rendert einen echten Record und prueft, ob die Werkzeugaufrufe darin auftauchen."""
    if not sample_file.exists():
        print(f"  (keine Stichprobe unter {sample_file}, Vorlage ungeprueft)")
        return
    from transformers import AutoTokenizer
    tokenizer = AutoTokenizer.from_pretrained(str(model_dir))
    record = json.loads(sample_file.read_text(encoding="utf-8").splitlines()[0])
    expected = [c["function"]["name"]
                for m in record["messages"] for c in (m.get("tool_calls") or [])]
    if not expected:
        return
    text = tokenizer.apply_chat_template(record["messages"], tools=record.get("tools"),
                                         tokenize=False)
    missing = [n for n in expected if f"{n}(" not in text]
    if missing:
        raise SystemExit(
            f"Die Vorlage in {model_dir} verschluckt die Aufrufe {missing}.\n"
            "Mit --template eine Vorlage angeben, die tool_calls rendert.")
    print(f"  Vorlage geprueft: {len(expected)} Werkzeugaufrufe werden gerendert")


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--adapter", type=pathlib.Path, required=True,
                   help="Verzeichnis mit dem PEFT-Adapter")
    p.add_argument("--name", required=True, help="Modellname fuer LM Studio")
    p.add_argument("--base", default="LiquidAI/LFM2.5-1.2B-Instruct",
                   help="Basismodell, gegen das der Adapter trainiert wurde. Muss dasselbe "
                        "sein wie beim Training, sonst passt der Adapter nicht")
    p.add_argument("--template", type=pathlib.Path,
                   help="Chat-Vorlage (.jinja), die ueber das Ergebnis geschrieben wird")
    p.add_argument("--bits", type=int, default=8, choices=[4, 6, 8],
                   help="Quantisierung. 8 entspricht dem, was LM Studio ueblicherweise "
                        "ausliefert -- gleiche Genauigkeit macht Vorher/Nachher vergleichbar")
    p.add_argument("--work", type=pathlib.Path, default=pathlib.Path("Finetuning"),
                   help="Arbeitsverzeichnis fuer Zwischenstaende")
    p.add_argument("--sample", type=pathlib.Path,
                   help="JSONL, dessen erste Zeile zur Pruefung der Vorlage gerendert wird "
                        "(Vorgabe: <work>/data/test.jsonl)")
    p.add_argument("--publisher", default="qapla", help="Herausgeber-Ordner in LM Studio")
    p.add_argument("--lmstudio-dir", type=pathlib.Path,
                   help="Modellordner von LM Studio (Vorgabe: aus dessen settings.json)")
    p.add_argument("--no-install", action="store_true",
                   help="nur bauen, nicht in LM Studio ablegen")
    args = p.parse_args()

    import torch
    from peft import PeftModel
    from transformers import AutoModelForCausalLM, AutoTokenizer

    if not (args.adapter / "adapter_config.json").exists():
        raise SystemExit(f"kein Adapter unter {args.adapter}")

    merged = args.work / "merged" / args.name
    print(f"rechne {args.adapter} in {args.base} ein")
    model = AutoModelForCausalLM.from_pretrained(args.base, dtype=torch.float32)
    model = PeftModel.from_pretrained(model, str(args.adapter)).merge_and_unload()
    merged.mkdir(parents=True, exist_ok=True)
    model.save_pretrained(str(merged))
    AutoTokenizer.from_pretrained(args.base).save_pretrained(str(merged))
    del model

    if align_config(merged / "config.json"):
        print("  Konfigurationsfelder an mlx-lm angeglichen")

    mlx_out = args.work / "mlx" / args.name
    if mlx_out.exists():
        shutil.rmtree(mlx_out)
    print(f"wandle nach MLX ({args.bits} bit)")
    subprocess.run([sys.executable, "-m", "mlx_lm", "convert", "--hf-path", str(merged),
                    "--mlx-path", str(mlx_out), "-q", "--q-bits", str(args.bits)], check=True)

    if args.template:
        shutil.copy(args.template, mlx_out / "chat_template.jinja")
        config_file = mlx_out / "tokenizer_config.json"
        if config_file.exists():
            config = json.loads(config_file.read_text(encoding="utf-8"))
            if "chat_template" in config:
                # eine hier eingebettete Vorlage sticht die .jinja-Datei aus
                config["chat_template"] = args.template.read_text(encoding="utf-8")
                config_file.write_text(json.dumps(config, indent=2, ensure_ascii=False),
                                       encoding="utf-8")
    check_template(mlx_out, args.sample or args.work / "data" / "test.jsonl")

    if args.no_install:
        print(f"\nfertig: {mlx_out}")
        return

    target = (args.lmstudio_dir or lmstudio_models_dir()) / args.publisher / args.name
    try:
        if target.exists():
            shutil.rmtree(target)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(mlx_out, target)
    except (PermissionError, OSError) as error:
        raise SystemExit(
            f"Kein Zugriff auf {target}: {error}\n"
            f"Das Modell liegt fertig unter {mlx_out} -- von Hand dorthin kopieren.")
    print(f"\nabgelegt: {target}\n  lms load {args.name} -c 12288 -y")


if __name__ == "__main__":
    main()
