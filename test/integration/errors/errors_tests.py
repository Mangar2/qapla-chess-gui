#!/usr/bin/env python3
"""Error-path tests -- engines that do not behave.

The three broken variants of the diagnostic engine are built alongside the good one and are the
only dependable way to reach these paths: an engine that never answers, one that hangs on
``isready``, and one that keeps overspending its clock until it forfeits.

These tests are the slow ones in the suite, and unavoidably so. Detecting an engine that will not
answer costs the protocol timeout twice over -- about three quarters of a minute -- and a game
against one costs it again per game. That is the application behaving correctly; it is measured
here so that a change which makes it worse shows up as a runtime, not as a mystery.
"""

import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as ec  # noqa: E402

FAST_TC = "0.2+0.01"


def _tournament_with(opponent: str, **overrides: Any) -> Dict[str, Any]:
    configuration = {
        "engines": [ec.DIAG, opponent],
        "games": 2,
        "time_control": FAST_TC,
        "openings_file": "{openings}",
        "pgn_file": "{pgn}",
    }
    configuration.update(overrides)
    return configuration


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "errors-engine-that-never-answers",
            "slow": True,
            "description": "An engine that ignores everything loses its games; the run still ends",
            "engines": [ec.DIAG, ec.DIAG_NOINIT],
            "steps": [
                {"call": "configure_tournament", "args": _tournament_with(ec.DIAG_NOINIT)},
                {"call": "start", "args": {"type": "tournament"}, "id": "start"},
                {"wait": "tournament", "timeout": 600, "id": "run"},
            ],
            "validators": [
                {"type": "ok", "step": "start"},
                # The point: a broken opponent must not leave the application waiting forever.
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "content", "step": "run", "pattern": r"Diag NoInit\s+\S+\s+-\s+0\.0/2",
                 "isRegex": True},
            ],
        },
        {
            "name": "errors-engine-that-hangs",
            "slow": True,
            "description": "An engine that hangs on isready does not hang the application",
            "engines": [ec.DIAG, ec.DIAG_LOOP],
            "steps": [
                {"call": "configure_tournament", "args": _tournament_with(ec.DIAG_LOOP)},
                {"call": "start", "args": {"type": "tournament"}, "id": "start"},
                {"wait": "tournament", "timeout": 600, "id": "run"},
            ],
            "validators": [
                {"type": "ok", "step": "start"},
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "content", "step": "run", "pattern": "Diag Loop"},
            ],
        },
        {
            "name": "errors-engine-that-loses-on-time",
            "slow": True,
            "description": "An engine that overspends its clock forfeits, and the cause is recorded",
            "engines": [ec.DIAG, ec.DIAG_LOSSONTIME],
            "steps": [
                {"call": "configure_tournament",
                 "args": _tournament_with(ec.DIAG_LOSSONTIME, time_control="1+0")},
                {"call": "start", "args": {"type": "tournament"}, "id": "start"},
                {"wait": "tournament", "timeout": 600, "id": "run"},
            ],
            "validators": [
                {"type": "ok", "step": "start"},
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "content", "step": "run", "pattern": "Diag LossOnTime"},
                {"type": "fileExists", "path": "games.pgn"},
            ],
        },
    ]
