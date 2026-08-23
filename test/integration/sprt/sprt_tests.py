#!/usr/bin/env python3
"""SPRT tests -- the second run type, which shares almost everything with the tournament.

Kept short on purpose: what is worth testing separately is that SPRT has a configuration of its
own that a tournament cannot disturb, that a test with a low game cap ends at that cap, and that
the verdict comes back.
"""

import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as ec  # noqa: E402

FAST_TC = "0.2+0.01"


def _sprt_configuration(**overrides: Any) -> Dict[str, Any]:
    configuration = {
        "champion": ec.DIAG,
        "challenger": ec.DIAG_B,
        "time_control": FAST_TC,
        "max_games": 4,
        "openings_file": "{openings}",
        "pgn_file": "{pgn}",
    }
    configuration.update(overrides)
    return configuration


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "sprt-runs-to-the-game-limit",
            "description": "An undecidable SPRT test stops at max_games and reports it",
            "engines": [ec.DIAG, ec.DIAG_B],
            "steps": [
                {"call": "configure_sprt", "args": _sprt_configuration(), "id": "config"},
                {"call": "start", "args": {"type": "sprt"}, "id": "start"},
                {"wait": "sprt", "timeout": 180, "id": "run"},
            ],
            "validators": [
                {"type": "content", "step": "config", "pattern": "Diag A"},
                {"type": "content", "step": "config", "pattern": "Diag B"},
                {"type": "ok", "step": "start"},
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "fileExists", "path": "games.pgn"},
            ],
        },
        {
            "name": "sprt-has-its-own-configuration",
            "description": "Configuring a tournament leaves the SPRT settings alone",
            "engines": [ec.DIAG, ec.DIAG_B, ec.QAPLA],
            "steps": [
                {"call": "configure_sprt", "args": _sprt_configuration()},
                {"call": "configure_tournament",
                 "args": {"engines": [ec.DIAG, ec.QAPLA], "games": 6,
                          "time_control": "1+0", "openings_file": "{openings}"}},
                {"call": "get_status", "args": {"type": "sprt"}, "id": "sprt_status"},
                {"call": "get_status", "args": {"type": "tournament"}, "id": "tour_status"},
            ],
            "validators": [
                # Each run type keeps its own engines, time control and openings -- the thing a
                # caller switching between them has to be able to rely on.
                {"type": "content", "step": "sprt_status", "pattern": "Diag B"},
                {"type": "content", "step": "sprt_status", "pattern": "0.2"},
                {"type": "content", "step": "tour_status", "pattern": "Qapla"},
                {"type": "content", "step": "tour_status", "pattern": "Games per pairing: 6"},
            ],
        },
        {
            "name": "sprt-concurrency-survives-a-stop",
            "description": "An abrupt stop leaves the configured concurrency alone",
            "engines": [ec.DIAG, ec.DIAG_B],
            "steps": [
                {"call": "configure_sprt",
                 "args": _sprt_configuration(max_games=1000, time_control="2+0.1",
                                             concurrency=5),
                 "id": "config"},
                {"call": "start", "args": {"type": "sprt"}},
                {"call": "stop", "args": {"type": "sprt", "mode": "abrupt"}, "id": "stop"},
                {"wait": "sprt", "timeout": 120},
                {"call": "get_status", "args": {"type": "sprt"}, "id": "after"},
            ],
            "validators": [
                {"type": "content", "step": "config", "pattern": "Concurrency: 5"},
                {"type": "ok", "step": "stop"},
                {"type": "content", "step": "after", "pattern": "Concurrency: 5"},
            ],
        },
        {
            "name": "sprt-stop-while-running",
            "description": "A long SPRT test can be stopped from outside",
            "engines": [ec.DIAG, ec.DIAG_B],
            "steps": [
                {"call": "configure_sprt",
                 "args": _sprt_configuration(max_games=1000, time_control="2+0.1")},
                {"call": "start", "args": {"type": "sprt"}},
                {"call": "stop", "args": {"type": "sprt"}, "id": "stop"},
                {"wait": "sprt", "timeout": 120, "id": "run"},
            ],
            "validators": [
                {"type": "ok", "step": "stop"},
                {"type": "waitReason", "step": "run",
                 "expected": ["stopped", "not_running"]},
            ],
        },
    ]
