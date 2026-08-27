#!/usr/bin/env python3
"""Tournament tests -- configure, start, watch, read the result, stop.

The runs are deliberately tiny (two engines, two games, a fifth of a second each). What is being
tested is the path through the application, not chess: that a configuration set over HTTP is the
one that gets played, that the run reaches its end, that the standings and the PGN come out, and
that a run can be stopped while it is going.
"""

import sys
import time
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
        # Sixteen, so that the default concurrency has sixteen games to actually run at once.
        "games": 16,
        "time_control": FAST_TC,
        "openings_file": "{openings}",
        "pgn_file": "{pgn}",
    }
    configuration.update(overrides)
    return configuration


def _survives_a_stop_straight_after_the_start(session, results):
    """Starts a tournament and stops it at once, over and over, watching the frame counter.

    This is the regression test for the freeze of 2026-08-25, and it is written the way the fault
    was found rather than the way a test is usually written. The fault was a stop that reached the
    *next* run: the pool answered "nothing left to do" while the stop it had just been given was
    still in a manager's queue, and the run started a moment later was torn down by it. Whoever
    waited for that run waited for ever -- and that waiting happens on the UI thread, so the whole
    application stopped answering.

    What makes it visible is the frame counter, not the call: a frozen UI thread still answers
    /health, and a tool call that never returns is exactly the symptom. Both are checked.

    Ten rounds because the fault was never certain: it showed at the second, third, fifth and
    eleventh attempt on the machine it was found on. Ten is far from proof and cheap enough to
    run every time -- see docs/bugs/gui-freeze-2026-08-25.md for what was actually established.
    """
    # The placeholders of a step are substituted by the framework; a validator that calls the
    # application itself has to name the paths, and the sandbox is where they are.
    sandbox = session.config_dir
    configuration = _basic_configuration(
        games=100, time_control="2+0.1",
        openings_file=str(sandbox / "openings.pgn"), pgn_file=str(sandbox / "games.pgn"))

    rounds = 10
    for round_number in range(1, rounds + 1):
        session.call("configure_tournament", configuration)
        session.call("start", {"type": "tournament"})

        deadline = time.time() + 30
        while time.time() < deadline:
            if session.state()["activities"]["tournament"]["running"]:
                break
            time.sleep(0.05)
        else:
            return False, f"round {round_number}: the tournament never reported itself running"

        before = session.remote.health()["frames"]["count"]
        try:
            session.call("stop", {"type": "tournament", "mode": "abrupt"}, timeout=30)
        except Exception as error:  # noqa: BLE001 -- a call that never returns is the finding
            return False, f"round {round_number}: the stop never came back ({error})"

        # A frame within two seconds, not within the instant the stop returned: the loop draws
        # roughly forty a second when it is healthy, so two seconds is a hundred-fold margin --
        # and the freeze this test was written for never ends on its own, at any margin.
        deadline = time.time() + 2
        while True:
            frames = session.remote.health()["frames"]
            if frames["count"] != before:
                break
            if time.time() >= deadline:
                return False, (f"round {round_number}: the stop returned but the UI thread drew "
                               f"no further frame for two seconds (stuck "
                               f"{frames['current_frame_ms']:.0f} ms in "
                               f"{frames['current_section']!r})")
            time.sleep(0.05)
        session.call("clear_result", {"type": "tournament"})

    return True, f"{rounds} stops straight after the start, and the frame loop kept running"


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
                {"type": "content", "step": "config", "pattern": "Games per pairing: 16"},
                {"type": "waitReason", "step": "run", "expected": "finished"},
                # The standings as data rather than as a regular expression over a sentence:
                # two engines, two rows, two games each, all accounted for.
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "running", "expected": False},
                {"type": "resultRows", "step": "state", "activity": "tournament", "count": 2},
                {"type": "resultCell", "step": "state", "activity": "tournament",
                 "row": 0, "column": "Total", "expected": "16"},
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
                 "args": {"engines": PAIR, "games": 16, "time_control": FAST_TC}},
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
            "name": "tournament-stop-right-after-the-start",
            "description": "Stopping a tournament the moment it starts does not freeze the GUI",
            "engines": PAIR,
            "steps": [],
            "validators": [
                {"type": "custom", "check": _survives_a_stop_straight_after_the_start},
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
