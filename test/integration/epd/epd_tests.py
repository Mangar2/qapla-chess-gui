#!/usr/bin/env python3
"""EPD tests -- analysing a set of positions rather than playing games.

The position file is four mates in one, written into every sandbox by the framework. Mates rather
than quiet positions on purpose: a real engine finds all four in hundredths of a second, so the
test can assert *how many* were solved instead of only that the run ended. The engine that plays
random moves solves none of them, which makes the opposite case just as checkable.
"""

import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as ec  # noqa: E402


def _solved_every_position(_session, results):
    """Every row of the report has to name the depth the mate was found at, not a dash."""
    text = str(results["run"].get("content", ""))
    unsolved = [line for line in text.splitlines()
                if line.startswith("mate-") and (" - " in line or line.rstrip().endswith("?"))]
    if unsolved:
        return False, "positions a real engine should have solved were not: " + "; ".join(unsolved)
    return True, "all four mates in one were found"


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "epd-solves-mates-in-one",
            "description": "A real engine analyses four mates in one and finds all of them",
            "engines": [ec.QAPLA],
            "steps": [
                {"call": "configure_epd",
                 "args": {"engines": [ec.QAPLA], "epd_file": "{epd}",
                          "max_time_seconds": 2, "min_time_seconds": 0},
                 "id": "config"},
                {"call": "start", "args": {"type": "epd"}, "id": "start"},
                {"wait": "epd", "timeout": 120, "id": "run"},
            ],
            "validators": [
                {"type": "content", "step": "config", "pattern": "positions.epd"},
                {"type": "ok", "step": "start"},
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "content", "step": "run", "pattern": "mate-back-rank"},
                {"type": "custom", "check": _solved_every_position},
            ],
        },
        {
            "name": "epd-reports-what-was-not-found",
            "description": "An engine that plays at random solves none, and the run still ends",
            "engines": [ec.DIAG],
            "steps": [
                {"call": "configure_epd",
                 "args": {"engines": [ec.DIAG], "epd_file": "{epd}",
                          "max_time_seconds": 1, "min_time_seconds": 0}},
                {"call": "start", "args": {"type": "epd"}},
                {"wait": "epd", "timeout": 120, "id": "run"},
            ],
            "validators": [
                {"type": "waitReason", "step": "run", "expected": "finished"},
                # Every position is listed; none of them is reported as solved.
                {"type": "content", "step": "run", "pattern": "mate-rook-corner"},
                {"type": "content", "step": "run", "pattern": r"mate-back-rank\s+\S+\s+-",
                 "isRegex": True},
            ],
        },
        {
            "name": "epd-concurrency-survives-a-stop",
            "description": "An abrupt stop leaves the configured concurrency alone",
            "engines": [ec.QAPLA],
            "steps": [
                {"call": "configure_epd",
                 "args": {"engines": [ec.QAPLA], "epd_file": "{epd}",
                          "max_time_seconds": 30, "min_time_seconds": 5, "concurrency": 5},
                 "id": "config"},
                {"call": "start", "args": {"type": "epd"}},
                {"call": "stop", "args": {"type": "epd", "mode": "abrupt"}, "id": "stop"},
                {"wait": "epd", "timeout": 120},
                {"call": "get_status", "args": {"type": "epd"}, "id": "after"},
            ],
            "validators": [
                {"type": "content", "step": "config", "pattern": "Concurrency: 5"},
                {"type": "ok", "step": "stop"},
                {"type": "content", "step": "after", "pattern": "Concurrency: 5"},
            ],
        },
        {
            "name": "epd-needs-a-position-file",
            "description": "Starting without a position file is refused, with the reason",
            "engines": [ec.DIAG],
            "steps": [
                {"call": "configure_epd", "args": {"engines": [ec.DIAG]}},
                {"call": "start", "args": {"type": "epd"}, "expect_ok": False, "id": "start"},
            ],
            "validators": [
                {"type": "failed", "step": "start"},
            ],
        },
        {
            "name": "epd-start-after-a-config-change-is-refused",
            "description": "Re-analysing without clearing is refused, and the refusal says how",
            "engines": [ec.QAPLA],
            "steps": [
                {"call": "configure_epd",
                 "args": {"engines": [ec.QAPLA], "epd_file": "{epd}", "max_time_seconds": 2,
                          "min_time_seconds": 0}},
                {"call": "start", "args": {"type": "epd"}},
                {"wait": "epd", "timeout": 120},
                # Changed but not cleared: the results on screen were produced under the old
                # setting, and re-running would silently mix the two.
                {"call": "configure_epd", "args": {"max_time_seconds": 1}, "id": "change"},
                {"call": "start", "args": {"type": "epd"}, "expect_ok": False, "id": "blocked"},
                {"call": "clear_result", "args": {"type": "epd"}, "id": "clear"},
                {"call": "start", "args": {"type": "epd"}, "id": "allowed"},
                {"wait": "epd", "timeout": 120},
            ],
            "validators": [
                {"type": "ok", "step": "change"},
                {"type": "failed", "step": "blocked"},
                # A refusal that names its own way out, rather than leaving the caller guessing.
                {"type": "content", "step": "blocked", "pattern": "clear_result"},
                {"type": "ok", "step": "clear"},
                {"type": "ok", "step": "allowed"},
            ],
        },
        {
            "name": "epd-two-engines-side-by-side",
            "description": "Two engines analyse the same positions and both get a column",
            "allow_ui_stalls": "installing an engine detects it synchronously on the UI thread (Actions::installEngines -> autoDetectSync). Remove this line when that moves off the UI thread.",
            "engines": [ec.QAPLA, ec.SPIKE],
            "steps": [
                {"call": "configure_epd",
                 "args": {"engines": [ec.QAPLA, ec.SPIKE], "epd_file": "{epd}",
                          "max_time_seconds": 2, "min_time_seconds": 0}},
                {"call": "start", "args": {"type": "epd"}},
                {"wait": "epd", "timeout": 180, "id": "run"},
            ],
            "validators": [
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "content", "step": "run", "pattern": "Qapla"},
                {"type": "content", "step": "run", "pattern": "Spike"},
            ],
        },
    ]
