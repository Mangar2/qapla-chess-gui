#!/usr/bin/env python3
"""Runs the on-screen GUI tests (ImGui Test Engine) from a known starting state.

    python3 scripts/run-gui-tests.py
    python3 scripts/run-gui-tests.py --filter Tournament
    python3 scripts/run-gui-tests.py --keep          # keep the configuration directory

Three things this does that starting the binary by hand does not:

* **An empty configuration.** Every run gets a directory of its own, so the tests start from a
  fresh installation rather than from whatever the developer's own settings happen to hold. What
  a test needs -- engines, snackbar timings -- it sets up itself; see QaplaTest::prepareTestEnvironment.
* **The remote control stays open.** A failing GUI test says which click did not work; the HTTP
  channel says what the application thought its state was at that moment, which is usually the
  more useful half. The port is printed and left open until the run ends.
* **It runs in its own directory**, so the engine log files land there and not in the repository.

The exit code is 0 only if every registered test ran and passed.
"""

import argparse
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]

#: Presets that carry the ImGui Test Engine. Without it the run reports nothing tested.
TEST_PRESETS = {"default": "test", "release": "releasetest"}


def executable(name: str) -> str:
    return f"{name}.exe" if platform.system() == "Windows" else name


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the GUI tests from a known state")
    parser.add_argument("--config", default="default",
                        help="build configuration the test-engine preset is derived from")
    parser.add_argument("--filter", default=None,
                        help="only tests whose name matches, e.g. 'Tournament' or 'EPD/Chatbot'")
    parser.add_argument("--keep", action="store_true",
                        help="keep the configuration directory instead of removing it")
    parser.add_argument("--port", type=int, default=0,
                        help="remote control port; 0 lets the OS pick one")
    arguments = parser.parse_args()

    preset = TEST_PRESETS.get(arguments.config, "test")
    build_dir = REPO_ROOT / "build" / preset
    binary = build_dir / executable("qapla")
    if not binary.is_file():
        print(f"No test-engine build at {binary}.", file=sys.stderr)
        print(f"Build it: cmake --preset {preset} && cmake --build --preset {preset}",
              file=sys.stderr)
        return 2

    engine_dir = build_dir / "bin"
    if not (engine_dir / executable("diagnostic-engine")).is_file():
        print(f"No diagnostic engine in {engine_dir} -- build the project first.", file=sys.stderr)
        return 2

    config_dir = Path(tempfile.mkdtemp(prefix="qapla-gui-tests-"))
    environment = dict(os.environ)
    environment["QAPLA_AUTO_RUN_TESTS"] = "1"
    environment["QAPLA_TEST_ENGINE_DIR"] = str(engine_dir)
    environment["QAPLA_TEST_DATA_DIR"] = str(REPO_ROOT / "src" / "test-system" / "test-data")
    if arguments.filter:
        environment["QAPLA_TEST_FILTER"] = arguments.filter

    command = [
        str(binary),
        f"--config-dir={config_dir}",
        "--remote-control",
        f"--remote-control-port={arguments.port}",
        "--remote-control-token=gui-tests",
    ]

    print(f"Configuration directory: {config_dir}")
    print(f"Running: {' '.join(command)}", flush=True)

    # In the configuration directory, so that what the engines write lands with the rest of the
    # run's leavings rather than in the repository.
    completed = subprocess.run(command, cwd=str(config_dir), env=environment, check=False,
                               capture_output=True, text=True, errors="replace")

    output = completed.stdout + completed.stderr
    print(output)

    summary = re.search(r"QAPLA_TEST_SUMMARY tested=(\d+) success=(\d+) inQueue=(\d+)", output)
    if summary:
        tested, success, in_queue = (int(value) for value in summary.groups())
        print(f"Tested {tested}, passed {success}, left in the queue {in_queue}.")

    if arguments.keep or completed.returncode != 0:
        print(f"Configuration directory kept: {config_dir}")
    else:
        shutil.rmtree(config_dir, ignore_errors=True)

    return completed.returncode


if __name__ == "__main__":
    sys.exit(main())
