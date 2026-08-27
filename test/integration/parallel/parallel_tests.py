#!/usr/bin/env python3
"""Parallel tests -- more than one run at a time, and the overview that reports them.

The GUI lets a tournament and an SPRT test go at once; they share the game manager pool. What a
caller needs from that is one truthful answer to "what is going on", and the ability to stop one
without disturbing the other. That is what these check.
"""

import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as ec  # noqa: E402

PAIR = [ec.DIAG, ec.DIAG_B]

#: Slow enough that both runs are certainly still going while the overview is read.
SLOW_TC = "2+0.1"


def _configure_both() -> List[Dict[str, Any]]:
    return [
        {"call": "configure_tournament",
         "args": {"engines": PAIR, "games": 200, "time_control": SLOW_TC,
                  "openings_file": "{openings}"}},
        {"call": "configure_sprt",
         "args": {"champion": ec.DIAG, "challenger": ec.DIAG_B, "max_games": 200,
                  "time_control": SLOW_TC, "openings_file": "{openings}"}},
    ]


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "parallel-overview-names-both-runs",
            # A tournament and an SPRT at the same time: the suite's concurrency is a
            # budget for the test, so each run gets half of it and what is actually
            # being played stays what every other test plays.
            "parallel_runs": 2,
            "description": "A tournament and an SPRT test at once are both reported as running",
            "engines": PAIR,
            "steps": _configure_both() + [
                {"call": "start", "args": {"type": "tournament"}, "id": "start_tournament"},
                {"call": "start", "args": {"type": "sprt"}, "id": "start_sprt"},
                {"state": True, "id": "state"},
                {"status": True, "id": "overview"},
                {"call": "stop", "args": {"type": "tournament"}},
                {"call": "stop", "args": {"type": "sprt"}},
            ],
            "validators": [
                {"type": "ok", "step": "start_tournament"},
                {"type": "ok", "step": "start_sprt"},
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "running", "expected": True},
                {"type": "stateField", "step": "state", "activity": "sprt",
                 "field": "running", "expected": True},
                # The spoken overview is checked too, since naming both runs in one sentence is
                # its own job -- but what is running is read from the data.
                {"type": "content", "step": "overview", "pattern": "tournament is running"},
                {"type": "content", "step": "overview", "pattern": "SPRT test is running"},
            ],
        },
        {
            "name": "parallel-stopping-one-leaves-the-other",
            # A tournament and an SPRT at the same time: the suite's concurrency is a
            # budget for the test, so each run gets half of it and what is actually
            # being played stays what every other test plays.
            "parallel_runs": 2,
            "description": "Stopping the tournament does not stop the SPRT test beside it",
            "engines": PAIR,
            "steps": _configure_both() + [
                {"call": "start", "args": {"type": "tournament"}},
                {"call": "start", "args": {"type": "sprt"}},
                {"call": "stop", "args": {"type": "tournament"}, "id": "stop"},
                {"wait": "tournament", "timeout": 120, "id": "tournament_ended"},
                {"state": True, "id": "state"},
                {"call": "stop", "args": {"type": "sprt"}},
                {"wait": "sprt", "timeout": 120},
            ],
            "validators": [
                {"type": "ok", "step": "stop"},
                {"type": "waitReason", "step": "tournament_ended",
                 "expected": ["stopped", "not_running"]},
                # The one that was not asked to stop is still going. Stopping a run stops the
                # games of that run, not everything sharing the game manager pool with it.
                {"type": "stateField", "step": "state", "activity": "sprt",
                 "field": "running", "expected": True},
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "running", "expected": False},
            ],
        },
        {
            "name": "parallel-second-start-of-the-same-type-is-refused",
            "description": "One tournament at a time: starting it twice is an error, not a second run",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament",
                 "args": {"engines": PAIR, "games": 200, "time_control": SLOW_TC,
                          "openings_file": "{openings}"}},
                {"call": "start", "args": {"type": "tournament"}, "id": "first"},
                {"call": "start", "args": {"type": "tournament"}, "expect_ok": False,
                 "id": "second"},
                {"call": "stop", "args": {"type": "tournament"}},
                {"wait": "tournament", "timeout": 120},
            ],
            "validators": [
                {"type": "ok", "step": "first"},
                {"type": "failed", "step": "second"},
            ],
        },
    ]
