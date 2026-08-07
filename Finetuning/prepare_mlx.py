"""Turns the dataset into what mlx_lm.lora expects: data/{train,valid,test}.jsonl.

Three things happen here that are not cosmetic:

0. Tool call "arguments" are parsed from their JSON string into a real object. The dataset
   stores them the way the OpenAI wire format does ("{\"concurrency\":10}"); the LFM2.5
   template refuses that outright ("Tool call arguments must be a mapping, got a
   JSON-encoded string"). Note this is a rendering concern only -- the dataset itself stays
   in the wire format it shares with the app.

1. Every line gets a "tools" field. The dataset only stores "messages", but the app also
   sends the tool schemas on the wire (buildToolsJson() in lm-studio-client.cpp) and the
   LFM2.5 template appends them to the system turn as "List of tools: [...]". Training
   without them would teach a prompt the model never sees at inference time. mlx-lm reads
   a per-line "tools" key and passes it to apply_chat_template (tuner/datasets.py:59).

2. The split runs over BASE records, not lines. build.py emits every base record once per
   world (build.py:135-137), so base record i sits on lines i, i+n, i+2n. Splitting lines
   at random would put world A of a record in train and world B in valid -- the validation
   loss would then measure memorisation and look great while meaning nothing.

Records using a tool that barely occurs are pinned to train: with a single example in the
whole set, holding it out costs the model the tool entirely and still measures nothing.

Usage:
    python3 prepare_mlx.py                 # writes ./data/
    python3 prepare_mlx.py --stats-only    # report only, writes nothing
"""
import argparse
import collections
import json
import pathlib
import random

from variants import WORLDS

HERE = pathlib.Path(__file__).parent
DATASET = HERE / "finetuning-001.json"
TOOLS = HERE / "tools.json"
OUT = HERE / "data"

# Held out per split, counted in BASE records -- each one carries its worlds along, so the
# written line count is this times the number of worlds.
VALID_BASE = 12
TEST_BASE = 12

# A tool appearing at most this often across all base records is treated as rare and its
# records stay in train.
RARE_AT_MOST = 2

SEED = 20260807


def parse_arguments(record):
    """tool_calls[].function.arguments: JSON string -> object, in a copy of the record.

    Only the chat template cares; the dataset keeps the wire format so it stays comparable
    to what the app sends and to what LlmFineTuningWriter appends.
    """
    record = json.loads(json.dumps(record))
    for message in record["messages"]:
        for call in message.get("tool_calls") or []:
            arguments = call["function"]["arguments"]
            if isinstance(arguments, str):
                call["function"]["arguments"] = json.loads(arguments)
    return record


def tool_chain(record):
    """Every tool name called anywhere in the record."""
    return [
        call["function"]["name"]
        for message in record["messages"]
        for call in (message.get("tool_calls") or [])
    ]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--stats-only", action="store_true")
    args = parser.parse_args()

    lines = [json.loads(line) for line in DATASET.open(encoding="utf-8") if line.strip()]
    tools = json.loads(TOOLS.read_text(encoding="utf-8"))

    worlds = len(WORLDS) + 1  # build.py emits the originals plus one copy per world
    if len(lines) % worlds:
        raise SystemExit(f"{len(lines)} lines is not divisible by {worlds} worlds -- "
                         "build.py's layout changed, the split logic below would be wrong")
    base_count = len(lines) // worlds

    # Sanity check on the assumed layout: a base record and its worlds must call the same
    # tools in the same order. If they don't, lines are not laid out as assumed.
    for i in range(base_count):
        chains = {tuple(tool_chain(lines[i + w * base_count])) for w in range(worlds)}
        if len(chains) != 1:
            raise SystemExit(f"base record {i}: worlds disagree on the tool chain {chains} "
                             "-- the block layout assumed here does not hold")

    usage = collections.Counter()
    for i in range(base_count):
        usage.update(set(tool_chain(lines[i])))
    rare = {name for name, n in usage.items() if n <= RARE_AT_MOST}

    pinned = {i for i in range(base_count) if rare & set(tool_chain(lines[i]))}
    free = [i for i in range(base_count) if i not in pinned]
    random.Random(SEED).shuffle(free)

    holdout = VALID_BASE + TEST_BASE
    if len(free) < holdout:
        raise SystemExit(f"only {len(free)} base records are free to hold out, need {holdout}")
    valid_base = sorted(free[:VALID_BASE])
    test_base = sorted(free[VALID_BASE:holdout])
    train_base = sorted(set(range(base_count)) - set(valid_base) - set(test_base))

    print(f"{len(lines)} lines = {base_count} base records x {worlds} worlds")
    print(f"rare tools (<={RARE_AT_MOST} base records, pinned to train): "
          + (", ".join(f"{n}({usage[n]})" for n in sorted(rare)) or "none"))
    print(f"pinned: {len(pinned)} base records")
    print(f"split (base): train {len(train_base)}, valid {len(valid_base)}, test {len(test_base)}")

    if args.stats_only:
        return

    OUT.mkdir(exist_ok=True)
    for name, base_indices in (("train", train_base), ("valid", valid_base), ("test", test_base)):
        path = OUT / f"{name}.jsonl"
        with path.open("w", encoding="utf-8") as handle:
            written = 0
            for i in base_indices:
                for w in range(worlds):
                    record = parse_arguments(lines[i + w * base_count])
                    record["tools"] = tools
                    handle.write(json.dumps(record, ensure_ascii=False) + "\n")
                    written += 1
        print(f"wrote {written:4} lines -> {path.relative_to(HERE)}")

    # The test split is also the eval set for before/after comparison, and for that the base
    # record indices need to survive -- they map back to review-ledger-001.md.
    (OUT / "test-base-indices.json").write_text(
        json.dumps({"base_indices": test_base, "worlds": worlds, "base_count": base_count},
                   indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
