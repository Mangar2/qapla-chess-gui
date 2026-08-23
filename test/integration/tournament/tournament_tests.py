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
                {"state": True, "id": "state"},
            ],
            "validators": [
                {"type": "content", "step": "config", "pattern": "Engines: Diag A, Diag B"},
                {"type": "content", "step": "config", "pattern": "Games per pairing: 2"},
                {"type": "waitReason", "step": "run", "expected": "finished"},
                # The standings as data rather than as a regular expression over a sentence:
                # two engines, two rows, two games each, all accounted for.
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "running", "expected": False},
                {"type": "resultRows", "step": "state", "activity": "tournament", "count": 2},
                {"type": "resultCell", "step": "state", "activity": "tournament",
                 "row": 0, "column": "Total", "expected": "2"},
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
                {"type": "waitReason", "step": "run",
                 "expected": ["stopped", "not_running"]},
            ],
        },
        {
            "name": "tournament-concurrency-survives-a-stop",
            "description": "An abrupt stop leaves the configured concurrency alone",
            "engines": PAIR,
            "steps": [
                # A regression test with a bug behind it: concurrency used to fall back to the
                # clamp floor of 1 when a run was stopped, throwing away what had been set.
                {"call": "configure_tournament",
                 "args": _basic_configuration(games=100, time_control="2+0.1", concurrency=5),
                 "id": "config"},
                {"call": "start", "args": {"type": "tournament"}},
                {"call": "stop", "args": {"type": "tournament", "mode": "abrupt"}, "id": "stop"},
                {"wait": "tournament", "timeout": 120},
                {"call": "get_status", "args": {"type": "tournament"}, "id": "after"},
            ],
            "validators": [
                {"type": "content", "step": "config", "pattern": "Concurrency: 5"},
                {"type": "ok", "step": "stop"},
                {"type": "content", "step": "after", "pattern": "Concurrency: 5"},
            ],
        },
        {
            "name": "tournament-adjudication-settings",
            "description": "The draw and resign adjudication fields arrive and are reported back",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament",
                 "args": {"draw_mode": "active", "draw_min_full_moves": 60,
                          "draw_required_consecutive_moves": 10,
                          "draw_centipawn_threshold": 15},
                 "id": "draw"},
                {"call": "configure_tournament",
                 "args": {"resign_mode": "test", "resign_required_consecutive_moves": 8,
                          "resign_centipawn_threshold": 600, "resign_two_sided": True},
                 "id": "resign"},
                {"restart": True},
                {"call": "get_status", "args": {"type": "tournament"}, "id": "after_restart"},
            ],
            "validators": [
                {"type": "content", "step": "draw",
                 "pattern": "min full moves=60, required consecutive moves=10, "
                            "centipawn threshold=15"},
                {"type": "content", "step": "resign",
                 "pattern": "required consecutive moves=8, centipawn threshold=600, "
                            "two-sided=yes"},
                # Adjudication is a stored setting like any other, so it has to survive a restart.
                {"type": "content", "step": "after_restart", "pattern": "min full moves=60"},
                {"type": "content", "step": "after_restart", "pattern": "two-sided=yes"},
            ],
        },
        {
            "name": "tournament-closes-while-a-run-is-going",
            "description": "The application shuts down properly with a tournament still playing",
            "engines": PAIR,
            "steps": [
                # No stop, on purpose: the session's own shutdown has to cope with games in
                # flight, which is what happens whenever somebody closes the window mid-run.
                # Measured at around twenty seconds on this machine -- slow, but an end.
                {"call": "configure_tournament",
                 "args": _basic_configuration(games=100, time_control="2+0.1")},
                {"call": "start", "args": {"type": "tournament"}},
                {"state": True, "id": "state"},
            ],
            "validators": [
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "running", "expected": True},
                # The verdict on the shutdown itself is the framework's: a session that had to be
                # killed, or that ended with a non-zero code, fails the test it belonged to.
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
                {"state": True, "id": "state"},
            ],
            "validators": [
                {"type": "ok", "step": "clear"},
                # Gone means gone: no result table at all, not a sentence saying there is none.
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "results", "expected": None},
            ],
        },
    ]
