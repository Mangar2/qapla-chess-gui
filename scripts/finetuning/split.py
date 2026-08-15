"""Teilt einen Datensatz in Trainings-, Validierungs- und Testteil.

Erwartet JSONL mit einem {"messages": [...]} je Zeile. Trägt eine Zeile zusätzlich
"group", bleiben alle Zeilen derselben Gruppe zusammen in einem Teil. Das ist der
eigentliche Zweck: Wenn ein Datensatz jeden Fall in mehreren Varianten enthält und
eine Variante im Training landet, die andere in der Validierung, dann misst die
Validierung Auswendiggelerntes und sieht gut aus, ohne etwas zu bedeuten.

Optional wird ein "tools"-Array an jede Zeile gehängt (--tools). Nötig, wenn das
Modell zur Laufzeit Werkzeugbeschreibungen mitgeliefert bekommt: Fehlen sie beim
Training, lernt das Modell einen Prompt, den es im Betrieb nie sieht.

Beispiele:
    python3 split.py --dataset data/x/generated/dataset.jsonl --out Finetuning/data
    python3 split.py --dataset d.jsonl --out out/ --tools data/x/tools.json
    python3 split.py --dataset d.jsonl --out out/ --valid 20 --test 20 --seed 7
"""
import argparse
import collections
import json
import pathlib
import random


def load(path):
    return [json.loads(line) for line in path.open(encoding="utf-8") if line.strip()]


def parse_arguments_field(record):
    """tool_calls[].function.arguments: JSON-String -> Objekt.

    Manche Chat-Vorlagen bestehen darauf ("Tool call arguments must be a mapping"),
    während der Datensatz die OpenAI-Konvention als String führt. Betrifft nur das
    Rendern, deshalb hier und nicht im Datensatz.
    """
    for message in record["messages"]:
        for call in message.get("tool_calls") or []:
            if isinstance(call["function"]["arguments"], str):
                call["function"]["arguments"] = json.loads(call["function"]["arguments"])
    return record


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--dataset", type=pathlib.Path, required=True, help="JSONL-Eingabe")
    p.add_argument("--out", type=pathlib.Path, required=True,
                   help="Zielverzeichnis für train/valid/test.jsonl")
    p.add_argument("--tools", type=pathlib.Path,
                   help="JSON mit Werkzeugbeschreibungen, wird an jede Zeile gehängt")
    p.add_argument("--valid", type=int, default=12, help="Gruppen für die Validierung")
    p.add_argument("--test", type=int, default=12, help="Gruppen für den Test")
    p.add_argument("--seed", type=int, default=20260807, help="für einen reproduzierbaren Split")
    p.add_argument("--keep-rare", type=int, default=2,
                   help="Werkzeuge mit höchstens so vielen Gruppen bleiben im Training. "
                        "Ein einziges Beispiel im Holdout nimmt dem Modell das Werkzeug "
                        "ganz und misst trotzdem nichts. 0 schaltet das ab")
    p.add_argument("--parse-arguments", action="store_true",
                   help="tool_calls-Argumente von JSON-String in Objekt wandeln")
    args = p.parse_args()

    records = load(args.dataset)
    tools = json.loads(args.tools.read_text(encoding="utf-8")) if args.tools else None

    groups = collections.OrderedDict()
    for i, record in enumerate(records):
        groups.setdefault(record.get("group", i), []).append(record)
    print(f"{len(records)} Zeilen in {len(groups)} Gruppen")

    pinned = set()
    if args.keep_rare:
        usage = collections.Counter()
        for key, rows in groups.items():
            names = {c["function"]["name"] for r in rows for m in r["messages"]
                     for c in (m.get("tool_calls") or [])}
            usage.update(names)
        rare = {n for n, c in usage.items() if c <= args.keep_rare}
        for key, rows in groups.items():
            names = {c["function"]["name"] for r in rows for m in r["messages"]
                     for c in (m.get("tool_calls") or [])}
            if rare & names:
                pinned.add(key)
        if rare:
            print("selten, bleibt im Training: "
                  + ", ".join(f"{n}({usage[n]})" for n in sorted(rare)))

    free = [k for k in groups if k not in pinned]
    random.Random(args.seed).shuffle(free)
    if len(free) < args.valid + args.test:
        raise SystemExit(f"nur {len(free)} Gruppen frei, {args.valid + args.test} gebraucht")
    valid_keys = set(free[:args.valid])
    test_keys = set(free[args.valid:args.valid + args.test])

    args.out.mkdir(parents=True, exist_ok=True)
    counts = {}
    for name, keys in (("train", [k for k in groups if k not in valid_keys | test_keys]),
                       ("valid", [k for k in groups if k in valid_keys]),
                       ("test", [k for k in groups if k in test_keys])):
        path = args.out / f"{name}.jsonl"
        written = 0
        with path.open("w", encoding="utf-8") as handle:
            for key in keys:
                for record in groups[key]:
                    row = json.loads(json.dumps(record))
                    if args.parse_arguments:
                        row = parse_arguments_field(row)
                    if tools is not None:
                        row["tools"] = tools
                    handle.write(json.dumps(row, ensure_ascii=False) + "\n")
                    written += 1
        counts[name] = (len(keys), written)
        print(f"{name:6} {len(keys):4} Gruppen, {written:4} Zeilen -> {path}")


if __name__ == "__main__":
    main()
