#!/usr/bin/env python3
"""Which engine binaries the suite uses, and where they come from.

Two kinds, and the difference matters:

* The **diagnostic engines** are build output (``diagnostic-engine/``). They exist on every
  platform, are rebuilt with the GUI, and their three broken variants are the only dependable way
  to test what happens when an engine will not start, hangs, or loses on time. With
  ``QAPLA_DIAG_SEED`` they also play the same game every time.
* The **real engines** (Qapla, Spike) are somebody's builds. They live in ``engines/`` next to the
  repository, which is git-ignored, and are copied into this suite's own ``engines/`` directory so
  that a run is self-contained and the platform difference sits in one place.

Missing real engines are reported once, up front, by the runner -- not as a wall of failures in
the middle of a run.
"""

import os
import platform
import shutil
import subprocess
from pathlib import Path
from typing import Dict, List, Optional

REPO_ROOT = Path(__file__).resolve().parents[2]
SUITE_ENGINES_DIR = Path(__file__).resolve().parent / "engines"

#: Catalog names the tests use. Kept short and stable: they appear in test bodies and in the
#: engine catalog of every sandbox.
DIAG = "Diag A"
DIAG_B = "Diag B"
DIAG_NOINIT = "Diag NoInit"
DIAG_LOOP = "Diag Loop"
DIAG_LOSSONTIME = "Diag LossOnTime"
QAPLA = "Qapla"
SPIKE = "Spike"

#: The engine the cross-tool tests play with, and the one qapla-engine-tester is given as well.
#: A build of its own rather than QAPLA, so that both tools are demonstrably driving the same
#: binary and a newer Qapla in engines/ cannot quietly change what those tests measure.
QAPLA_040 = "Qapla 0.4.0"


def platform_suffix() -> str:
    """``windows`` / ``linux`` / ``macos`` -- the same three words the engine tester uses."""
    system = platform.system()
    if system == "Windows":
        return "windows"
    if system == "Darwin":
        return "macos"
    return "linux"


def executable_suffix() -> str:
    return ".exe" if platform_suffix() == "windows" else ""


def build_bin_dir(config: str) -> Path:
    """Where the build puts the diagnostic engines."""
    return REPO_ROOT / "build" / config / "bin"


def _diagnostic_source(config: str, variant: str) -> Path:
    base = "diagnostic-engine" if not variant else f"diagnostic-engine-{variant}"
    return build_bin_dir(config) / f"{base}{executable_suffix()}"


def _real_engine_source(file_stem: str) -> Optional[Path]:
    """Looks for a real engine, first in this suite's directory, then in the developer's.

    ``engines/`` beside the repository is where the binaries actually are on a developer machine
    and is already git-ignored; this suite's own directory is where a run wants them, so that the
    set of engines a test saw is visible in one place afterwards.
    """
    name = f"{file_stem}{executable_suffix()}"
    for candidate in (SUITE_ENGINES_DIR / name, REPO_ROOT / "engines" / name):
        if candidate.is_file():
            return candidate
    return None


def _clear_quarantine(path: Path) -> None:
    """Removes the macOS quarantine flag, which would otherwise refuse the first start."""
    if platform_suffix() != "macos":
        return
    subprocess.run(
        ["xattr", "-d", "com.apple.quarantine", str(path)],
        check=False,
        capture_output=True,
    )


def _materialize(source: Path, target_name: Optional[str] = None) -> Path:
    """Copies one engine into this suite's engines directory and returns the copy.

    Copied rather than referenced so that a run does not depend on a build directory that the next
    ``cmake --build`` replaces underneath it, and so that ``engines/`` after a run shows exactly
    what was played with.

    ``target_name`` gives the copy a name of its own. That is what makes two catalog entries out
    of one binary: the GUI identifies an engine by its executable, so a second sparring partner
    has to be a second file. The diagnostic engine reads its mode from its own filename, and
    anything without a known mode word in it runs as the ordinary, playing one.
    """
    SUITE_ENGINES_DIR.mkdir(parents=True, exist_ok=True)
    target = SUITE_ENGINES_DIR / (target_name or source.name)
    if not target.exists() or source.stat().st_mtime > target.stat().st_mtime:
        shutil.copy2(source, target)
        os.chmod(target, 0o755)
        _clear_quarantine(target)
    return target


def surviving_engine_processes() -> List[str]:
    """Engines from this suite's directory that are still running.

    A leak detector, and it has caught one: an engine orphaned by a killed GUI does not idle, it
    sits at a full core. One went unnoticed for an hour and a quarter and was the real reason a
    run took half again as long as the one before it and started timing out.
    """
    if platform.system() == "Windows":
        return []  # tasklist gives no command line; not worth a WMI dependency for a warning
    try:
        listing = subprocess.run(["ps", "-Ao", "pid=,command="], capture_output=True, text=True,
                                 check=False, timeout=10).stdout
    except (OSError, subprocess.SubprocessError):
        return []
    marker = str(SUITE_ENGINES_DIR)
    return [line.strip() for line in listing.splitlines() if marker in line]


class EngineCatalog:
    """The engine binaries available to a run, by the catalog name the tests use."""

    def __init__(self, config: str = "default"):
        self.config = config
        self._paths: Dict[str, Path] = {}
        self.missing: List[str] = []
        self._collect()

    def _collect(self) -> None:
        # (catalog name, build variant, filename of the copy). Diag A and Diag B are the same
        # program under two names, which is what a tournament needs: two opponents.
        diagnostics = (
            (DIAG, "", f"diagnostic-engine-a{executable_suffix()}"),
            (DIAG_B, "", f"diagnostic-engine-b{executable_suffix()}"),
            (DIAG_NOINIT, "noinit", None),
            (DIAG_LOOP, "loop", None),
            (DIAG_LOSSONTIME, "lossontime", None),
        )
        for name, variant, target_name in diagnostics:
            source = _diagnostic_source(self.config, variant)
            if source.is_file():
                self._paths[name] = _materialize(source, target_name)
            else:
                self.missing.append(f"{name}: no {source} (build the project first)")

        for name, stem in ((QAPLA, "Qapla"), (SPIKE, "SpikeEngine"), (QAPLA_040, "Qapla0.4.0")):
            source = _real_engine_source(stem)
            if source is not None:
                self._paths[name] = _materialize(source)
            else:
                self.missing.append(
                    f"{name}: no {stem}{executable_suffix()} in "
                    f"{SUITE_ENGINES_DIR} or {REPO_ROOT / 'engines'}"
                )

    def has(self, name: str) -> bool:
        return name in self._paths

    def path(self, name: str) -> Path:
        if name not in self._paths:
            raise KeyError(
                f"engine '{name}' is not available; the runner reports what is missing at startup"
            )
        return self._paths[name]

    def names(self) -> List[str]:
        return sorted(self._paths)
