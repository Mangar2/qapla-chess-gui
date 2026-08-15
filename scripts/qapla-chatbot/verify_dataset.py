"""Prueft den erzeugten Datensatz gegen den Katalog, der aus Systemprompt und
Werkzeugbeschreibungen abgeleitet ist.

Geprueft wird unter anderem:

  * Aufbau: system zuerst, jeder Werkzeugaufruf von einem Ergebnis gefolgt, kein Klartext
    neben tool_calls, genau ein Aufruf je Nachricht
  * nur vorhandene Werkzeuge, nur vorhandene Argumentnamen
  * Belegbarkeit: jede Zahl und jeder Pfad in der Antwort muss in einem Werkzeugergebnis,
    der Nutzernachricht oder dem Kontext desselben Records vorkommen. Das ist die zentrale
    Regel -- sie ist mechanisch entscheidbar und faengt genau die Halluzinationen
  * terminale Werkzeuge beenden den Turn: danach darf keine Antwort mehr folgen

Der Gegentest gehoert dazu: Ein Pruefer, der "0 Beanstandungen" meldet, ohne je an
fehlerhaften Daten gezeigt zu haben, dass er ueberhaupt anschlaegt, beweist nichts.

Beispiele:
    python3 verify_dataset.py                                  # erwartet 0
    python3 verify_dataset.py --dataset alte-rohdaten.json     # Gegentest, erwartet >0
"""

import argparse
import json
import re
import sys
import pathlib

REPO = pathlib.Path(__file__).resolve().parents[2]
DEFAULT_DATASET = REPO / "data" / "qapla-chatbot" / "generated" / "dataset.jsonl"
# Defaults to the corrected dataset; pass a path to check any other JSONL file. Running it
# against original.json is the control experiment that proves the checks actually bite --
# a verifier that reports "0 findings" without ever having been shown a bad file proves
# nothing (that mistake is what let the first cleanup pass declare success wrongly).
_parser = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
_parser.add_argument("--dataset", type=pathlib.Path, default=DEFAULT_DATASET,
                     help=f"zu pruefende JSONL-Datei (Vorgabe: {DEFAULT_DATASET})")
DATA = _parser.parse_args().dataset

# Tool name -> allowed argument keys, transcribed from the TOOLS block the app sends.
SCHEMA = {
    "list_installed_engines": set(),
    "open_add_engine_dialog": set(),
    "select_engines": {"engines"},
    "configure_tournament": {
        "concurrency", "draw_centipawn_threshold", "draw_min_full_moves", "draw_mode",
        "draw_required_consecutive_moves", "event", "games", "openings_file",
        "openings_file_dialog", "pgn_file", "pgn_file_dialog", "resign_centipawn_threshold",
        "resign_mode", "resign_required_consecutive_moves", "resign_two_sided", "rounds",
        "time_control"},
    "select_sprt_engines": {"champion", "challenger"},
    "configure_sprt": {
        "alpha", "beta", "concurrency", "draw_centipawn_threshold", "draw_min_full_moves",
        "draw_mode", "draw_required_consecutive_moves", "elo0", "elo1", "max_games", "model",
        "openings_file", "openings_file_dialog", "pentanomial", "pgn_file", "pgn_file_dialog",
        "resign_centipawn_threshold", "resign_mode", "resign_required_consecutive_moves",
        "resign_two_sided", "time_control"},
    "select_epd_engines": {"engines"},
    "configure_epd": {"concurrency", "epd_file", "epd_file_dialog", "max_time_seconds",
                      "min_time_seconds", "seen_plies"},
    "get_running_status": set(),
    "start": {"type"},
    "stop": {"mode", "type"},
    "get_status": {"type"},
    "clear_result": {"type"},
    "show_result": {"type"},
    "close_application": set(),
    "open_pgn_file": {"source"},
    "reply_to_user": {"text"},
}

# Tools whose result ends the turn (GuiToolResult::terminal). open_add_engine_dialog is
# always terminal; the configure_* dialogs only when a file was actually picked.
ALWAYS_TERMINAL = {"open_add_engine_dialog"}
DIALOG_FLAGS = {"openings_file_dialog", "pgn_file_dialog", "epd_file_dialog"}

# Phrases the system prompt or a tool result explicitly rules out of a reply.
FORBIDDEN = [
    (re.compile(r"[Bb]itte wählen Sie|bitte wähle .*aus dem"), "Auswahl-Aufforderung"),
    (re.compile(r"get_status|show_result|configure_\w+|get_running_status|open_pgn_file|"
                r"select_\w+|clear_result|<tool_call|<function"), "Tool-Name im Antworttext"),
    (re.compile(r"[Ѐ-ӿ]"), "kyrillische Zeichen"),
    (re.compile(r"[一-鿿぀-ヿ]"), "CJK-Zeichen"),
]

NUMBER = re.compile(r"\d+(?:[.,]\d+)*")
PATH = re.compile(r"/[\w./-]+")


def tool_calls(message):
    return message.get("tool_calls") or []


def check(record, index):
    problems = []
    messages = record["messages"]

    if messages[0]["role"] != "system":
        problems.append("erste Message ist kein system prompt")

    # --- structure -------------------------------------------------------------
    reply_text = None
    grounding = []          # everything the model legitimately saw this turn
    seen_terminal = False
    for position, message in enumerate(messages):
        role = message.get("role")
        calls = tool_calls(message)

        if role in ("user", "system"):
            grounding.append(message.get("content") or "")
        if role == "assistant" and not calls:
            grounding.append(message.get("content") or "")   # flattened history
        if role == "tool":
            grounding.append(message.get("content") or "")

        if calls:
            if role != "assistant":
                problems.append(f"tool_calls an einer {role}-Message")
            if (message.get("content") or "").strip():
                problems.append("Klartext neben tool_calls")
            if len(calls) != 1:
                problems.append(f"{len(calls)} tool_calls in einer Message")
            for call in calls:
                name = call["function"]["name"]
                if name not in SCHEMA:
                    problems.append(f"unbekanntes Tool: {name}")
                    continue
                try:
                    args = json.loads(call["function"].get("arguments") or "{}")
                except json.JSONDecodeError:
                    problems.append(f"{name}: Argumente sind kein gültiges JSON")
                    continue
                unknown = set(args) - SCHEMA[name]
                if unknown:
                    problems.append(f"{name}: unbekannte Argumente {sorted(unknown)}")
                if seen_terminal:
                    problems.append(f"{name} nach einem terminalen Tool")
                if name == "reply_to_user":
                    reply_text = args.get("text", "")
                    if position != len(messages) - 1:
                        problems.append("reply_to_user ist nicht die letzte Message")
                else:
                    if position + 1 >= len(messages) or messages[position + 1]["role"] != "tool":
                        problems.append(f"{name}: kein tool-Ergebnis danach")
                    # dialogOpened is set unconditionally the moment a *_file_dialog flag is
                    # honoured (gui-tool-tournament-register.cpp:465/472 and the sprt/epd
                    # equivalents) -- a cancelled dialog is just as terminal as a successful
                    # one. Do NOT exempt the cancel case.
                    if (name in ALWAYS_TERMINAL or DIALOG_FLAGS & set(args)
                            or (name == "open_pgn_file"
                                and args.get("source", "dialog") == "dialog")):
                        seen_terminal = True

    last = messages[-1]
    if tool_calls(last):
        if tool_calls(last)[0]["function"]["name"] != "reply_to_user":
            problems.append("Turn endet auf einem Tool-Call statt auf reply_to_user")
    elif last["role"] != "tool":
        problems.append("Turn endet weder auf reply_to_user noch auf einem tool-Ergebnis")
    elif not seen_terminal:
        problems.append("Turn endet ohne Antwort, obwohl kein terminales Tool beteiligt war")

    # --- reply content ---------------------------------------------------------
    if reply_text is not None:
        if not reply_text.strip():
            problems.append("leere Antwort")
        for pattern, label in FORBIDDEN:
            if pattern.search(reply_text):
                problems.append(f"verbotene Formulierung: {label}")

        # Dialog-Narration is only a defect where a dialog was actually invoked -- a reply
        # that merely *explains* why announcing an open dialog is wrong (the meta turns) is
        # not narrating one. Scoping this to real dialog calls keeps the rule honest instead
        # of relying on a hand-waved false-positive exception.
        invoked_dialog = any(
            DIALOG_FLAGS & set(json.loads(call["function"].get("arguments") or "{}"))
            or call["function"]["name"] in ALWAYS_TERMINAL
            for message in messages for call in tool_calls(message))
        if invoked_dialog and re.search(r"[Dd]ialog\s+(wurde|wird|ist)\b.*geöffnet", reply_text):
            problems.append("verbotene Formulierung: Dialog-Narration")

        haystack = "\n".join(grounding)
        for number in set(NUMBER.findall(reply_text)):
            if number not in haystack:
                problems.append(f"ungedeckte Zahl in der Antwort: {number}")
        for path in set(PATH.findall(reply_text)):
            if path not in haystack:
                problems.append(f"ungedeckter Pfad in der Antwort: {path}")

    return problems


def main():
    records = [json.loads(line) for line in DATA.open() if line.strip()]
    prompts = {r["messages"][0]["content"] for r in records}
    print(f"{len(records)} Records, {len(prompts)} verschiedene System-Prompts")
    if len(prompts) != 1:
        print("FEHLER: der System-Prompt ist nicht in allen Records identisch")

    total = 0
    for index, record in enumerate(records):
        problems = check(record, index)
        if problems:
            total += len(problems)
            user = next((m["content"] for m in record["messages"] if m["role"] == "user"), "")
            print(f"\n#{index}  {user[:60]}")
            for problem in problems:
                print(f"    - {problem}")

    print(f"\n{'=' * 60}\n{total} Beanstandungen")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
