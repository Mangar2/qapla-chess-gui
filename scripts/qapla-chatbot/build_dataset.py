"""Baut aus den Datendateien den Trainingsdatensatz im OpenAI-Chatformat.

    records.json + worlds.json + system-prompt.txt  ->  dataset.jsonl + ledger.md

records.json ist die einzige Stelle, an der die Records selbst stehen. Wird ein Tool
umbenannt oder ein Argument umgebaut, ändert sich nur diese Datei -- kein Skript.

Jede Ausgabezeile trägt zusätzlich "group": den Index des Basis-Records. Die Welten
desselben Records haben dieselbe Gruppe, und darauf verlässt sich das Aufteilen in
Train/Valid/Test (scripts/finetuning/split.py): Landet eine Welt im Training und eine
andere in der Validierung, misst die Validierung Auswendiggelerntes.

Beispiele:
    python3 build_dataset.py
    python3 build_dataset.py --data-dir data/qapla-chatbot --out dataset-002.jsonl
    python3 build_dataset.py --no-worlds        # nur die 151 Basis-Records
"""
import argparse
import json
import pathlib
import sys


def substitute(text, rules):
    """Textersetzung nach Regelliste; Reihenfolge zählt (längere Muster zuerst)."""
    if not isinstance(text, str):
        return text
    for old, new in rules:
        text = text.replace(old, new)
    return text


def map_values(value, rules):
    """Rekursiv: Argumente sind auch Listen (engines=[...]) und verschachtelte Objekte."""
    if isinstance(value, str):
        return substitute(value, rules)
    if isinstance(value, list):
        return [map_values(v, rules) for v in value]
    if isinstance(value, dict):
        return {k: map_values(v, rules) for k, v in value.items()}
    return value


def apply_world(record, world_index, world, paraphrases):
    """Denselben Record in einer anderen Welt: alle Textfelder durch dieselben Regeln."""
    rules = world["rules"]
    variant = paraphrases.get(str(record["index"]))
    if variant and world_index < len(variant):
        user, reply = variant[world_index]
    else:
        user, reply = record["user"], record["reply"]
    return {
        "index": record["index"],
        "variant": world["name"],
        "user": substitute(user, rules),
        "reply": None if reply is None else substitute(reply, rules),
        "context": [[substitute(u, rules), substitute(a, rules)] for u, a in record["context"]],
        "steps": [{"tool": s["tool"], "args": map_values(s["args"], rules),
                   "result": substitute(s["result"], rules)} for s in record["steps"]],
        "note": record["note"],
    }


def to_messages(record, system_prompt):
    """Ein Record im Wire-Format, exakt so wie die App es sendet."""
    messages = [{"role": "system", "content": system_prompt}]
    for user_text, assistant_text in record["context"]:
        messages.append({"role": "user", "content": user_text})
        messages.append({"role": "assistant", "content": assistant_text})
    messages.append({"role": "user", "content": record["user"]})

    tag = record.get("variant", "")
    for position, step in enumerate(record["steps"]):
        call_id = f"call_{record['index']}{tag}_{position}"
        messages.append({"role": "assistant", "content": "", "tool_calls": [{
            "id": call_id, "type": "function",
            "function": {"name": step["tool"],
                         "arguments": json.dumps(step["args"], ensure_ascii=False)}}]})
        messages.append({"role": "tool", "tool_call_id": call_id, "content": step["result"]})

    if record["reply"] is None:
        # Terminales Tool (Dateidialog): der Turn endet am Tool-Ergebnis, das Modell
        # wird nicht erneut gefragt -- eine Antwort kann es hier gar nicht geben.
        return messages

    reply_id = f"call_{record['index']}{tag}_reply"
    messages.append({"role": "assistant", "content": "", "tool_calls": [{
        "id": reply_id, "type": "function",
        "function": {"name": "reply_to_user",
                     "arguments": json.dumps({"text": record["reply"]}, ensure_ascii=False)}}]})
    return messages


def main():
    here = pathlib.Path(__file__).resolve()
    default_data = here.parents[2] / "data" / "qapla-chatbot"
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--data-dir", type=pathlib.Path, default=default_data,
                   help=f"Verzeichnis mit records.json/worlds.json (Vorgabe: {default_data})")
    p.add_argument("--out", default="dataset.jsonl",
                   help="Dateiname des Datensatzes unter <data-dir>/generated/")
    p.add_argument("--ledger", default="ledger.md",
                   help="Dateiname des Review-Protokolls unter <data-dir>/generated/")
    p.add_argument("--no-worlds", action="store_true",
                   help="nur die Basis-Records ausgeben, ohne Wertevariation")
    args = p.parse_args()

    d = args.data_dir
    for name in ("records.json", "worlds.json", "system-prompt.txt"):
        if not (d / name).exists():
            raise SystemExit(f"fehlt: {d / name}")

    records = json.loads((d / "records.json").read_text(encoding="utf-8"))
    worlds_file = json.loads((d / "worlds.json").read_text(encoding="utf-8"))
    prompt = (d / "system-prompt.txt").read_text(encoding="utf-8")
    worlds = [] if args.no_worlds else worlds_file["worlds"]
    paraphrases = worlds_file.get("paraphrases", {})

    emitted = list(records)
    for i, world in enumerate(worlds):
        emitted.extend(apply_world(r, i, world, paraphrases) for r in records)

    out_dir = d / "generated"
    out_dir.mkdir(parents=True, exist_ok=True)
    with (out_dir / args.out).open("w", encoding="utf-8") as handle:
        for record in emitted:
            handle.write(json.dumps({"messages": to_messages(record, prompt),
                                     "group": record["index"]}, ensure_ascii=False) + "\n")

    with (out_dir / args.ledger).open("w", encoding="utf-8") as handle:
        handle.write(f"# Review-Protokoll\n\n{len(records)} Basis-Records")
        if worlds:
            handle.write(", je zusätzlich in den Welten: "
                         + ", ".join(w["name"] for w in worlds))
        handle.write(f" -> {len(emitted)} Zeilen.\n\n")
        handle.write("| # | User-Nachricht | Tools | Änderung |\n|---|---|---|---|\n")
        for record in records:
            tools = " → ".join(s["tool"] for s in record["steps"]) or "—"
            user = record["user"].replace("|", "\\|")
            handle.write(f"| {record['index']} | {user} | {tools} | {record['note']} |\n")

    print(f"{len(emitted)} Zeilen ({len(records)} Basis x {len(worlds) + 1}) "
          f"-> {(out_dir / args.out).relative_to(d.parent.parent)}")


if __name__ == "__main__":
    main()
