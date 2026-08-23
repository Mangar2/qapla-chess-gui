#!/usr/bin/env python3
"""State tests -- the machine-readable answer, and that it agrees with the spoken one.

``GET /status`` describes what is going on in sentences, written for a language model. ``GET
/state`` reports the same facts as data: a run state out of a fixed vocabulary, whether an
activity could be started, and its result table as headers and rows.

The distinction is not cosmetic. The sentences change wording as their descriptions are reworked,
and they follow the GUI's language setting; a test that matches on them fails for reasons that
have nothing to do with the application being wrong. Everything a test wants to compare should
come from here.
"""

import sys
from pathlib import Path
from typing import Any, Dict, List

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as ec  # noqa: E402

PAIR = [ec.DIAG, ec.DIAG_B]
FAST_TC = "0.2+0.01"


def _reports_every_activity(_session, results):
    """All four run types answer, whether or not anything was ever configured for them."""
    activities = results["state"].get("activities", {})
    missing = [name for name in ("tournament", "sprt", "epd", "clop") if name not in activities]
    if missing:
        return False, f"/state says nothing about: {', '.join(missing)}"
    fields = ("state", "running", "ready_to_start")
    for name, reported in activities.items():
        absent = [field for field in fields if field not in reported]
        if absent:
            return False, f"{name} is missing {', '.join(absent)}"
    return True, "all four activities report state, running and ready_to_start"


def _state_matches_the_spoken_status(session, _results):
    """The two answers must not be able to disagree: same fact, two renderings."""
    running = session.state()["activities"]["tournament"]["running"]
    spoken = str(session.call("get_status", {"type": "tournament"}).get("content", ""))
    says_running = "A tournament is currently running" in spoken
    if running == says_running:
        return True, f"/state and /status agree that running is {running}"
    return False, f"/state says running={running}, /status says {says_running}"


def _answers_in_english_whatever_the_gui_speaks(session, _results):
    """The remote control is a machine interface and must not follow the GUI's language.

    Nothing under src/llm/ goes through the translator today, so this holds by construction. It is
    checked anyway: the day someone wraps an action's text in translate(), every test in this
    suite that reads a sentence starts failing for a reason that has nothing to do with the
    application being wrong, and this is the test that says so plainly.
    """
    spoken = str(session.call("get_status", {"type": "tournament"}).get("content", ""))
    if "No tournament is currently running" in spoken:
        return True, "the remote control answers in English with the GUI set to German"
    return False, f"the status came back in an unexpected wording: {spoken[:200]}"


def get_tests() -> List[Dict[str, Any]]:
    return [
        {
            "name": "state-reports-every-activity",
            "description": "A fresh session reports all four run types as idle and not running",
            "steps": [
                {"state": True, "id": "state"},
            ],
            "validators": [
                {"type": "custom", "check": _reports_every_activity},
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "state", "expected": "idle"},
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "running", "expected": False},
                {"type": "stateField", "step": "state", "activity": "sprt",
                 "field": "running", "expected": False},
                # Nothing is configured yet, so nothing could be started.
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "ready_to_start", "expected": False},
            ],
        },
        {
            "name": "state-knows-when-a-run-could-start",
            "description": "Configuring a tournament flips ready_to_start, without starting it",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament",
                 "args": {"engines": PAIR, "games": 2, "time_control": FAST_TC,
                          "openings_file": "{openings}"}},
                {"state": True, "id": "state"},
            ],
            "validators": [
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "ready_to_start", "expected": True},
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "running", "expected": False},
                # Configuring one activity says nothing about another.
                {"type": "stateField", "step": "state", "activity": "sprt",
                 "field": "ready_to_start", "expected": False},
            ],
        },
        {
            "name": "state-reports-a-run-while-it-is-going",
            "description": "A running tournament is reported as running, and as running only",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament",
                 "args": {"engines": PAIR, "games": 200, "time_control": "2+0.1",
                          "openings_file": "{openings}"}},
                {"call": "start", "args": {"type": "tournament"}},
                {"state": True, "id": "state"},
                {"call": "stop", "args": {"type": "tournament"}},
                {"wait": "tournament", "timeout": 120},
            ],
            "validators": [
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "running", "expected": True},
                {"type": "stateField", "step": "state", "activity": "sprt",
                 "field": "running", "expected": False},
            ],
        },
        {
            "name": "state-reports-the-standings-as-data",
            "description": "A finished tournament's table comes back as headers and rows",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament",
                 "args": {"engines": PAIR, "games": 2, "time_control": FAST_TC,
                          "openings_file": "{openings}"}},
                {"call": "start", "args": {"type": "tournament"}},
                {"wait": "tournament", "timeout": 120},
                {"state": True, "id": "state"},
            ],
            "validators": [
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "running", "expected": False},
                # Two engines, two rows -- and every game accounted for. This is the assertion
                # the prose version could only approximate with a regular expression.
                {"type": "resultRows", "step": "state", "activity": "tournament", "count": 2},
                {"type": "resultCell", "step": "state", "activity": "tournament",
                 "row": 0, "column": "Total", "expected": "2"},
                {"type": "resultCell", "step": "state", "activity": "tournament",
                 "row": 1, "column": "Total", "expected": "2"},
            ],
        },
        {
            "name": "state-agrees-with-the-spoken-status",
            "description": "The data answer and the sentence answer report the same fact",
            "engines": PAIR,
            "steps": [
                {"call": "configure_tournament",
                 "args": {"engines": PAIR, "games": 200, "time_control": "2+0.1",
                          "openings_file": "{openings}"}},
                {"call": "start", "args": {"type": "tournament"}},
                {"call": "stop", "args": {"type": "tournament"}},
                {"wait": "tournament", "timeout": 120},
            ],
            "validators": [
                {"type": "custom", "check": _state_matches_the_spoken_status},
            ],
        },
        {
            "name": "state-answers-in-english-with-a-german-gui",
            "description": "The remote control does not follow the GUI's language setting",
            "language": "deu",
            "steps": [
                {"state": True, "id": "state"},
            ],
            "validators": [
                # The data answer cannot be translated at all -- these are the words of the
                # contract, not a rendering of them.
                {"type": "stateField", "step": "state", "activity": "tournament",
                 "field": "state", "expected": "idle"},
                {"type": "custom", "check": _answers_in_english_whatever_the_gui_speaks},
            ],
        },
        {
            "name": "state-is-not-a-published-tool",
            "description": "get_state serves the endpoint; it is not offered to a model",
            "steps": [
                {"state": True, "id": "state"},
            ],
            "validators": [
                {"type": "custom",
                 "check": lambda session, _results: (
                     "get_state" not in session.tool_names(),
                     "get_state is published in /tools" if "get_state" in session.tool_names()
                     else "get_state is callable but not published")},
            ],
        },
    ]
