#!/usr/bin/env python3
"""Session tests -- the channel itself, before any chess happens.

If these fail, nothing else in the suite means anything: they check that a GUI started for a test
comes up, answers, refuses what it should refuse, and goes away again cleanly.
"""

from typing import Any, Dict, List

EXPECTED_TOOLS = [
    "start",
    "stop",
    "get_status",
    "clear_result",
    "save_results",
    "load_results",
    "configure_tournament",
    "configure_sprt",
    "configure_epd",
    "configure_clop",
    "manage_engines",
]


def _publishes_the_expected_tools(session, _results):
    """The tool list is the contract every other test in this suite is written against."""
    published = session.tool_names()
    missing = [name for name in EXPECTED_TOOLS if name not in published]
    if missing:
        return False, f"tools missing from /tools: {', '.join(missing)}"
    return True, f"/tools publishes all {len(EXPECTED_TOOLS)} expected tools"


def _keeps_local_only_tools_to_itself(session, _results):
    """What is meant for the person at the window must not be offered down the wire."""
    published = session.tool_names()
    leaked = [name for name in ("close_application", "open_pgn_file") if name in published]
    if leaked:
        return False, f"local-only tools published remotely: {', '.join(leaked)}"
    return True, "local-only tools are not published remotely"


def _starts_with_an_empty_catalog(session, _results):
    """A fresh configuration directory has to mean a fresh installation, or nothing is isolated."""
    answer = session.call("manage_engines", {"command": "list"})
    text = str(answer.get("content", ""))
    for name in ("Qapla", "Spike", "Diag"):
        if name in text:
            return False, f"the catalog of a fresh sandbox already knows '{name}': {text[:200]}"
    return True, "a fresh sandbox starts with no engines installed"


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "session-starts",
            "description": "The GUI comes up, publishes its tools and shuts down again",
            "steps": [],
            "validators": [
                {"type": "custom", "check": _publishes_the_expected_tools},
                {"type": "custom", "check": _keeps_local_only_tools_to_itself},
            ],
        },
        {
            "name": "session-empty-sandbox",
            "description": "A test's configuration directory starts as a fresh installation",
            "validators": [
                {"type": "custom", "check": _starts_with_an_empty_catalog},
            ],
        },
        {
            "name": "session-health-needs-no-token",
            "description": "/health answers without the token, so liveness can always be checked",
            "steps": [
                {"raw": "/health", "with_token": False, "expect_status": 200, "id": "health"},
            ],
            "validators": [
                {"type": "httpStatus", "step": "health", "expected": 200},
                {"type": "ok", "step": "health"},
            ],
        },
        {
            "name": "session-tools-need-the-token",
            "description": "Everything but /health is refused without the shared secret",
            "steps": [
                {"raw": "/tools", "with_token": False, "expect_status": 401, "id": "tools"},
                {"raw": "/status", "with_token": False, "expect_status": 401, "id": "status"},
                {"raw": "/shutdown", "method": "POST", "with_token": False,
                 "expect_status": 401, "id": "shutdown"},
            ],
            "validators": [
                {"type": "httpStatus", "step": "tools", "expected": 401},
                {"type": "httpStatus", "step": "status", "expected": 401},
                {"type": "httpStatus", "step": "shutdown", "expected": 401},
            ],
        },
        {
            "name": "session-unknown-tool",
            "description": "A tool that does not exist is a wrong address, not a failed call",
            "steps": [
                {"raw": "/tools/no_such_tool", "method": "POST", "body": {},
                 "expect_status": 404, "id": "unknown"},
            ],
            "validators": [
                {"type": "httpStatus", "step": "unknown", "expected": 404},
                {"type": "content", "step": "unknown", "pattern": "No such tool"},
            ],
        },
        {
            "name": "session-local-only-tool-is-refused",
            "description": "close_application exists, but not for a remote caller",
            "steps": [
                {"call": "close_application", "expect_ok": False, "id": "close"},
            ],
            "validators": [
                {"type": "failed", "step": "close"},
                {"type": "content", "step": "close", "pattern": "inside the application window"},
            ],
        },
        {
            "name": "session-status-when-idle",
            "description": "The overview answers even when nothing is running",
            "steps": [
                {"status": True, "id": "overview"},
            ],
            "validators": [
                {"type": "ok", "step": "overview"},
            ],
        },
    ]
