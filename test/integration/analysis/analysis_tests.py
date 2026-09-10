#!/usr/bin/env python3
"""Backward analysis tests -- recomputing games that were played, rather than playing them.

The input is the two finished games the framework writes into every sandbox ({played}). Finished
and short on purpose: the analysis walks every position of every game backwards from the last
move, so the length of the run is the number of half moves times the time per position, and
nothing about the engine's speed. Eleven positions at a fifth of a second each is a run that
ends by itself inside a test, and still long enough to be caught while it is going.

What the assertions are about is what comes out: the games are written again, with the players
they were played by, and carrying the evaluation of every position that was recomputed.
"""

import re
import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as ec  # noqa: E402


def _every_move_was_evaluated(session, _results):
    """Each half move of each game comes back with a score, a depth and a time.

    Written as ``{+0.21/20 0.18s (...)}`` behind the move. Checking the comments rather than only
    that the file exists is the whole point of the run: a file with the games in it and nothing
    said about them means the analysis wrote what it read.
    """
    output = Path(session.config_dir) / "analysed.pgn"
    if not output.is_file():
        return False, "the analysis wrote no output file"
    text = output.read_text(encoding="utf-8", errors="replace")
    evaluated = re.findall(r"\{[-+M]?[\d.]+/\d+ [\d.]+s", text)
    if len(evaluated) < 11:
        return False, f"only {len(evaluated)} of the 11 half moves carry an evaluation"
    if "White Player" not in text or "Other Black" not in text:
        return False, "the analysed games do not name the players who played them"
    return True, f"all {len(evaluated)} half moves carry an evaluation, players kept"


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "analysis-recomputes-every-position",
            "description": "Two played games are analysed backwards and written out with scores",
            "engines": [ec.QAPLA],
            "steps": [
                {"call": "configure_backward_analysis",
                 "args": {"engines": [ec.QAPLA], "pgn_file": "{played}", "move_time_ms": 200,
                          "output_file": "{sandbox}/analysed.pgn", "append_output": False,
                          "concurrency": 2},
                 "id": "config"},
                {"call": "start", "args": {"type": "analysis"}, "id": "start"},
                {"wait": "analysis", "timeout": 180, "id": "run"},
                {"call": "get_status", "args": {"type": "analysis"}, "id": "after"},
            ],
            "validators": [
                {"type": "content", "step": "config", "pattern": "played.pgn"},
                {"type": "content", "step": "config", "pattern": "200 ms"},
                {"type": "ok", "step": "start"},
                {"type": "content", "step": "start", "pattern": "2 games"},
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "content", "step": "after", "pattern": "2 of 2 games analysed"},
                {"type": "fileExists", "path": "analysed.pgn"},
                {"type": "fileContent", "path": "analysed.pgn", "content": "Annotator"},
                {"type": "custom", "check": _every_move_was_evaluated},
            ],
        },
        {
            "name": "analysis-needs-a-pgn-file",
            "description": "Starting without games to read is refused, with the reason",
            "engines": [ec.DIAG],
            "steps": [
                {"call": "configure_backward_analysis",
                 "args": {"engines": [ec.DIAG], "output_file": "{sandbox}/analysed.pgn"}},
                {"call": "start", "args": {"type": "analysis"}, "expect_ok": False, "id": "start"},
            ],
            "validators": [
                {"type": "failed", "step": "start"},
                {"type": "content", "step": "start", "pattern": "PGN"},
                {"type": "fileMissing", "path": "analysed.pgn"},
            ],
        },
        {
            "name": "analysis-takes-a-new-concurrency-while-it-runs",
            "description": "Concurrency changes during a run; the rest of the settings do not",
            "engines": [ec.QAPLA],
            "steps": [
                {"call": "configure_backward_analysis",
                 "args": {"engines": [ec.QAPLA], "pgn_file": "{played}", "move_time_ms": 4000,
                          "output_file": "{sandbox}/analysed.pgn", "concurrency": 1},
                 "id": "config"},
                {"call": "start", "args": {"type": "analysis"}, "id": "start"},
                # While it runs: the number of games at a time may change, what is being computed
                # may not.
                {"call": "configure_backward_analysis", "args": {"concurrency": 2}, "id": "faster"},
                {"call": "configure_backward_analysis", "args": {"move_time_ms": 10},
                 "expect_ok": False, "id": "blocked"},
                {"call": "stop", "args": {"type": "analysis", "mode": "abrupt"}, "id": "stop"},
                {"wait": "analysis", "timeout": 120},
                {"call": "get_status", "args": {"type": "analysis"}, "id": "after"},
            ],
            "validators": [
                {"type": "ok", "step": "start"},
                {"type": "ok", "step": "faster"},
                {"type": "content", "step": "faster", "pattern": "Concurrency: 2"},
                {"type": "failed", "step": "blocked"},
                {"type": "ok", "step": "stop"},
                # The refused change left the run alone: still the time it was started with.
                {"type": "content", "step": "after", "pattern": "4000 ms"},
                {"type": "content", "step": "after", "pattern": "Concurrency: 2"},
            ],
        },
        {
            "name": "analysis-shows-up-in-the-state",
            "description": "/state reports the analysis beside the other activities",
            "engines": [ec.QAPLA],
            "steps": [
                {"call": "configure_backward_analysis",
                 "args": {"engines": [ec.QAPLA], "pgn_file": "{played}", "move_time_ms": 200,
                          "output_file": "{sandbox}/analysed.pgn"}},
                {"state": True, "id": "idle"},
                {"call": "start", "args": {"type": "analysis"}},
                {"wait": "analysis", "timeout": 180},
                {"state": True, "id": "done"},
            ],
            "validators": [
                {"type": "stateField", "step": "idle", "activity": "analysis",
                 "field": "state", "expected": "idle"},
                {"type": "stateField", "step": "done", "activity": "analysis",
                 "field": "state", "expected": "idle"},
            ],
        },
    ]
