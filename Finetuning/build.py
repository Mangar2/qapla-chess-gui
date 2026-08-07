"""Assembles the corrected finetuning dataset from the per-chunk record modules.

Authoring rules (decided with Volker, 2026-08-07):
  * reply language mirrors the user's language
  * follow-up turns get the preceding turn prepended as flattened user/assistant
    text -- exactly the shape LlmChatController builds at runtime (see
    llm-chat-controller.cpp:346-348). Context may contain the OLD, wrong reply:
    it is input, never a training target.
  * tool results use today's full-status format
  * every factual claim in a reply must be traceable to a tool result in the
    same record
"""
import json
import importlib
import pathlib

from variants import WORLDS, apply_world

HERE = pathlib.Path(__file__).parent
# original.json is the checked-in copy of ~/.qapla-chess-gui/finetuning.json as it stood
# when this cleanup started. Reading the prompt from it rather than from the live file
# keeps a rebuild reproducible and independent of any one machine.
SOURCE = HERE / "original.json"

# Durchnummeriert, weil weitere Datensätze folgen werden (002, 003, ...). Wer einen neuen
# aufsetzt, erhöht die Nummer hier und in verify.py -- der alte bleibt unverändert liegen.
DATASET = "finetuning-001.json"
LEDGER = "review-ledger-001.md"


def system_prompt():
    """The single canonical system prompt -- identical in all 149 source records."""
    prompts = set()
    with SOURCE.open() as handle:
        for line in handle:
            if line.strip():
                prompts.add(json.loads(line)["messages"][0]["content"])
    if len(prompts) != 1:
        raise SystemExit(f"expected 1 system prompt, found {len(prompts)}")
    return prompts.pop()


# Canonical activity phrases, verbatim from describe*Activity() in
# src/llm/gui-tool-status-register.cpp. buildRunningStatusText() lists ONLY what is
# active; start/stop append it to their own message (see handleStartOrStop).
T_STARTING = "a tournament is starting"
T_RUNNING = "a tournament is running"
T_GRACEFUL = "a tournament is running but stopping gracefully (finishing in-progress games)"
T_ABRUPT = "a tournament is stopping abruptly"
S_STARTING = "an SPRT test is starting"
S_RUNNING = "an SPRT test is running"
S_GRACEFUL = "an SPRT test is running but stopping gracefully (finishing its in-progress game)"
E_STARTING = "an EPD analysis is starting"
E_RUNNING = "an EPD analysis is running"
E_GRACEFUL = "an EPD analysis is running but stopping gracefully (finishing in-progress positions)"
E_ABRUPT = "an EPD analysis is stopping abruptly"


def running(*items):
    """buildRunningStatusText(): only active entries, nothing about what is idle."""
    if not items:
        return "Nothing is currently running -- no tournament, no SPRT test, no EPD analysis."
    return "Currently: " + "; ".join(items) + "."


def R(index, user, steps, reply, context=(), note=""):
    """One corrected record.

    index   -- original record index, kept for the review ledger
    user    -- the user message (verbatim from the source, never edited)
    steps   -- [(tool_name, args_dict, tool_result_text), ...] in order
    reply   -- the reply_to_user text, or None for a turn ending on a terminal tool
               (file dialogs, open_add_engine_dialog). GuiToolResult::terminal ends
               the turn without asking the model again, so no reply can exist.
    context -- [(user_text, assistant_text), ...] prepended as flattened history
    note    -- what was changed and why, for the review ledger
    """
    return {"index": index, "variant": "", "user": user, "steps": steps, "reply": reply,
            "context": list(context), "note": note}


def to_messages(record, prompt):
    messages = [{"role": "system", "content": prompt}]
    for user_text, assistant_text in record["context"]:
        messages.append({"role": "user", "content": user_text})
        messages.append({"role": "assistant", "content": assistant_text})
    messages.append({"role": "user", "content": record["user"]})

    for position, (name, args, result) in enumerate(record["steps"]):
        call_id = f"call_{record['index']}{record.get('variant', '')}_{position}"
        messages.append({
            "role": "assistant",
            "content": "",
            "tool_calls": [{
                "id": call_id,
                "type": "function",
                "function": {"name": name, "arguments": json.dumps(args, ensure_ascii=False)},
            }],
        })
        messages.append({"role": "tool", "tool_call_id": call_id, "content": result})

    if record["reply"] is None:
        # Terminal tool: the turn ends on the tool result, the model is never asked again.
        return messages

    reply_id = f"call_{record['index']}{record.get('variant', '')}_reply"
    messages.append({
        "role": "assistant",
        "content": "",
        "tool_calls": [{
            "id": reply_id,
            "type": "function",
            "function": {
                "name": "reply_to_user",
                "arguments": json.dumps({"text": record["reply"]}, ensure_ascii=False),
            },
        }],
    })
    return messages


def main():
    prompt = system_prompt()
    records = []
    # fixed_*.py = aus den Rohdaten korrigierte Records, added_*.py = neu geschriebene.
    # Getrennte Präfixe, damit die Herkunft im Verzeichnis sichtbar bleibt.
    for chunk in sorted(list(HERE.glob("fixed_*.py")) + list(HERE.glob("added_*.py"))):
        module = importlib.import_module(chunk.stem)
        records.extend(module.RECORDS)

    records.sort(key=lambda r: r["index"])

    # Jedes Record zusätzlich in den Welten aus variants.py, damit das Modell nicht die
    # immer gleichen Engine-Namen, Pfade und Schranken als Default lernt.
    emitted = list(records)
    for name, rules in WORLDS:
        emitted.extend(apply_world(record, name, rules) for record in records)

    out = HERE / DATASET
    with out.open("w") as handle:
        for record in emitted:
            handle.write(json.dumps({"messages": to_messages(record, prompt)},
                                    ensure_ascii=False) + "\n")

    # Das Ledger listet nur die Basis-Records: die Varianten sind mechanische Ableitungen
    # derselben Entscheidung, ihre Begründung wäre 1:1 dieselbe Zeile.
    ledger = HERE / LEDGER
    with ledger.open("w") as handle:
        handle.write(f"{len(records)} Basis-Records, je zusätzlich ausgegeben in den Welten: "
                     + ", ".join(name for name, _ in WORLDS)
                     + f" -- {len(emitted)} Records insgesamt.\n\n")
        handle.write("| # | User-Nachricht | Tools (korrigiert) | Änderung |\n")
        handle.write("|---|---|---|---|\n")
        for record in records:
            tools = " → ".join(name for name, _, _ in record["steps"]) or "—"
            user = record["user"].replace("|", "\\|")[:70]
            handle.write(f"| {record['index']} | {user} | {tools} | {record['note']} |\n")

    print(f"wrote {len(emitted)} records ({len(records)} base x {len(WORLDS) + 1}) -> {out}")
    print(f"wrote ledger -> {ledger}")


if __name__ == "__main__":
    main()
