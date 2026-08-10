"""Checks tools.json against the C++ tool definitions it was captured from.

tools.json is a capture of the "tools" array the app sends to LM Studio -- lifted from the
chat log, so it is genuinely what went over the wire. That is also its weakness: a capture
ages the moment a description changes in the C++, and a stale capture is invisible. It cost
us one already (open_add_engine_dialog still carried the pre-.terminal wording).

The check is deliberately crude: pull every string literal out of src/llm, concatenate, strip
whitespace, and ask whether each description from tools.json occurs in there. Crude is the
point -- it needs no knowledge of how the literals are split across lines or which of them
belong to which ToolSpec, so refactoring the C++ does not break it.

Whitespace-insensitive because the C++ wraps descriptions across concatenated literals at
arbitrary points; that is a formatting difference, not a content difference.

Usage:
    python3 check_tools.py     # exit 0 = tools.json matches the code
"""
import argparse
import json
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parents[2]
# Rekursiv durchsuchen: die Werkzeugdefinitionen liegen in Unterverzeichnissen
# (src/llm/tools/, actions/, api/). Ein flacher Scan findet sie nicht und meldet
# dann jedes Werkzeug als "nicht gefunden" -- also eine Abweichung, die es nicht gibt.


def literal_blob(path):
    """Every string literal in the file, concatenated. Handles raw strings and comments."""
    src = path.read_text(encoding="utf-8")
    out = []
    # Raw strings first and remove them from the source -- their contents are full of quotes
    # that would otherwise be parsed as literal delimiters (this is what a first version of
    # this check got wrong on reply_to_user's schema).
    def take_raw(match):
        out.append(match.group("body"))
        return " "
    src = re.sub(r'R"(?P<d>[^(]*)\((?P<body>.*?)\)(?P=d)"', take_raw, src, flags=re.S)
    src = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    src = re.sub(r"//[^\n]*", " ", src)
    for lit in re.findall(r'"((?:[^"\\]|\\.)*)"', src):
        try:
            out.append(lit.encode().decode("unicode_escape"))
        except Exception:
            out.append(lit)
    return "".join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tools", type=pathlib.Path, default=REPO / "data/qapla-chatbot/tools.json",
                        help="die zu pruefende Datei mit den Werkzeugbeschreibungen")
    parser.add_argument("--sources", type=pathlib.Path, default=REPO / "src/llm",
                        help="Verzeichnis mit den C++-Quellen, rekursiv durchsucht")
    args = parser.parse_args()
    sources = sorted(args.sources.rglob("*.cpp")) + sorted(args.sources.rglob("*.h"))
    if not sources:
        raise SystemExit(f"keine Quellen unter {args.sources}")
    blob = re.sub(r"\s+", "", "".join(literal_blob(p) for p in sources))
    squash = lambda s: re.sub(r"\s+", "", s)

    tools = json.loads(args.tools.read_text(encoding="utf-8"))
    problems = []
    for entry in tools:
        function = entry["function"]
        name = function["name"]
        # blob holds literal *contents*, so the surrounding quotes are already gone
        if name not in blob:
            problems.append(f"{name}: tool name not found in src/llm")
        if squash(function.get("description", "")) not in blob:
            problems.append(f"{name}: description differs from the C++ source")
        for arg, spec in (function.get("parameters") or {}).get("properties", {}).items():
            described = spec.get("description", "")
            if described and squash(described) not in blob:
                problems.append(f"{name}.{arg}: argument description differs")

    print(f"{len(tools)} tools checked against {len(sources)} C++ sources")
    for problem in problems:
        print("  ", problem)
    if problems:
        print(f"\n{len(problems)} mismatch(es) -- tools.json is stale, re-capture it from the "
              "chat log after running the current build")
        return 1
    print("tools.json matches the code")
    return 0


if __name__ == "__main__":
    sys.exit(main())
