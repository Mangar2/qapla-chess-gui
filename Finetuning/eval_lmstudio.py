"""Scores a model served by LM Studio against a split, one turn deep.

Runs the same request the app makes at the start of a turn -- everything up to and including
the user's message, plus the tool schemas, tool_choice=required -- and compares what comes
back against what the dataset says should have happened.

Two numbers, in the order they matter:

  acted    the model returned a tool call at all, rather than plain prose. This is the
           failure the dataset exists to fix ("behaupten statt handeln"): the app can only
           ever show the user a reply_to_user call, so prose is not a worse answer, it is
           no answer. Note LM Studio reports these with finish_reason=stop and an empty
           tool_calls list even under tool_choice=required.
  correct  ...and it was the tool the dataset called first.

Only the FIRST step is judged. Scoring the whole chain would need the real tools to run to
produce the next tool result, and a mismatch at step one makes everything after it
incomparable anyway.

Usage:
    python3 eval_lmstudio.py                      # test split, model as loaded
    python3 eval_lmstudio.py --split valid
    python3 eval_lmstudio.py --out baseline.json  # keep results for a before/after diff
"""
import argparse
import collections
import json
import pathlib
import urllib.request

HERE = pathlib.Path(__file__).parent
ENDPOINT = "http://localhost:1234/v1"


def post(path, body, timeout=600):
    request = urllib.request.Request(
        f"{ENDPOINT}{path}", data=json.dumps(body).encode("utf-8"),
        headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return json.load(response)


def turn_start(messages):
    """Everything up to and including the last user message -- the app's state at turn start."""
    last_user = max(i for i, m in enumerate(messages) if m["role"] == "user")
    return messages[:last_user + 1]


def expected_first_tool(messages):
    for message in messages:
        for call in message.get("tool_calls") or []:
            return call["function"]["name"]
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--split", default="test")
    parser.add_argument("--model", default=None, help="defaults to whatever LM Studio has loaded")
    parser.add_argument("--out", default=None)
    args = parser.parse_args()

    model = args.model or post("/models", None) if False else args.model
    if not model:
        with urllib.request.urlopen(f"{ENDPOINT}/models", timeout=30) as response:
            model = json.load(response)["data"][0]["id"]

    lines = [json.loads(l) for l in (HERE / "data" / f"{args.split}.jsonl").open(encoding="utf-8")
             if l.strip()]
    print(f"model: {model}\nsplit: {args.split} ({len(lines)} records)\n")

    results = []
    acted = correct = 0
    confusion = collections.Counter()
    for i, record in enumerate(lines, 1):
        want = expected_first_tool(record["messages"])
        reply = post("/chat/completions", {
            "model": model,
            "messages": turn_start(record["messages"]),
            "tools": record["tools"],
            "tool_choice": "required",
            "temperature": 0,
            "max_tokens": 400,
        })["choices"][0]["message"]

        calls = reply.get("tool_calls") or []
        got = calls[0]["function"]["name"] if calls else None
        acted += bool(got)
        correct += got == want
        confusion[(want, got)] += 1
        results.append({"want": want, "got": got,
                        "content": (reply.get("content") or "")[:300]})
        print(f"  [{i:3}/{len(lines)}] want {want or '-':24} got {got or 'PROSE':24}"
              f" {'ok' if got == want else 'x'}")

    n = len(lines)
    print(f"\nacted (any tool call): {acted}/{n} = {acted/n:.1%}")
    print(f"correct first tool:    {correct}/{n} = {correct/n:.1%}")
    print("\nmost common misses:")
    for (want, got), count in confusion.most_common():
        if want != got:
            print(f"  {want or '-':24} -> {got or 'PROSE':24} x{count}")

    if args.out:
        (HERE / args.out).write_text(json.dumps(
            {"model": model, "split": args.split, "acted": acted, "correct": correct,
             "n": n, "results": results}, indent=2, ensure_ascii=False), encoding="utf-8")
        print(f"\nwrote {args.out}")


if __name__ == "__main__":
    main()
