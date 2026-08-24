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
import time
from pathlib import Path
from typing import Dict, List, Optional

REPO_ROOT = Path(__file__).resolve().parents[1]

#: The preset that carries the ImGui Test Engine, per build configuration.
#:
#: The GUI suites are compiled in only when QAPLA_WITH_TEST_ENGINE is on, and it is off in both
#: ordinary presets. Running the plain binary with QAPLA_AUTO_RUN_TESTS set does not fail loudly,
#: it reports nothing tested -- which this check treats as a failure, correctly, but for a reason
#: that would take a while to work out.
GUI_TEST_PRESET = {"default": "test", "release": "releasetest"}


def executable(name: str) -> str:
    return f"{name}.exe" if platform.system() == "Windows" else name


def format_duration(seconds: float) -> str:
    if seconds < 60:
        return f"{seconds:.1f}s"
    return f"{int(seconds // 60)}m {seconds % 60:.0f}s"


def ensure_configured(preset: str) -> bool:
    """Runs `cmake --preset` when that preset has no build directory yet.

    So that asking for the release check does not first require knowing which presets it uses.
    """
    if (REPO_ROOT / "build" / preset / "CMakeCache.txt").is_file():
        return True
    print(f"  configuring preset '{preset}' (no build directory yet)", flush=True)
    completed = subprocess.run(["cmake", "--preset", preset], cwd=str(REPO_ROOT), check=False)
    return completed.returncode == 0


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
    try:
        completed = subprocess.run(command, cwd=str(REPO_ROOT), env=merged, check=False)
    except OSError as error:
        # A missing binary is a failed stage with a reason, not a stack trace, and it is a real
        # possibility: every build preset here restricts itself to the "qapla" target, so anything
        # else this check runs has to be named as a target above or it is simply not there.
        print(f"-- {title}: FAILED, could not run it: {error}", file=sys.stderr)
        return False
    runtime = format_duration(time.monotonic() - started)

    if completed.returncode == 0:
        print(f"-- {title}: ok ({runtime})")
        return True
    print(f"-- {title}: FAILED with code {completed.returncode} ({runtime})", file=sys.stderr)
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Build and run every test layer before a release")
    parser.add_argument("--config", default="release",
                        help="build configuration to check: 'release' (the default -- a release "
                             "check should check what ships) or 'default' for the debug build")
    parser.add_argument("--skip-gui", action="store_true",
                        help="leave out the on-screen GUI suite")
    arguments = parser.parse_args()

    build_dir = REPO_ROOT / "build" / arguments.config
    gui_preset = GUI_TEST_PRESET.get(arguments.config, "test")

    if not ensure_configured(arguments.config):
        print(f"Could not configure preset '{arguments.config}'.", file=sys.stderr)
        return 1
    if not arguments.skip_gui and not ensure_configured(gui_preset):
        print(f"Could not configure preset '{gui_preset}'.", file=sys.stderr)
        return 1

    # Named targets rather than the preset's own: every build preset restricts itself to "qapla",
    # so a check that trusted it built the GUI and then ran a unit-tests binary and diagnostic
    # engines that nothing had rebuilt. A release check that certifies stale binaries is worse
    # than none.
    stages = [
        (f"Build ({arguments.config})",
         ["cmake", "--build", "--preset", arguments.config,
          "--target", "qapla", "unit-tests", "diagnostic-engine"], None),
        ("Unit tests",
         [str(build_dir / executable("unit-tests"))], None),
        ("Integration tests (HTTP)",
         [sys.executable, str(REPO_ROOT / "test" / "integration" / "test_runner.py"),
          "--config", arguments.config], None),
    ]

    if not arguments.skip_gui:
        # Last, because it is the slowest of the three. Still a gate, with --skip-gui as the
        # deliberate way past it rather than a quiet exclusion.
        #
        # Its own build, because the suites are only compiled in with QAPLA_WITH_TEST_ENGINE.
        stages.append((
            f"Build with the test engine ({gui_preset})",
            ["cmake", "--build", "--preset", gui_preset,
             "--target", "qapla", "diagnostic-engine"], None))
        # Through the runner rather than by starting the binary here: the suite needs a starting
        # state -- an empty configuration, the two real engines, a directory of its own to leave
        # engine logs in -- and that state is set up in one place. Started by hand from the
        # repository, the run left its engine logs in the repository and depended on having been
        # started from exactly the right directory.
        stages.append((
            "GUI tests (ImGui Test Engine)",
            [sys.executable, str(REPO_ROOT / "scripts" / "run-gui-tests.py"),
             "--config", arguments.config], None))

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
