#!/usr/bin/env python3
"""Runs the GUI integration tests.

    python3 test/integration/test_runner.py                 # everything
    python3 test/integration/test_runner.py --filter tournament-*
    python3 test/integration/test_runner.py --test session-starts --keep-sandbox
    python3 test/integration/test_runner.py --list

The one entry point, on all three platforms. On Linux without a desktop session it puts itself
under xvfb-run, so nothing outside needs to know about that.

Each test starts its own GUI with a configuration directory of its own and drives it over the
HTTP remote control. The runner adds three things around that: it says up front what is missing
instead of letting a run fail piecemeal, it keeps a protocol so a long run can be picked up where
it stopped, and it checks that the suite left the developer's own configuration untouched.
"""

import argparse
import hashlib
import importlib.util
import os
import platform
import re
import shutil
import sys
from pathlib import Path
from typing import Any, Dict, List, Tuple

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

import engines as engine_catalog  # noqa: E402  (needs the path above)
from test_framework import Colors, format_duration, invoke_test  # noqa: E402

RESULTS_FILE = HERE / "test_results.log"
_RESULT_ENTRY = re.compile(r"^(?P<verdict>PASSED|FAILED)(?: \((?P<runtime>.+)\))?$")

#: Set on the way into xvfb-run, so the second start does not try the same thing again.
_UNDER_XVFB = "QAPLA_IT_UNDER_XVFB"


def ensure_a_display() -> None:
    """On Linux without a desktop session, starts this run again under a virtual display.

    The tests really open a window -- that is the point of the suite, since it drives the same
    application a person would be watching. A machine without a display cannot do that, so the
    run puts xvfb in front of itself rather than failing at the first test with an error about
    GLFW.
    """
    if platform.system() != "Linux":
        return
    if os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"):
        return
    if os.environ.get(_UNDER_XVFB):
        return

    xvfb_run = shutil.which("xvfb-run")
    if xvfb_run is None:
        print("No display, and xvfb-run is not installed: the GUI cannot open a window.",
              file=sys.stderr)
        print("Install it (apt install xvfb) or run from a desktop session.", file=sys.stderr)
        raise SystemExit(2)

    print("No display found -- running under xvfb-run.")
    os.environ[_UNDER_XVFB] = "1"
    os.execvp(xvfb_run, [xvfb_run, "-a", sys.executable, str(Path(__file__).resolve()),
                         *sys.argv[1:]])


# ---------------------------------------------------------------------------
# Protocol of earlier runs
# ---------------------------------------------------------------------------


def load_previous_results() -> Dict[str, str]:
    results: Dict[str, str] = {}
    if not RESULTS_FILE.exists():
        return results
    for line in RESULTS_FILE.read_text(encoding="utf-8").splitlines():
        parts = line.strip().rsplit(" - ", 1)
        if len(parts) == 2:
            results[parts[0]] = parts[1]
    return results


def save_results(results: Dict[str, str]) -> None:
    RESULTS_FILE.write_text(
        "".join(f"{name} - {results[name]}\n" for name in sorted(results)), encoding="utf-8"
    )


def verdict_of(entry: str) -> str:
    match = _RESULT_ENTRY.match(entry)
    return match.group("verdict") if match else entry


# ---------------------------------------------------------------------------
# The promise: the developer's own configuration is not touched
# ---------------------------------------------------------------------------


def real_config_fingerprint() -> str:
    """A hash over the configuration directory the developer actually uses.

    Compared before and after the run. Every session is started with --config-dir, so this must
    not change -- and if it ever does, that is a bug worth stopping for, not a detail.
    """
    from os_config_directory import real_config_directory

    directory = real_config_directory()
    if not directory.is_dir():
        return "absent"
    digest = hashlib.sha256()
    for path in sorted(p for p in directory.iterdir() if p.is_file()):
        digest.update(path.name.encode("utf-8"))
        try:
            digest.update(str(path.stat().st_mtime_ns).encode("utf-8"))
            digest.update(str(path.stat().st_size).encode("utf-8"))
        except OSError:
            pass
    return digest.hexdigest()


# ---------------------------------------------------------------------------
# Discovery
# ---------------------------------------------------------------------------


def load_module_tests(test_file: Path) -> List[Dict[str, Any]]:
    spec = importlib.util.spec_from_file_location(test_file.stem, test_file)
    if not spec or not spec.loader:
        return []
    module = importlib.util.module_from_spec(spec)
    try:
        spec.loader.exec_module(module)
    except Exception as error:
        print(f"{Colors.RED}Cannot load {test_file}: {error}{Colors.RESET}")
        return []
    return module.get_tests() if hasattr(module, "get_tests") else []


def collect_tests() -> List[Dict[str, Any]]:
    """Every test the suite has, the slow ones last.

    Ordering matters for how the suite feels to use: the error-path tests take minutes each,
    because provoking an engine that will not answer means waiting out its timeouts. Running them
    first would mean four minutes of nothing before the first quick verdict. A module marks them
    with ``"slow": True`` and they move to the end, where they belong.
    """
    found: List[Dict[str, Any]] = []
    for directory in sorted(p for p in HERE.iterdir() if p.is_dir()):
        module_file = directory / f"{directory.name}_tests.py"
        if module_file.exists():
            found.extend(load_module_tests(module_file))
    return sorted(found, key=lambda test: bool(test.get("slow")))


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main() -> int:
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, ValueError):
            pass

    parser = argparse.ArgumentParser(description="Run the GUI integration tests")
    parser.add_argument("--filter", default="*", help="name pattern, wildcards allowed")
    parser.add_argument("--test", help="run one test by name")
    parser.add_argument("--list", action="store_true", help="list the tests and exit")
    parser.add_argument("--config", default="default", help="build configuration (default/release)")
    parser.add_argument("--skip-passed", action="store_true",
                        help="skip what passed in the last run")
    parser.add_argument("--keep-sandbox", action="store_true",
                        help="keep the configuration directory of tests that passed, too")
    parser.add_argument("--timeout-scale", type=float, default=1.0,
                        help="stretch every timeout, for slow machines and CI")
    arguments = parser.parse_args()

    if not arguments.list:
        ensure_a_display()

    all_tests = collect_tests()
    if arguments.list:
        print(f"{Colors.CYAN}Available tests:{Colors.RESET}\n")
        for test in all_tests:
            print(f"  {Colors.GREEN}+ {test['name']}{Colors.RESET}")
            print(f"    {Colors.GRAY}{test.get('description', '')}{Colors.RESET}")
        return 0

    if arguments.test:
        tests_to_run = [t for t in all_tests if t["name"] == arguments.test]
        if not tests_to_run:
            print(f"{Colors.RED}No such test: {arguments.test}{Colors.RESET}")
            return 1
    else:
        pattern = re.compile(arguments.filter.replace("*", ".*").replace("?", "."))
        tests_to_run = [t for t in all_tests if pattern.match(t["name"])]

    if not tests_to_run:
        print(f"{Colors.RED}Nothing matches: {arguments.filter}{Colors.RESET}")
        return 1

    catalog = engine_catalog.EngineCatalog(arguments.config)
    if catalog.missing:
        # Said once, here, rather than as one failure per test that needed an engine.
        print(f"{Colors.RED}Engines missing:{Colors.RESET}")
        for line in catalog.missing:
            print(f"  {Colors.RED}- {line}{Colors.RESET}")
        print(f"{Colors.YELLOW}Build the project, and put Qapla and SpikeEngine into "
              f"{engine_catalog.REPO_ROOT / 'engines'}.{Colors.RESET}")
        return 1

    cumulative = load_previous_results()
    if arguments.skip_passed:
        before = len(tests_to_run)
        tests_to_run = [t for t in tests_to_run
                        if verdict_of(cumulative.get(t["name"], "")) != "PASSED"]
        if before != len(tests_to_run):
            print(f"{Colors.YELLOW}Skipping {before - len(tests_to_run)} tests that already "
                  f"passed.{Colors.RESET}")
        if not tests_to_run:
            print(f"{Colors.GREEN}Nothing left to do.{Colors.RESET}")
            return 0

    fingerprint_before = real_config_fingerprint()

    print()
    print(f"{Colors.CYAN}{'=' * 46}{Colors.RESET}")
    print(f"{Colors.CYAN}  Qapla Chess GUI - Integration Tests (HTTP){Colors.RESET}")
    print(f"{Colors.CYAN}{'=' * 46}{Colors.RESET}")
    print()
    print(f"{Colors.YELLOW}Tests to run: {len(tests_to_run)}   "
          f"Engines: {', '.join(catalog.names())}{Colors.RESET}")

    results: List[Tuple[str, bool, float]] = []
    for test in tests_to_run:
        passed, runtime = invoke_test(
            test,
            catalog,
            build_config=arguments.config,
            keep_sandbox=arguments.keep_sandbox,
            timeout_scale=arguments.timeout_scale,
        )
        results.append((test["name"], passed, runtime))
        cumulative[test["name"]] = \
            f"{'PASSED' if passed else 'FAILED'} ({format_duration(runtime)})"
        save_results(cumulative)

    passed_count = sum(1 for _, passed, _ in results if passed)
    failed_count = len(results) - passed_count
    total_runtime = sum(runtime for _, _, runtime in results)

    print()
    print(f"{Colors.CYAN}{'=' * 46}{Colors.RESET}")
    print(f"{Colors.CYAN}  Results{Colors.RESET}")
    print(f"{Colors.CYAN}{'=' * 46}{Colors.RESET}")
    print()
    for name, passed, runtime in results:
        mark = f"{Colors.GREEN}[PASS]" if passed else f"{Colors.RED}[FAIL]"
        print(f"{mark} {name} ({format_duration(runtime)}){Colors.RESET}")

    if len(results) > 3:
        print()
        print(f"{Colors.CYAN}Slowest tests:{Colors.RESET}")
        for name, _, runtime in sorted(results, key=lambda r: r[2], reverse=True)[:3]:
            print(f"{Colors.GRAY}  {format_duration(runtime):>10}  {name}{Colors.RESET}")

    print()
    print(f"{Colors.YELLOW}Total: {len(results)} | Passed: {passed_count} | "
          f"Failed: {failed_count} | Runtime: {format_duration(total_runtime)}{Colors.RESET}")

    fingerprint_after = real_config_fingerprint()
    if fingerprint_after != fingerprint_before:
        print()
        print(f"{Colors.RED}The suite changed the configuration directory you work with. "
              f"That must not happen -- every session is started with --config-dir.{Colors.RESET}")
        return 1
    print(f"{Colors.GRAY}Your own configuration directory is unchanged.{Colors.RESET}")

    print()
    if failed_count == 0:
        print(f"{Colors.GREEN}All tests passed!{Colors.RESET}")
        return 0
    print(f"{Colors.RED}{failed_count} test(s) failed{Colors.RESET}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
