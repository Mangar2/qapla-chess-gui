#!/usr/bin/env python3
"""Runs one integration test against a freshly started GUI.

The shape is the engine tester's (``extern/qapla-engine-tester/test/integration``): a test is a
dictionary, a module publishes them through ``get_tests()``, and the runner collects the modules.
What a test *is* differs, and that is the reason this is a second framework rather than a reuse
of the first: there, a test is one process invocation and the evidence is the files it left
behind. Here, a test is a conversation with a program that stays running, and the evidence is
what that program answered while a person could have been watching it.

A test looks like this::

    {
        "name": "tournament-basic",
        "description": "Two diagnostic engines play a short tournament to the end",
        "engines": [engines.DIAG, engines.DIAG_B],
        "steps": [
            {"call": "configure_tournament", "args": {"games": 2}, "id": "config"},
            {"call": "start", "args": {"type": "tournament"}},
            {"wait": "tournament", "timeout": 120, "id": "run"},
        ],
        "validators": [
            {"type": "waitReason", "step": "run", "expected": "finished"},
            {"type": "content", "step": "run", "pattern": "Games played: 2"},
        ],
    }

Every step must succeed unless it says otherwise (``"expect_ok": False``), because a test whose
setup quietly failed will report on a state nobody asked for.
"""

import json
import os
import re
import sys
import time
import traceback
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Tuple

import engines as engine_catalog
from gui_session import GuiSession, GuiStartError
from http_client import RemoteControlError
from sandbox import discard, fresh_sandbox


def _colours_are_usable() -> bool:
    """Whether escape codes will be shown as colour rather than as gibberish.

    Off when the output is being captured (a log file, a CI step summary) and off when NO_COLOR
    is set. Windows consoles older than the current terminal do not interpret these codes either,
    but they are not distinguishable from here -- redirect the output there and it comes out
    clean.
    """
    if os.environ.get("NO_COLOR"):
        return False
    return bool(getattr(sys.stdout, "isatty", lambda: False)())


class Colors:
    """ANSI colours, in the engine tester's palette so both suites read the same."""

    _ON = _colours_are_usable()
    GREEN = "\033[92m" if _ON else ""
    RED = "\033[91m" if _ON else ""
    YELLOW = "\033[93m" if _ON else ""
    CYAN = "\033[96m" if _ON else ""
    GRAY = "\033[90m" if _ON else ""
    RESET = "\033[0m" if _ON else ""


def format_duration(seconds: float) -> str:
    if seconds < 1:
        return f"{seconds * 1000:.0f}ms"
    if seconds < 60:
        return f"{seconds:.1f}s"
    return f"{int(seconds // 60)}m {seconds % 60:.0f}s"


def _ok(message: str) -> None:
    print(f"  {Colors.GREEN}[OK]{Colors.RESET} {message}")


def _fail(message: str) -> None:
    print(f"  {Colors.RED}[FAIL]{Colors.RESET} {message}")


def _info(message: str) -> None:
    print(f"  {Colors.GRAY}{message}{Colors.RESET}")


# ---------------------------------------------------------------------------
# Fixtures a test can refer to by name
# ---------------------------------------------------------------------------

#: Four short openings, written into every sandbox as ``openings.pgn``.
#:
#: Generated rather than committed, for a small reason and a good one: ``*.pgn`` is git-ignored
#: repository-wide, so a committed opening book would need an exception carved out for it -- and
#: a book whose content is right here in the source is easier to reason about than one that has
#: to be found and opened. Four lines is enough: a tournament of two engines over two games needs
#: openings to start from, not variety.
OPENINGS_PGN = """[Event "Integration openings"]
[Site "?"]
[Date "????.??.??"]
[Round "1"]
[White "?"]
[Black "?"]
[Result "*"]

1. e4 e5 2. Nf3 Nc6 *

[Event "Integration openings"]
[Site "?"]
[Date "????.??.??"]
[Round "2"]
[White "?"]
[Black "?"]
[Result "*"]

1. d4 d5 2. c4 e6 *

[Event "Integration openings"]
[Site "?"]
[Date "????.??.??"]
[Round "3"]
[White "?"]
[Black "?"]
[Result "*"]

1. c4 Nf6 2. Nc3 g6 *

[Event "Integration openings"]
[Site "?"]
[Date "????.??.??"]
[Round "4"]
[White "?"]
[Black "?"]
[Result "*"]

1. Nf3 d5 2. g3 Nf6 *
"""


#: Sixteen positions with a forced mate in one, written into every sandbox as ``positions.epd``.
#:
#: Generated rather than committed, because ``*.epd`` is git-ignored repository-wide -- and a
#: position set written here is easier to reason about than one that has to be found and opened.
#: Mates in one rather than quiet positions, so that a real engine finds every one of them in a
#: fraction of a second and a test can assert the number solved instead of merely that the run
#: ended. Sixteen of them rather than a handful, so that an analysis at the default concurrency
#: really has sixteen positions being worked on at once.
#:
#: Two motifs, each from both sides: a rook coming to the back rank behind a castled king, and a
#: rook mating a king boxed into the corner by the enemy king. The file the rook comes down is
#: what varies -- enough to make sixteen distinct positions, all of them mate in one.
POSITIONS_EPD = """6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - bm Ra8+; id "mate-back-rank";
6k1/5ppp/8/8/8/8/5PPP/1R4K1 w - - bm Rb8+; id "mate-rook-b8";
6k1/5ppp/8/8/8/8/5PPP/2R3K1 w - - bm Rc8+; id "mate-rook-c8";
6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - bm Rd8+; id "mate-rook-d8";
6k1/5ppp/8/8/8/8/5PPP/4R1K1 w - - bm Re8+; id "mate-rook-e8";
r5k1/5ppp/8/8/8/8/5PPP/6K1 b - - bm Ra1+; id "mate-black-a1";
1r4k1/5ppp/8/8/8/8/5PPP/6K1 b - - bm Rb1+; id "mate-black-b1";
2r3k1/5ppp/8/8/8/8/5PPP/6K1 b - - bm Rc1+; id "mate-black-c1";
3r2k1/5ppp/8/8/8/8/5PPP/6K1 b - - bm Rd1+; id "mate-black-d1";
4r1k1/5ppp/8/8/8/8/5PPP/6K1 b - - bm Re1+; id "mate-black-e1";
k7/8/1K6/8/8/8/8/7R w - - bm Rh8+; id "mate-rook-corner";
k7/8/1K6/8/8/8/8/6R1 w - - bm Rg8+; id "mate-rook-g8";
k7/8/1K6/8/8/8/8/5R2 w - - bm Rf8+; id "mate-rook-f8";
7r/8/8/8/8/1k6/8/K7 b - - bm Rh1+; id "mate-black-corner";
6r1/8/8/8/8/1k6/8/K7 b - - bm Rg1+; id "mate-black-g1";
5r2/8/8/8/8/1k6/8/K7 b - - bm Rf1+; id "mate-black-f1";
"""


def _seed_language(sandbox: Path, language: str) -> None:
    """Puts a language setting into the sandbox before the GUI reads it.

    Written as an ini section rather than set through a tool, because the language is read once at
    startup -- there is no moment after the session is up at which it could still be changed.
    Mirrors Configuration::updateLanguageConfiguration (src/configuration.cpp).
    """
    (sandbox / "qapla-chess-gui.ini").write_text(
        "[languagesettings]\nid=general\nlanguagecode=" + language + "\n", encoding="utf-8")


def _engine_key(name: str) -> str:
    """``Diag NoInit`` -> ``engine_diag_noinit``, the placeholder a test writes."""
    return "engine_" + "".join(
        character.lower() if character.isalnum() else "_" for character in name
    )


def _prepare_fixtures(sandbox: Path, catalog: engine_catalog.EngineCatalog) -> Dict[str, str]:
    """Writes what tests refer to by placeholder, and returns the substitutions.

    A test writes ``{openings}`` or ``{engine_diag_noinit}`` where it needs a path. Paths cannot
    be written literally: the sandbox is made per test and per machine, and the engines are built
    output, so a test that spelled one out would only run on the machine it was written on.
    """
    openings = sandbox / "openings.pgn"
    openings.write_text(OPENINGS_PGN, encoding="utf-8")
    positions = sandbox / "positions.epd"
    positions.write_text(POSITIONS_EPD, encoding="utf-8")
    fixtures = {
        "sandbox": str(sandbox),
        "openings": str(openings),
        "epd": str(positions),
        "pgn": str(sandbox / "games.pgn"),
        "results": str(sandbox / "results.qtour"),
        "sprt_results": str(sandbox / "results.qsprt"),
    }
    for name in catalog.names():
        fixtures[_engine_key(name)] = str(catalog.path(name))
    return fixtures


def _substitute(value: Any, mapping: Dict[str, str]) -> Any:
    """Replaces ``{name}`` placeholders anywhere in a step's arguments."""
    if isinstance(value, str):
        return value.format(**mapping) if "{" in value else value
    if isinstance(value, dict):
        return {key: _substitute(item, mapping) for key, item in value.items()}
    if isinstance(value, list):
        return [_substitute(item, mapping) for item in value]
    return value


# ---------------------------------------------------------------------------
# Engine catalog
# ---------------------------------------------------------------------------


#: Games (or positions) a test may have going at once, across everything it runs.
#:
#: The application starts at one, which is no setting to test anything at: a suite that only ever
#: ran one game at a time would never touch the scheduling, the pool, or the concurrency control
#: -- and those are where this application's hard bugs have been. Ten puts real load on all three
#: and still leaves the machine able to draw a window; sixteen, on the slower of the three hosts,
#: cost frames rather than found bugs.
#:
#: It is a budget for the test, not a number per run: a test that has a tournament and an SPRT
#: going at the same time says ``"parallel_runs": 2`` and each of them gets five, so that what is
#: actually being played stays ten either way. A test that needs another number passes
#: ``"concurrency"``, which is then used per run and not divided.
#:
#: The number is only half of it: a run needs at least this many games to have ten going at once,
#: so the runs are sized accordingly (see each module's configuration).
DEFAULT_CONCURRENCY = 10


def set_concurrency(session: GuiSession, concurrency: int) -> None:
    """Sets the same concurrency on all four run types, before the test configures anything.

    Applied first so that a later configure call in the test keeps it -- an unpassed field keeps
    its prior value -- while a test that names its own concurrency still wins.
    """
    for activity in ("tournament", "sprt", "epd", "clop"):
        answer = session.call(f"configure_{activity}", {"concurrency": concurrency})
        if not answer.get("ok"):
            raise RemoteControlError(
                f"could not set the {activity} concurrency: {answer.get('content')}")


def install_engines(session: GuiSession, catalog: engine_catalog.EngineCatalog,
                    names: List[str]) -> None:
    """Puts the engines a test needs into the sandbox's empty catalog.

    Installed through ``manage_engines`` rather than by writing an ini file, for two reasons: the
    test then does not depend on a storage format that is free to change, and installing is
    itself a piece of the application being exercised on every single test.
    """
    for name in names:
        path = catalog.path(name)
        # Detection runs synchronously inside install (see Actions::installEngines) and probes
        # the engine over both protocols. A well-behaved engine answers in well under a second;
        # the deliberately broken ones cost the full protocol timeout twice over -- measured at
        # around 45 seconds for the one that never answers at all.
        answer = session.call(
            "manage_engines",
            {"command": "install", "path": str(path), "new_name": name},
            timeout=240,
        )
        if not answer.get("ok"):
            raise RemoteControlError(
                f"could not install engine '{name}' from {path}: {answer.get('content')}"
            )


# ---------------------------------------------------------------------------
# Steps
# ---------------------------------------------------------------------------


def _run_step(session: GuiSession, step: Dict[str, Any], index: int,
              fixtures: Dict[str, str]) -> Dict[str, Any]:
    """Performs one step and returns its answer, with the step's own bookkeeping attached."""
    timeout_scale = session.timeout_scale

    if "call" in step:
        name = step["call"]
        args = _substitute(step.get("args", {}), fixtures)
        _info(f"{index}. call {name} {args if args else ''}")
        return session.call(name, args, timeout=step.get("timeout", 60) * timeout_scale)

    if "wait" in step:
        activity = step["wait"]
        seconds = step.get("timeout", 120)
        _info(f"{index}. wait for {activity} (up to {seconds}s)")
        return session.wait(activity, seconds)

    if "status" in step:
        _info(f"{index}. status")
        return session.status()

    if "state" in step:
        _info(f"{index}. state")
        return session.state()

    if "restart" in step:
        _info(f"{index}. restart the application on the same configuration directory")
        session.restart()
        return {"ok": True, "content": "restarted", "status": 200}

    if "raw" in step:
        # For the tests that are about the channel itself rather than about the GUI: a request
        # without the token, a tool that does not exist, a malformed body.
        path = step["raw"]
        method = step.get("method", "GET")
        _info(f"{index}. {method} {path}")
        return session._remote()._request(  # deliberate: only these tests need the raw layer
            path,
            method=method,
            body=step.get("body"),
            with_token=step.get("with_token", True),
        )

    raise ValueError(f"step {index} names no action: {step}")


def _check_step_expectation(step: Dict[str, Any], answer: Dict[str, Any], index: int) -> bool:
    """A step that did not do what it said it would ends the test where it went wrong."""
    expected_status = step.get("expect_status")
    if expected_status is not None and answer.get("status") != expected_status:
        _fail(f"step {index}: HTTP {answer.get('status')} (expected {expected_status})")
        return False

    if expected_status is not None:
        return True  # a status-only expectation says nothing about ok

    expect_ok = step.get("expect_ok", True)
    if bool(answer.get("ok")) != expect_ok:
        wanted = "succeed" if expect_ok else "fail"
        _fail(f"step {index}: expected the call to {wanted}: {answer.get('content')}")
        return False
    return True


# ---------------------------------------------------------------------------
# Validators
# ---------------------------------------------------------------------------


def _resolve_step(validator: Dict[str, Any], results: Dict[str, Dict[str, Any]]) -> Optional[Dict]:
    key = validator.get("step", "last")
    answer = results.get(key)
    if answer is None:
        _fail(f"validator refers to step '{key}', which no step produced")
    return answer


def _text_of(answer: Dict[str, Any]) -> str:
    """What a content validator matches against.

    Normally the ``content`` field, which is what a tool answers with. Endpoints that carry no
    such field -- ``/health`` is one -- are matched against their whole answer instead, so that a
    validator does not silently compare against an empty string.
    """
    if "content" in answer:
        return str(answer["content"])
    return json.dumps({key: value for key, value in answer.items() if key != "status"})


def _validate(validator: Dict[str, Any], session: GuiSession, sandbox: Path,
              results: Dict[str, Dict[str, Any]]) -> bool:
    kind = validator["type"]

    if kind in ("ok", "failed"):
        answer = _resolve_step(validator, results)
        if answer is None:
            return False
        succeeded = bool(answer.get("ok"))
        if succeeded == (kind == "ok"):
            _ok(f"step '{validator.get('step', 'last')}' {'succeeded' if succeeded else 'failed'}"
                " as expected")
            return True
        _fail(f"step '{validator.get('step', 'last')}': {_text_of(answer)}")
        return False

    if kind in ("content", "contentLacks"):
        answer = _resolve_step(validator, results)
        if answer is None:
            return False
        text = _text_of(answer)
        pattern = validator["pattern"]
        found = (re.search(pattern, text) is not None) if validator.get("isRegex") \
            else (pattern in text)
        if found == (kind == "content"):
            _ok(f"content {'contains' if kind == 'content' else 'does not contain'} "
                f"{pattern!r}")
            return True
        _fail(f"content does not match {pattern!r}:\n      {text[:500]}")
        return False

    if kind == "waitReason":
        answer = _resolve_step(validator, results)
        if answer is None:
            return False
        reason = answer.get("reason")
        # A list of acceptable reasons, not just one, because some of the distinctions are a
        # race and not a property. "stopped" means the wait was there to see it stop;
        # "not_running" means it was already over by the time the wait arrived. Which of the two
        # a caller gets depends on how fast the machine is, so a test that insists on one of them
        # is a test that fails on somebody else's laptop.
        expected = validator["expected"]
        acceptable = [expected] if isinstance(expected, str) else list(expected)
        if reason in acceptable:
            _ok(f"wait ended with '{reason}'")
            return True
        _fail(f"wait ended with '{reason}' (expected {' or '.join(acceptable)})")
        return False

    if kind == "httpStatus":
        answer = _resolve_step(validator, results)
        if answer is None:
            return False
        if answer.get("status") == validator["expected"]:
            _ok(f"HTTP {answer.get('status')}")
            return True
        _fail(f"HTTP {answer.get('status')} (expected {validator['expected']})")
        return False

    if kind in ("fileExists", "fileMissing"):
        path = Path(str(validator["path"]))
        path = path if path.is_absolute() else sandbox / path
        exists = path.exists()
        if exists == (kind == "fileExists"):
            _ok(f"{path.name} {'exists' if exists else 'is not there'}")
            return True
        _fail(f"{path} {'is missing' if kind == 'fileExists' else 'should not be there'}")
        return False

    if kind == "fileContent":
        path = Path(validator["path"])
        path = path if path.is_absolute() else sandbox / path
        if not path.is_file():
            _fail(f"{path} is missing")
            return False
        text = path.read_text(encoding="utf-8", errors="replace")
        pattern = validator["content"]
        found = (re.search(pattern, text) is not None) if validator.get("isRegex") \
            else (pattern in text)
        if found:
            _ok(f"{path.name} contains {pattern!r}")
            return True
        _fail(f"{path.name} does not contain {pattern!r}")
        return False

    if kind in ("stateField", "resultRows", "resultCell"):
        answer = _resolve_step(validator, results)
        if answer is None:
            return False
        activity = validator["activity"]
        activities = answer.get("activities") or {}
        if activity not in activities:
            _fail(f"/state says nothing about '{activity}'")
            return False
        reported = activities[activity]

        if kind == "stateField":
            field = validator["field"]
            expected = validator["expected"]
            actual = reported.get(field)
            if actual == expected:
                _ok(f"{activity}.{field} is {actual!r}")
                return True
            _fail(f"{activity}.{field} is {actual!r} (expected {expected!r})")
            return False

        table = reported.get("results")
        if table is None:
            _fail(f"{activity} has no result table")
            return False

        if kind == "resultRows":
            expected = validator["count"]
            actual = len(table.get("rows", []))
            if actual == expected:
                _ok(f"{activity} reports {actual} result rows")
                return True
            _fail(f"{activity} reports {actual} result rows (expected {expected})")
            return False

        headers = table.get("headers", [])
        column = validator["column"]
        if column not in headers:
            _fail(f"{activity} has no column {column!r}; it has {headers}")
            return False
        rows = table.get("rows", [])
        wanted_row = validator.get("row", 0)
        if wanted_row >= len(rows):
            _fail(f"{activity} has {len(rows)} result rows, so row {wanted_row} does not exist")
            return False
        actual = rows[wanted_row][headers.index(column)]
        expected = validator["expected"]
        if actual == expected:
            _ok(f"{activity} result [{wanted_row}].{column} is {actual!r}")
            return True
        _fail(f"{activity} result [{wanted_row}].{column} is {actual!r} (expected {expected!r})")
        return False

    if kind == "custom":
        check: Callable[[GuiSession, Dict[str, Any]], Tuple[bool, str]] = validator["check"]
        passed, message = check(session, results)
        (_ok if passed else _fail)(message)
        return passed

    _fail(f"unknown validator: {kind}")
    return False


# ---------------------------------------------------------------------------
# One test
# ---------------------------------------------------------------------------


def invoke_test(test: Dict[str, Any], catalog: engine_catalog.EngineCatalog,
                build_config: str = "default", keep_sandbox: bool = False,
                timeout_scale: float = 1.0,
                concurrency: Optional[int] = None) -> Tuple[bool, float]:
    """Starts a GUI, walks the steps, checks the validators. Returns verdict and runtime."""
    name = test["name"]
    print()
    print(f"  {Colors.CYAN}Test: {name}{Colors.RESET}")
    print(f"  {Colors.GRAY}{test.get('description', '')}{Colors.RESET}")

    sandbox = fresh_sandbox(name)
    if test.get("language"):
        _seed_language(sandbox, test["language"])
    started = time.monotonic()
    passed = True
    session: Optional[GuiSession] = None

    try:
        session = GuiSession(
            config_dir=sandbox,
            seed=test.get("seed", None if test.get("unseeded") else 4242),
            build_config=build_config,
            timeout_scale=timeout_scale,
        )
        session.start()

        needed = test.get("engines", [])
        if needed:
            install_engines(session, catalog, needed)
            _info(f"catalog: {', '.join(needed)}")

        # A test that names its own concurrency keeps it: it says so because its subject depends
        # on it, and a caller measuring load has no business overruling that. Otherwise the
        # budget is shared out between the runs the test has going at the same time.
        budget = concurrency if concurrency else DEFAULT_CONCURRENCY
        share = max(1, budget // max(1, int(test.get("parallel_runs", 1))))
        set_concurrency(session, test.get("concurrency", share))

        fixtures = _prepare_fixtures(sandbox, catalog)
        results: Dict[str, Dict[str, Any]] = {}
        for index, step in enumerate(test.get("steps", []), start=1):
            answer = _run_step(session, step, index, fixtures)
            results["last"] = answer
            if "id" in step:
                results[step["id"]] = answer
            if not _check_step_expectation(step, answer, index):
                passed = False
                break

        if passed:
            for validator in test.get("validators", []):
                if not _validate(validator, session, sandbox, results):
                    passed = False

    except (GuiStartError, RemoteControlError) as error:
        _fail(str(error))
        passed = False
    except Exception as error:  # a broken test is a failing test, not a crashed runner
        _fail(f"{type(error).__name__}: {error}")
        print(f"{Colors.GRAY}{traceback.format_exc()}{Colors.RESET}")
        passed = False
    finally:
        if session is not None:
            try:
                session.stop()
            except Exception as error:
                _fail(f"the application did not shut down cleanly: {error}")
                passed = False
            # What the UI thread did while the test drove it. A frame that takes longer than a
            # twentieth of a second is a frame in which the window is not drawing and the tool
            # queue is not being served -- so the application is answering nobody, the caller
            # included. Every one of these is work that belongs on a thread of its own.
            if session.stalls_seen:
                # The breakdown, not just the name: a frame blocked by one thing and a frame in
                # which six things were each too slow read the same without it.
                breakdown = ", ".join(f"{name} {milliseconds:.0f}ms"
                                      for name, milliseconds in session.worst_frame_sections[:6])
                message = (f"the UI thread stalled {session.stalls_seen}x, worst "
                           f"{session.worst_frame_ms:.0f} ms in {session.worst_section or '?'}"
                           + (f" -- {breakdown}" if breakdown else ""))
                if test.get("allow_ui_stalls"):
                    _info(f"[known] {message} -- {test['allow_ui_stalls']}")
                else:
                    _fail(message)
                    passed = False

            if session.was_killed:
                _fail("the application had to be killed -- it did not answer /shutdown")
                passed = False
            elif session.exit_code not in (None, 0):
                # Negative codes are signals: -6 is SIGABRT, which the GUI reaches by way of an
                # uncaught exception. macOS writes a report to ~/Library/Logs/DiagnosticReports.
                signal_note = (f" (signal {-session.exit_code})" if session.exit_code < 0 else "")
                _fail(f"the application ended with code {session.exit_code}{signal_note}")
                passed = False

    runtime = time.monotonic() - started

    if not passed and session is not None:
        print(f"  {Colors.GRAY}Last output of the GUI:{Colors.RESET}")
        print(f"{Colors.GRAY}{session.output_tail()}{Colors.RESET}")
        print(f"  {Colors.GRAY}Sandbox kept at: {sandbox}{Colors.RESET}")
    elif passed and not keep_sandbox:
        discard(sandbox)

    print()
    verdict = f"{Colors.GREEN}[PASS]{Colors.RESET}" if passed else f"{Colors.RED}[FAIL]{Colors.RESET}"
    print(f"  {verdict} {name} ({format_duration(runtime)})")
    return passed, runtime
