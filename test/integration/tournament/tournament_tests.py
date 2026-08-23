#!/usr/bin/env python3
"""Tournament tests -- configure, start, watch, read the result, stop.

The runs are deliberately tiny (two engines, two games, a fifth of a second each). What is being
tested is the path through the application, not chess: that a configuration set over HTTP is the
one that gets played, that the run reaches its end, that the standings and the PGN come out, and
that a run can be stopped while it is going.
"""

import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as ec  # noqa: E402

PAIR = [ec.DIAG, ec.DIAG_B]

#: Fast enough that a whole tournament fits in a second, slow enough not to be a time forfeit.
FAST_TC = "0.2+0.01"


def _basic_configuration(**overrides: Any) -> Dict[str, Any]:
    configuration = {
        "engines": PAIR,
        "games": 2,
        "time_control": FAST_TC,
        "openings_file": "{openings}",
        "pgn_file": "{pgn}",
    }
    configuration.update(overrides)
    return configuration


def _plays_the_same_game_twice(session, results):
    """The seeded diagnostic engine is what makes a result worth asserting at all.

    Without QAPLA_DIAG_SEED the moves are random, and a test could only ever check that something
    happened. With it, the same tournament has to produce the same standings -- so this replays it
    in the same session and compares.
    """
    first = str(results["run"].get("content", ""))
    session.call("clear_result", {"type": "tournament"})
    session.call("start", {"type": "tournament"})
    second = str(session.wait("tournament", 120).get("content", ""))

    def standings(text: str) -> str:
        marker = "Name"
        return text[text.find(marker):] if marker in text else text

    if standings(first) == standings(second):
        return True, "the same seed plays the same tournament twice"
    return False, f"the standings differ between two runs of the same seed:\n{standings(first)}\n---\n{standings(second)}"


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "tournament-basic",
            "description": "Two engines play a two-game tournament to the end",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament", "args": _basic_configuration(), "id": "config"},
                {"call": "start", "args": {"type": "tournament"}, "id": "start"},
                {"wait": "tournament", "timeout": 120, "id": "run"},
            ],
            "validators": [
                {"type": "content", "step": "config", "pattern": "Engines: Diag A, Diag B"},
                {"type": "content", "step": "config", "pattern": "Games per pairing: 2"},
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "content", "step": "run", "pattern": r"Diag A\s+\S+\s+\S*\s*\S*/2",
                 "isRegex": True},
                {"type": "fileExists", "path": "games.pgn"},
                {"type": "fileContent", "path": "games.pgn", "content": "[White "},
            ],
        },
        {
            "name": "tournament-needs-openings",
            "description": "A tournament without an openings file is refused, with the reason",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament",
                 "args": {"engines": PAIR, "games": 2, "time_control": FAST_TC}},
                {"call": "start", "args": {"type": "tournament"}, "expect_ok": False,
                 "id": "start"},
            ],
            "validators": [
                {"type": "failed", "step": "start"},
                {"type": "content", "step": "start", "pattern": "openings"},
            ],
        },
        {
            "name": "tournament-reports-the-standings",
            "description": "The finished run answers with the standings table, not just a verdict",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament", "args": _basic_configuration()},
                {"call": "start", "args": {"type": "tournament"}},
                {"wait": "tournament", "timeout": 120, "id": "run"},
                {"call": "get_status", "args": {"type": "tournament"}, "id": "status"},
            ],
            "validators": [
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "content", "step": "status", "pattern": "Elo"},
                {"type": "content", "step": "status", "pattern": "Diag A"},
                {"type": "content", "step": "status", "pattern": "Diag B"},
            ],
        },
        {
            "name": "tournament-is-reproducible",
            "description": "With a fixed engine seed, the same tournament produces the same table",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament", "args": _basic_configuration()},
                {"call": "start", "args": {"type": "tournament"}},
                {"wait": "tournament", "timeout": 120, "id": "run"},
            ],
            "validators": [
                {"type": "custom", "check": _plays_the_same_game_twice},
            ],
        },
        {
            "name": "tournament-stop-while-running",
            "description": "A long tournament can be stopped, and says so",
            "engines": PAIR,
            "steps": [
                # Long enough that it is certainly still playing when stop arrives.
                {"call": "configure_tournament",
                 "args": _basic_configuration(games=100, time_control="2+0.1")},
                {"call": "start", "args": {"type": "tournament"}},
                {"call": "stop", "args": {"type": "tournament"}, "id": "stop"},
                {"wait": "tournament", "timeout": 120, "id": "run"},
            ],
            "validators": [
                {"type": "ok", "step": "stop"},
                {"type": "waitReason", "step": "run", "expected": "stopped"},
            ],
        },
        {
            "name": "tournament-clear-result",
            "description": "Clearing takes the finished run's results away",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament", "args": _basic_configuration()},
                {"call": "start", "args": {"type": "tournament"}},
                {"wait": "tournament", "timeout": 120},
                {"call": "clear_result", "args": {"type": "tournament"}, "id": "clear"},
                {"call": "get_status", "args": {"type": "tournament"}, "id": "status"},
            ],
            "validators": [
                {"type": "ok", "step": "clear"},
                {"type": "content", "step": "status", "pattern": "No tournament results"},
            ],
        },
    ]
