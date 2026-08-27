#!/usr/bin/env python3
"""Persistence tests -- what survives the application being closed and started again.

These are the tests the whole harness was built for, and the reason ``POST /shutdown`` exists at
all: killing the process would skip the shutdown that writes the settings, and every check here
would be meaningless. A restart in this module is a real one -- same configuration directory, new
process -- so what the second session finds is what the first one actually stored.
"""

import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as ec  # noqa: E402

FAST_TC = "0.2+0.01"


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "persistence-engine-catalog-survives-a-restart",
            "description": "Engines installed in one session are still there in the next",
            "engines": [ec.DIAG, ec.DIAG_B],
            "steps": [
                {"call": "manage_engines", "args": {"command": "list"}, "id": "before"},
                {"restart": True},
                {"call": "manage_engines", "args": {"command": "list"}, "id": "after"},
            ],
            "validators": [
                {"type": "content", "step": "before", "pattern": "Diag A"},
                {"type": "content", "step": "after", "pattern": "Diag A"},
                {"type": "content", "step": "after", "pattern": "Diag B"},
                {"type": "fileContent", "path": "qapla-chess-gui.ini", "content": "Diag A"},
            ],
        },
        {
            "name": "persistence-tournament-settings-survive-a-restart",
            "description": "A configuration set over HTTP is still configured after a restart",
            "engines": [ec.DIAG, ec.DIAG_B],
            "steps": [
                {"call": "configure_tournament",
                 "args": {"engines": [ec.DIAG, ec.DIAG_B], "games": 17,
                          "time_control": FAST_TC, "openings_file": "{openings}",
                          "event": "Restart Check"}},
                {"restart": True},
                {"call": "get_status", "args": {"type": "tournament"}, "id": "after"},
            ],
            "validators": [
                {"type": "content", "step": "after", "pattern": "Games per pairing: 17"},
                {"type": "content", "step": "after", "pattern": "Restart Check"},
                {"type": "content", "step": "after", "pattern": "Diag A"},
            ],
        },
        {
            "name": "persistence-saved-tournament-loads-after-a-restart",
            "description": "A finished tournament written to a file is read back in a new session",
            "engines": [ec.DIAG, ec.DIAG_B],
            "steps": [
                {"call": "configure_tournament",
                 "args": {"engines": [ec.DIAG, ec.DIAG_B], "games": 16,
                          "time_control": FAST_TC, "openings_file": "{openings}",
                          "event": "Saved Run"}},
                {"call": "start", "args": {"type": "tournament"}},
                {"wait": "tournament", "timeout": 120, "id": "run"},
                {"call": "save_results", "args": {"type": "tournament", "file": "{results}"},
                 "id": "save"},
                {"restart": True},
                {"call": "clear_result", "args": {"type": "tournament"}},
                {"call": "load_results", "args": {"type": "tournament", "file": "{results}"},
                 "id": "load"},
                {"call": "get_status", "args": {"type": "tournament"}, "id": "after"},
            ],
            "validators": [
                {"type": "waitReason", "step": "run", "expected": "finished"},
                {"type": "ok", "step": "save"},
                {"type": "fileExists", "path": "results.qtour"},
                {"type": "ok", "step": "load"},
                {"type": "content", "step": "after", "pattern": "Saved Run"},
                {"type": "content", "step": "after", "pattern": "Diag A"},
            ],
        },
        {
            "name": "persistence-load-a-file-that-is-not-there",
            "description": "Loading a missing results file is refused rather than half-applied",
            "engines": [ec.DIAG],
            "steps": [
                {"call": "load_results",
                 "args": {"type": "tournament", "file": "{sandbox}/never-written.qtour"},
                 "expect_ok": False, "id": "load"},
                {"call": "manage_engines", "args": {"command": "list"}, "id": "list"},
            ],
            "validators": [
                {"type": "failed", "step": "load"},
                # The refusal has to leave the session as it was, not half-replaced.
                {"type": "content", "step": "list", "pattern": "Diag A"},
            ],
        },
    ]
