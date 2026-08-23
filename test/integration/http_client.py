#!/usr/bin/env python3
"""The remote control of a running Qapla Chess GUI, as seen from a test.

Nothing here knows what a test is. It turns the endpoints of
``src/llm/remote-control-server.cpp`` into methods and returns their parsed answers, so the
layers above can talk about tournaments instead of about HTTP.

Standard library only, on purpose: the suite has to run on three operating systems and on a CI
image nobody prepared for it, and ``urllib`` is always there while ``requests`` is not.
"""

import json
import urllib.error
import urllib.request
from typing import Any, Dict, List, Optional


class RemoteControlError(RuntimeError):
    """A request the GUI refused, or could not be made at all."""


class RemoteControl:
    """Talks to one running GUI on 127.0.0.1."""

    def __init__(self, port: int, token: str = "", timeout: float = 30.0):
        self.port = port
        self.token = token
        self.timeout = timeout

    # -- plumbing ---------------------------------------------------------

    def _url(self, path: str) -> str:
        return f"http://127.0.0.1:{self.port}{path}"

    def _request(
        self,
        path: str,
        method: str = "GET",
        body: Optional[Dict[str, Any]] = None,
        timeout: Optional[float] = None,
        with_token: bool = True,
    ) -> Dict[str, Any]:
        """Performs one request and returns the parsed JSON body.

        A 401 or a 404 is an answer, not a crash: tests check for both. Only a request that could
        not be made at all raises.
        """
        data = None
        headers = {}
        if method == "POST":
            data = json.dumps(body if body is not None else {}).encode("utf-8")
            headers["Content-Type"] = "application/json"
        if with_token and self.token:
            headers["Authorization"] = f"Bearer {self.token}"

        request = urllib.request.Request(
            self._url(path), data=data, headers=headers, method=method
        )
        try:
            with urllib.request.urlopen(request, timeout=timeout or self.timeout) as response:
                return self._parse(response.status, response.read())
        except urllib.error.HTTPError as error:
            return self._parse(error.code, error.read())
        except Exception as error:  # connection refused, timeout, DNS -- all "no answer"
            raise RemoteControlError(f"{method} {path}: {error}") from error

    @staticmethod
    def _parse(status: int, raw: bytes) -> Dict[str, Any]:
        try:
            parsed = json.loads(raw.decode("utf-8", errors="replace"))
        except json.JSONDecodeError:
            parsed = {"ok": False, "content": raw.decode("utf-8", errors="replace")}
        if not isinstance(parsed, dict):
            parsed = {"ok": True, "content": parsed}
        parsed["status"] = status
        return parsed

    # -- endpoints --------------------------------------------------------

    def health(self) -> Dict[str, Any]:
        """Answers even while the UI thread is busy, so it means "the process is alive"."""
        return self._request("/health", with_token=False)

    def tools(self) -> List[Dict[str, Any]]:
        """The published tool specs, in OpenAI function-calling shape."""
        answer = self._request("/tools")
        published = answer.get("tools", answer.get("content"))
        return published if isinstance(published, list) else []

    def tool_names(self) -> List[str]:
        names = []
        for tool in self.tools():
            function = tool.get("function", tool) if isinstance(tool, dict) else {}
            name = function.get("name")
            if name:
                names.append(name)
        return names

    def call(
        self, name: str, args: Optional[Dict[str, Any]] = None, timeout: Optional[float] = None
    ) -> Dict[str, Any]:
        """Runs one tool and answers with what the GUI actually did, not what it was asked."""
        return self._request(f"/tools/{name}", method="POST", body=args or {}, timeout=timeout)

    def status(self) -> Dict[str, Any]:
        return self._request("/status")

    def state(self) -> Dict[str, Any]:
        """What every activity is doing, as data rather than as sentences.

        The counterpart to status(): same facts, but comparable. A test that needs a number or a
        run state asks here; the sentences status() answers with are written for a language model
        and follow the GUI's language setting.
        """
        return self._request("/state")

    def wait(self, activity: str, timeout: float = 300.0) -> Dict[str, Any]:
        """Blocks until that activity stops, then reports why plus the full status.

        The HTTP timeout is given a margin over the one the server is asked for, so a server-side
        ``timeout`` verdict arrives as an answer rather than as a dead connection.
        """
        return self._request(
            f"/wait?type={activity}&timeout={int(timeout)}", timeout=timeout + 15.0
        )

    def shutdown(self) -> Dict[str, Any]:
        """Asks the application to close the way its window button would."""
        return self._request("/shutdown", method="POST")
