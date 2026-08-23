#!/usr/bin/env python3
"""One run of the GUI, started for a test and shut down again afterwards.

A session owns three things: the process, the directory it keeps its state in, and the remote
control in front of it. Everything a test needs from the running application goes through here.

Two properties are the whole reason this class exists rather than a couple of ``subprocess``
calls in each test:

* **The session starts from nothing.** ``--config-dir`` points at a directory made for this test
  alone, so there is no engine list, no window layout and no half-finished tournament left over
  from an earlier run -- and nothing this test does can reach the configuration the developer
  works with.
* **The session ends properly.** ``POST /shutdown`` runs the same shutdown the window's close
  button does, which is what writes the settings out. Killing the process would be quicker and
  would make every test that looks at what was stored meaningless.
"""

import os
import subprocess
import time
from pathlib import Path
from typing import Any, Dict, List, Optional

from http_client import RemoteControl, RemoteControlError

REPO_ROOT = Path(__file__).resolve().parents[2]

#: How long to wait for the window, the OpenGL context and the server to come up.
START_TIMEOUT = 60.0

#: How long a shutdown may take before the process is killed and the test called failed.
SHUTDOWN_TIMEOUT = 30.0

#: Fixed by default, so a test that asserts a game result gets the same game every time.
DEFAULT_SEED = 4242


class GuiStartError(RuntimeError):
    """The GUI did not come up, with whatever it managed to say before giving up."""


def resolve_binary(config: str = "default") -> Path:
    """The GUI executable of a build configuration; ``QAPLA_IT_BINARY`` overrides it."""
    override = os.getenv("QAPLA_IT_BINARY", "").strip()
    if override:
        return Path(override)
    suffix = ".exe" if os.name == "nt" else ""
    return REPO_ROOT / "build" / config / f"qapla{suffix}"


class GuiSession:
    """A running GUI with its remote control, usable as a context manager."""

    def __init__(
        self,
        config_dir: Path,
        binary: Optional[Path] = None,
        token: str = "integration",
        seed: Optional[int] = DEFAULT_SEED,
        build_config: str = "default",
        timeout_scale: float = 1.0,
    ):
        self.config_dir = Path(config_dir)
        self.binary = Path(binary) if binary else resolve_binary(build_config)
        self.token = token
        self.seed = seed
        self.timeout_scale = max(1.0, timeout_scale)
        self.process: Optional[subprocess.Popen] = None
        self.remote: Optional[RemoteControl] = None
        self.output_path = self.config_dir / "gui-output.log"
        self._output_file = None
        self.was_killed = False

    # -- lifetime ---------------------------------------------------------

    def __enter__(self) -> "GuiSession":
        self.start()
        return self

    def __exit__(self, *_exception) -> None:
        self.stop()

    @property
    def port_file(self) -> Path:
        return self.config_dir / "remote-control.port"

    def start(self) -> None:
        """Starts the process and waits until its remote control answers."""
        if not self.binary.is_file():
            raise GuiStartError(
                f"no GUI executable at {self.binary} -- build it first, or set QAPLA_IT_BINARY"
            )
        self.config_dir.mkdir(parents=True, exist_ok=True)
        # A leftover from the previous session of the same directory would otherwise be read as
        # this one's port. Restarts (persistence tests) reuse the directory on purpose.
        self.port_file.unlink(missing_ok=True)

        environment = dict(os.environ)
        if self.seed is not None:
            environment["QAPLA_DIAG_SEED"] = str(self.seed)

        self._output_file = open(self.output_path, "a", encoding="utf-8", errors="replace")
        self._output_file.write(f"\n===== start {time.strftime('%H:%M:%S')} =====\n")
        self._output_file.flush()

        self.process = subprocess.Popen(
            [
                str(self.binary),
                f"--config-dir={self.config_dir}",
                "--remote-control",
                "--remote-control-port=0",
                f"--remote-control-token={self.token}",
            ],
            cwd=str(REPO_ROOT),
            stdout=self._output_file,
            stderr=subprocess.STDOUT,
            env=environment,
        )

        port = self._await_port()
        self.remote = RemoteControl(port, self.token)
        self._await_health()

    def _await_port(self) -> int:
        """Reads the port out of the configuration directory once the server has bound it."""
        deadline = time.monotonic() + START_TIMEOUT * self.timeout_scale
        while time.monotonic() < deadline:
            if self.process and self.process.poll() is not None:
                raise GuiStartError(
                    f"the GUI ended during startup with code {self.process.returncode}\n"
                    f"{self.output_tail()}"
                )
            try:
                text = self.port_file.read_text(encoding="utf-8").strip()
                if text:
                    return int(text)
            except (FileNotFoundError, ValueError):
                pass
            time.sleep(0.05)
        self.kill()
        raise GuiStartError(
            f"no {self.port_file.name} after {START_TIMEOUT * self.timeout_scale:.0f}s\n"
            f"{self.output_tail()}"
        )

    def _await_health(self) -> None:
        deadline = time.monotonic() + START_TIMEOUT * self.timeout_scale
        last_error = ""
        while time.monotonic() < deadline:
            try:
                if self.remote and self.remote.health().get("ok"):
                    return
            except RemoteControlError as error:
                last_error = str(error)
            time.sleep(0.05)
        self.kill()
        raise GuiStartError(f"the remote control never answered ({last_error})\n{self.output_tail()}")

    def stop(self) -> None:
        """Asks the application to close and waits for it; kills it only if that fails."""
        if self.process is None:
            self._close_output()
            return
        if self.process.poll() is None:
            try:
                if self.remote:
                    self.remote.shutdown()
            except RemoteControlError:
                pass  # about to be killed anyway
            try:
                self.process.wait(timeout=SHUTDOWN_TIMEOUT * self.timeout_scale)
            except subprocess.TimeoutExpired:
                # Not silently tolerated: a GUI that will not close is a finding, and the test
                # that ran into it is told so.
                self.was_killed = True
                self.kill()
        self.process = None
        self.remote = None
        self._close_output()

    def restart(self) -> None:
        """Closes the application and starts it again on the same configuration directory.

        The point of the persistence tests: whatever the first session stored has to be there for
        the second one to find.
        """
        self.stop()
        self.was_killed = False
        self.start()

    def kill(self) -> None:
        if self.process is not None and self.process.poll() is None:
            self.process.kill()
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                pass
        self._close_output()

    def _close_output(self) -> None:
        if self._output_file is not None:
            self._output_file.close()
            self._output_file = None

    @property
    def exit_code(self) -> Optional[int]:
        return self.process.returncode if self.process else None

    def output_tail(self, lines: int = 25) -> str:
        """The last of what the GUI wrote -- the first thing anybody wants after a failure."""
        try:
            text = self.output_path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            return "(no output captured)"
        tail = text.splitlines()[-lines:]
        return "\n".join(f"    | {line}" for line in tail) if tail else "    | (empty)"

    # -- the remote control, for tests ------------------------------------

    def call(self, name: str, args: Optional[Dict[str, Any]] = None, **kwargs) -> Dict[str, Any]:
        return self._remote().call(name, args, **kwargs)

    def wait(self, activity: str, timeout: float = 300.0) -> Dict[str, Any]:
        return self._remote().wait(activity, timeout * self.timeout_scale)

    def status(self) -> Dict[str, Any]:
        return self._remote().status()

    def tool_names(self) -> List[str]:
        return self._remote().tool_names()

    def _remote(self) -> RemoteControl:
        if self.remote is None:
            raise RemoteControlError("the session is not running")
        return self.remote
