#!/usr/bin/env python3
"""Puts the engines the test suites need into ``engines/``.

    python3 scripts/provision-engines.py            # copy what is missing or out of date
    python3 scripts/provision-engines.py --check    # only say what is there and what is not
    python3 scripts/provision-engines.py --force    # copy again even if it looks current

Three engines are needed and none of them can be committed: they are build output of other
projects, and ``engines/`` is git-ignored. Where they come from differs per machine, which is
exactly the knowledge that gets lost -- a release check then fails on one host with "no
Qapla0.4.0", and whoever picks it up has to find out again where that file lives. It is written
down here instead, per platform, so provisioning a machine is one command that can be repeated.

What is needed, and why:

* ``Qapla`` and ``SpikeEngine`` -- the GUI suites play real games with them. The diagnostic
  engine moves at random, which is no use where a test waits for a move to be found.
* ``Qapla0.4.0`` -- the cross-tool tests play it against Spike, in the GUI and in
  qapla-engine-tester. A build of its own on purpose: a newer Qapla in ``engines/`` must not
  quietly change what those tests measure.
"""

import argparse
import platform
import shutil
import sys
from pathlib import Path
from typing import Dict, List, Tuple

REPO_ROOT = Path(__file__).resolve().parents[1]
TARGET_DIR = REPO_ROOT / "engines"

#: Where each engine comes from, per platform. The first source that exists is used, so a machine
#: may keep them in either of the usual places without this needing to know which.
#:
#: Windows has no build of its own for Qapla 0.4.0: the release build under Qapla2 is that
#: version there, and it is copied twice, once under each name.
SOURCES: Dict[str, Dict[str, List[Path]]] = {
    "Darwin": {
        "Qapla": [REPO_ROOT.parent / "qapla" / "build" / "Release" / "Qapla"],
        "SpikeEngine": [REPO_ROOT.parent / "engine-tester" / "test" / "integration" / "engines"
                        / "Spike1.4.1-macos"],
        "Qapla0.4.0": [REPO_ROOT.parent / "engine-tester" / "test" / "integration" / "engines"
                       / "Qapla0.4.0-macos"],
    },
    "Linux": {
        "Qapla": [Path.home() / "dev" / "qapla" / "build" / "Release" / "Qapla"],
        "SpikeEngine": [Path.home() / "dev" / "qapla-engine-tester" / "test" / "integration"
                        / "engines" / "Spike1.4.1"],
        "Qapla0.4.0": [Path.home() / "dev" / "qapla-engine-tester" / "test" / "integration"
                       / "engines" / "Qapla0.4.0"],
    },
    "Windows": {
        "Qapla.exe": [Path("C:/Development/Qapla2/build/Release/Qapla.exe")],
        "SpikeEngine.exe": [Path("C:/Development/SpikeEngine/build/release-auslieferung"
                                 "/SpikeEngine.exe")],
        "Qapla0.4.0.exe": [Path("C:/Development/Qapla2/build/Release/Qapla.exe")],
    },
}


def sources_for_this_machine() -> Dict[str, List[Path]]:
    system = platform.system()
    if system not in SOURCES:
        print(f"No engine sources are written down for {system}.", file=sys.stderr)
        raise SystemExit(2)
    return SOURCES[system]


def first_existing(candidates: List[Path]) -> Path | None:
    return next((path for path in candidates if path.is_file()), None)


def is_current(source: Path, target: Path) -> bool:
    """Whether the copy is the same file as its source, by size and modification time."""
    if not target.is_file():
        return False
    source_stat, target_stat = source.stat(), target.stat()
    return (source_stat.st_size == target_stat.st_size
            and int(source_stat.st_mtime) == int(target_stat.st_mtime))


def provision(check_only: bool, force: bool) -> int:
    TARGET_DIR.mkdir(parents=True, exist_ok=True)
    missing: List[Tuple[str, List[Path]]] = []
    copied, current = 0, 0

    for name, candidates in sorted(sources_for_this_machine().items()):
        target = TARGET_DIR / name
        source = first_existing(candidates)
        if source is None:
            missing.append((name, candidates))
            print(f"  MISSING  {name}: no source found")
            continue

        if is_current(source, target) and not force:
            current += 1
            print(f"  current  {name}  <- {source}")
            continue

        if check_only:
            missing.append((name, candidates))
            print(f"  OUTDATED {name}  <- {source}")
            continue

        # copy2 keeps the modification time, which is what is_current() compares next time.
        shutil.copy2(source, target)
        target.chmod(target.stat().st_mode | 0o111)
        copied += 1
        print(f"  copied   {name}  <- {source}")

    if missing:
        print()
        print("Not provisioned:")
        for name, candidates in missing:
            print(f"  {name}, looked in:")
            for candidate in candidates:
                print(f"    {candidate}")
        print()
        print("Build the project it comes from, or put the file at one of those paths. The test "
              "suites stop before their first test while one is missing -- see "
              "test/integration/engines.py.")
        return 1

    print()
    print(f"{TARGET_DIR}: {copied} copied, {current} already current.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Put the engines the tests need into engines/")
    parser.add_argument("--check", action="store_true",
                        help="report what is there and what is missing, copy nothing")
    parser.add_argument("--force", action="store_true",
                        help="copy again even where the file looks current")
    arguments = parser.parse_args()

    print(f"Engines for {platform.system()} -> {TARGET_DIR}")
    return provision(check_only=arguments.check, force=arguments.force)


if __name__ == "__main__":
    raise SystemExit(main())
