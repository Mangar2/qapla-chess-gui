#!/usr/bin/env python3
"""CLOP tests -- parameter tuning, the fourth run type.

Kept to what is worth checking from outside: that a tuning run configures, plays its samples,
ends, and reports an estimate for the parameter it was given. What the estimate *is* is not a
question an integration test can answer -- the engine under tuning here plays at random, so its
Hash value cannot influence anything. The point is the machinery around it.
"""

import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as ec  # noqa: E402

PAIR = [ec.DIAG, ec.DIAG_B]


def _clop_configuration(**overrides: Any) -> Dict[str, Any]:
    configuration = {
        "engine": ec.DIAG,
        "opponents": [ec.DIAG_B],
        "parameters": {"Hash": "1 64"},
        "samples": 4,
        "games_per_sample": 2,
        "warmup_samples": 2,
        "time_control": "0.2+0.01",
        "openings_file": "{openings}",
        "seed": 7,
    }
    configuration.update(overrides)
    return configuration


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "clop-runs-to-the-end",
            "description": "A short tuning run plays its samples and reports an estimate",
            "engines": PAIR,
            "steps": [
                {"call": "configure_clop", "args": _clop_configuration(), "id": "config"},
                {"call": "start", "args": {"type": "clop"}, "id": "start"},
                {"wait": "clop", "timeout": 300, "id": "run"},
            ],
            "validators": [
                {"type": "content", "step": "config", "pattern": "Hash (1..64)"},
                {"type": "content", "step": "config", "pattern": "Engine under tuning: Diag A"},
                {"type": "ok", "step": "start"},
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "content", "step": "run", "pattern": "Hash"},
            ],
        },
        {
            "name": "clop-needs-a-parameter",
            "description": "Tuning nothing is refused: a run without a parameter has no subject",
            "engines": PAIR,
            "steps": [
                {"call": "configure_clop",
                 "args": {"engine": ec.DIAG, "opponents": [ec.DIAG_B],
                          "time_control": "0.2+0.01", "openings_file": "{openings}"}},
                {"call": "start", "args": {"type": "clop"}, "expect_ok": False, "id": "start"},
            ],
            "validators": [
                {"type": "failed", "step": "start"},
            ],
        },
        {
            "name": "clop-stop-while-running",
            "description": "A long tuning run can be stopped from outside",
            "engines": PAIR,
            "steps": [
                # Few active pairs, many samples: starting stays cheap while the run is
                # certainly still going when stop arrives. The generous timeout on start is not
                # cosmetic -- starting a CLOP run builds its optimizer and launches its engines
                # on the UI thread, and on a loaded machine that has been measured taking longer
                # than a minute, during which the application answers nothing at all.
                {"call": "configure_clop",
                 "args": _clop_configuration(samples=500, active_pairs=2, time_control="2+0.1")},
                {"call": "start", "args": {"type": "clop"}, "timeout": 180},
                {"call": "stop", "args": {"type": "clop"}, "id": "stop"},
                {"wait": "clop", "timeout": 180, "id": "run"},
            ],
            "validators": [
                {"type": "ok", "step": "stop"},
                {"type": "waitReason", "step": "run",
                 "expected": ["stopped", "not_running"]},
            ],
        },
    ]
