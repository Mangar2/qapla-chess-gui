#!/usr/bin/env python3
"""Where the GUI keeps its state when nobody passes ``--config-dir``.

The one place in the suite that has to know the real directory -- and it only wants to know so
that it can prove nothing was written there. Mirrors
``QaplaHelpers::OsHelpers::defaultConfigDirectory()`` (``src/os-helpers.cpp``,
``src/os-helpers-apple.cpp``).
"""

import os
from pathlib import Path


def real_config_directory() -> Path:
    if os.name == "nt":
        local_app_data = os.environ.get("LOCALAPPDATA", ".")
        return Path(local_app_data) / "qapla-chess-gui"
    return Path(os.path.expanduser("~")) / ".qapla-chess-gui"
