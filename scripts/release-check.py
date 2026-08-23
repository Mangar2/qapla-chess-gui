#!/usr/bin/env python3
"""The check that goes in front of a release.

Builds, then runs all three test layers in the order that finds the cheapest failures first. The
first stage that fails ends the run, and the exit code is 0 only if every stage passed -- so this
is something a release script can gate on rather than something a person has to read.

    python3 scripts/release-check.py
    python3 scripts/release-check.py --config release
    python3 scripts/release-check.py --skip-gui

Every stage that starts the GUI gives it a configuration directory of its own, so a release check
never touches, and never depends on, the configuration you work with.

Python rather than a shell script on purpose: the release has to be checked on Windows as well as
on Linux and macOS, and one file that runs everywhere beats a .sh and a .ps1 that have to be kept
saying the same thing.
"""

import argparse
import os
import platform
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Dict, List, Optional

REPO_ROOT = Path(__file__).resolve().parents[1]


def executable(name: str) -> str:
    return f"{name}.exe" if platform.system() == "Windows" else name


def format_duration(seconds: float) -> str:
    if seconds < 60:
        return f"{seconds:.1f}s"
    return f"{int(seconds // 60)}m {seconds % 60:.0f}s"


def run_stage(number: int, title: str, command: List[str],
              environment: Optional[Dict[str, str]] = None) -> bool:
    """Runs one stage, letting its output through as it happens."""
    print()
    print("=" * 46)
    print(f"  {number}. {title}")
    print("=" * 46)
    print(f"  {' '.join(command)}", flush=True)

    started = time.monotonic()
    merged = dict(os.environ)
    merged.update(environment or {})
    completed = subprocess.run(command, cwd=str(REPO_ROOT), env=merged, check=False)
    runtime = format_duration(time.monotonic() - started)

    if completed.returncode == 0:
        print(f"-- {title}: ok ({runtime})")
        return True
    print(f"-- {title}: FAILED with code {completed.returncode} ({runtime})", file=sys.stderr)
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Build and run every test layer before a release")
    parser.add_argument("--config", default="default",
                        help="build configuration to check (default, release)")
    parser.add_argument("--skip-gui", action="store_true",
                        help="leave out the on-screen GUI suite")
    arguments = parser.parse_args()

    build_dir = REPO_ROOT / "build" / arguments.config

    with tempfile.TemporaryDirectory(prefix="qapla-release-check-") as scratch:
        stages = [
            (f"Build ({arguments.config})",
             ["cmake", "--build", "--preset", arguments.config], None),
            ("Unit tests",
             [str(build_dir / executable("unit-tests"))], None),
            ("Integration tests (HTTP)",
             [sys.executable, str(REPO_ROOT / "test" / "integration" / "test_runner.py"),
              "--config", arguments.config], None),
        ]

        if not arguments.skip_gui:
            # Last, because it is the slowest and, today, the least steady of the three: a full
            # run does not always report the same number of tests. Still a gate, with --skip-gui
            # as the deliberate way past it rather than a quiet exclusion.
            stages.append((
                "GUI tests (ImGui Test Engine)",
                [str(build_dir / executable("qapla")),
                 f"--config-dir={Path(scratch) / 'gui-tests'}"],
                {"QAPLA_AUTO_RUN_TESTS": "1"},
            ))

        for number, (title, command, environment) in enumerate(stages, start=1):
            if not run_stage(number, title, command, environment):
                print()
                print(f"Release check failed at: {title}", file=sys.stderr)
                return 1

    print()
    print("=" * 46)
    print("  All stages passed. Ready to release.")
    print("=" * 46)
    return 0


if __name__ == "__main__":
    sys.exit(main())
