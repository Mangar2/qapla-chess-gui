#!/usr/bin/env python3
"""The directory one test lets the GUI keep its state in.

A fresh directory is a fresh installation: no engines, no window layout, no leftover tournament.
That is the whole point -- a test that starts from whatever the last run left behind proves
nothing, and the GUI test suite has been paying for exactly that.

Kept under ``test/integration/sandbox/`` (git-ignored) rather than under the system temp
directory, so that what a failing test left behind is somewhere obvious.
"""

import shutil
from pathlib import Path

SANDBOX_ROOT = Path(__file__).resolve().parent / "sandbox"


def _safe_name(test_name: str) -> str:
    return "".join(character if character.isalnum() or character in "-_" else "-"
                   for character in test_name)


def fresh_sandbox(test_name: str) -> Path:
    """Removes anything left from an earlier run of this test and returns the empty directory."""
    path = SANDBOX_ROOT / _safe_name(test_name)
    shutil.rmtree(path, ignore_errors=True)
    path.mkdir(parents=True, exist_ok=True)
    return path


def discard(path: Path) -> None:
    """Throws a sandbox away. Not called when the test failed -- then it is evidence."""
    shutil.rmtree(path, ignore_errors=True)


def clear_all() -> None:
    shutil.rmtree(SANDBOX_ROOT, ignore_errors=True)
