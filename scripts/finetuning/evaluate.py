"""Misst ein Modell gegen Testfaelle: Wird gehandelt, und wird richtig gehandelt?

Schickt je Fall genau das, was die Anwendung am Anfang eines Turns schickt -- alles bis
einschliesslich der Nutzernachricht, dazu die Werkzeugbeschreibungen -- und vergleicht die
Antwort mit dem, was im Datensatz steht.

Zwei Zahlen:

  acted    ueberhaupt ein Werkzeugaufruf statt Prosa. Das ist die eigentliche Zielgroesse,
           wenn die Anwendung nur Werkzeugaufrufe anzeigen kann: Prosa ist dann keine
           schlechtere Antwort, sondern gar keine.
  correct  ...und es war das Werkzeug, das der Datensatz zuerst aufruft.

Bewertet wird nur der erste Schritt. Die ganze Kette zu pruefen braeuchte die echten
Werkzeuge, um das naechste Ergebnis zu erzeugen -- und nach einem Fehlgriff im ersten
Schritt ist ohnehin nichts mehr vergleichbar.

Zur Aussagekraft: Enthaelt der Datensatz jeden Fall mehrfach (Varianten desselben
Records), sind das keine unabhaengigen Messpunkte. 36 Zeilen aus 12 Gruppen tragen so viel
Information wie 12 Faelle, nicht wie 36.

Setzt einen laufenden LM-Studio-Server voraus oder einen anderen OpenAI-kompatiblen
Endpunkt.

Beispiele:
    python3 evaluate.py --dataset Finetuning/data/test.jsonl
    python3 evaluate.py --dataset t.jsonl --out ergebnis.json --model mein-modell
"""
import argparse
import collections
import json
import pathlib
import urllib.request


def post(endpoint, path, body, timeout=600):
    request = urllib.request.Request(
        f"{endpoint}{path}", data=json.dumps(body).encode("utf-8"),
        headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return json.load(response)


def turn_start(messages):
    """Alles bis einschliesslich der letzten Nutzernachricht -- der Zustand bei Turn-Beginn."""
    last_user = max(i for i, m in enumerate(messages) if m["role"] == "user")
    return messages[:last_user + 1]


def expected_first_tool(messages):
    for message in messages:
        for call in message.get("tool_calls") or []:
            return call["function"]["name"]
    return None


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--dataset", type=pathlib.Path, required=True,
                   help="JSONL mit den Testfaellen (aus split.py)")
    p.add_argument("--model", help="Modellkennung; ohne Angabe das in LM Studio geladene")
    p.add_argument("--out", type=pathlib.Path,
                   help="Ergebnisse als JSON sichern, fuer Vorher/Nachher-Vergleiche")
    p.add_argument("--endpoint", default="http://localhost:1234/v1",
                   help="OpenAI-kompatibler Endpunkt")
    p.add_argument("--max-tokens", type=int, default=400,
                   help="Obergrenze je Antwort; nur der erste Aufruf wird bewertet")
    args = p.parse_args()

    model = args.model
    if not model:
        with urllib.request.urlopen(f"{args.endpoint}/models", timeout=30) as response:
            loaded = json.load(response)["data"]
        if not loaded:
            raise SystemExit("In LM Studio ist kein Modell geladen (lms load <name>)")
        model = loaded[0]["id"]

    lines = [json.loads(l) for l in args.dataset.open(encoding="utf-8") if l.strip()]
    groups = {l.get("group") for l in lines if "group" in l}
    print(f"Modell: {model}")
    print(f"Datei:  {args.dataset} ({len(lines)} Faelle"
          + (f" aus {len(groups)} Gruppen)" if groups else ")") + "\n")

    results, acted, correct = [], 0, 0
    confusion = collections.Counter()
    for i, record in enumerate(lines, 1):
        want = expected_first_tool(record["messages"])
        reply = post(args.endpoint, "/chat/completions", {
            "model": model,
            "messages": turn_start(record["messages"]),
            "tools": record.get("tools"),
            "tool_choice": "required",
            "temperature": 0,
            "max_tokens": args.max_tokens,
        })["choices"][0]["message"]

        calls = reply.get("tool_calls") or []
        got = calls[0]["function"]["name"] if calls else None
        acted += bool(got)
        correct += got == want
        confusion[(want, got)] += 1
        results.append({"want": want, "got": got,
                        "content": (reply.get("content") or "")[:300]})
        print(f"  [{i:3}/{len(lines)}] soll {want or '-':24} ist {got or 'PROSA':24}"
              f" {'ok' if got == want else 'x'}")

    n = len(lines)
    print(f"\nacted   {acted}/{n} = {acted/n:.1%}   (ueberhaupt ein Werkzeugaufruf)")
    print(f"correct {correct}/{n} = {correct/n:.1%}   (das richtige Werkzeug)")
    misses = [(w, g, c) for (w, g), c in confusion.most_common() if w != g]
    if misses:
        print("\nhaeufigste Fehlgriffe:")
        for want, got, count in misses[:8]:
            print(f"  {want or '-':24} -> {got or 'PROSA':24} {count}x")

    if args.out:
        args.out.write_text(json.dumps(
            {"model": model, "dataset": str(args.dataset), "acted": acted,
             "correct": correct, "n": n, "results": results},
            indent=2, ensure_ascii=False), encoding="utf-8")
        print(f"\ngesichert: {args.out}")


if __name__ == "__main__":
    main()
