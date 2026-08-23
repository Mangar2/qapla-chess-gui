#!/usr/bin/env python3
"""Engine catalog tests -- everything ``manage_engines`` can do, over HTTP.

The catalog is the first thing every other test depends on, and the first thing a fresh sandbox
does not have. These check the commands one by one, including what happens when the executable
does not exist or does not answer.
"""

import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as ec  # noqa: E402


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "engines-install-and-list",
            "description": "An installed engine is detected and then listed by name",
            "steps": [
                {"call": "manage_engines",
                 "args": {"command": "install", "path": "{engine_diag_a}", "new_name": "Diag A"},
                 "timeout": 120, "id": "install"},
                {"call": "manage_engines", "args": {"command": "list"}, "id": "list"},
            ],
            "validators": [
                {"type": "ok", "step": "install"},
                {"type": "content", "step": "install", "pattern": "Diag A"},
                {"type": "content", "step": "list", "pattern": "Diag A"},
            ],
        },
        {
            "name": "engines-details",
            "description": "Details name the executable and the protocol that was detected",
            "engines": [ec.DIAG],
            "steps": [
                {"call": "manage_engines", "args": {"command": "details", "engine": ec.DIAG},
                 "id": "details"},
            ],
            "validators": [
                {"type": "ok", "step": "details"},
                {"type": "content", "step": "details", "pattern": "uci"},
                {"type": "content", "step": "details", "pattern": "diagnostic-engine-a"},
            ],
        },
        {
            "name": "engines-copy-and-delete",
            "description": "An engine can be copied under a new name and deleted again",
            "engines": [ec.DIAG],
            "steps": [
                {"call": "manage_engines",
                 "args": {"command": "copy", "engine": ec.DIAG, "new_name": "Diag Copy"},
                 "id": "copy"},
                {"call": "manage_engines", "args": {"command": "list"}, "id": "after_copy"},
                {"call": "manage_engines",
                 "args": {"command": "delete", "engine": "Diag Copy"}, "id": "delete"},
                {"call": "manage_engines", "args": {"command": "list"}, "id": "after_delete"},
            ],
            "validators": [
                {"type": "ok", "step": "copy"},
                {"type": "content", "step": "after_copy", "pattern": "Diag Copy"},
                {"type": "ok", "step": "delete"},
                {"type": "contentLacks", "step": "after_delete", "pattern": "Diag Copy"},
            ],
        },
        {
            "name": "engines-install-nonexistent-path",
            "description": "Installing something that is not there is refused, saying so",
            "steps": [
                {"call": "manage_engines",
                 "args": {"command": "install", "path": "{sandbox}/no-such-engine",
                          "new_name": "Ghost"},
                 "expect_ok": False, "id": "install"},
                {"call": "manage_engines", "args": {"command": "list"}, "id": "list"},
            ],
            "validators": [
                {"type": "failed", "step": "install"},
                {"type": "contentLacks", "step": "list", "pattern": "Ghost"},
            ],
        },
        {
            "name": "engines-details-of-unknown-engine",
            "description": "Asking about an engine that was never installed is an error, not empty",
            "steps": [
                {"call": "manage_engines",
                 "args": {"command": "details", "engine": "Nothing Like This"},
                 "expect_ok": False, "id": "details"},
            ],
            "validators": [
                {"type": "failed", "step": "details"},
            ],
        },
        {
            "name": "engines-real-engines-install",
            "description": "The two real engines install and are detected as UCI",
            "engines": [ec.QAPLA, ec.SPIKE],
            "steps": [
                {"call": "manage_engines", "args": {"command": "list"}, "id": "list"},
                {"call": "manage_engines", "args": {"command": "details", "engine": ec.QAPLA},
                 "id": "qapla"},
            ],
            "validators": [
                {"type": "content", "step": "list", "pattern": "Qapla"},
                {"type": "content", "step": "list", "pattern": "Spike"},
                {"type": "ok", "step": "qapla"},
                {"type": "content", "step": "qapla", "pattern": "uci"},
            ],
        },
    ]
