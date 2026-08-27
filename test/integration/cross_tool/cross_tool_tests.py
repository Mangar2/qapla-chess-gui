#!/usr/bin/env python3
"""One run, carried between the GUI and qapla-engine-tester.

Both programs write the same tournament and SPRT files, and the promise that gives a user is that
a run started in one can be finished in the other. Nothing else here checks that promise: the GUI
suite drives only the GUI, the engine tester's suite only the command line, and a format that both
sides read separately can still drift apart in what it means.

So each test plays one stretch in the GUI, hands the file over, plays the next stretch on the
command line, hands it back, and plays a third in the GUI -- adding games every time and checking
that the earlier ones survived the trip. Real engines and real time control, because a handover
that only works for a game nobody played is worth nothing.

They need qapla-engine-tester, which is a different repository. Where it is not built, the tests
are not offered rather than failed -- see get_tests().
"""

import re
import subprocess
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Tuple

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import engines as engine_catalog  # noqa: E402  (needs the path above)

#: What the engine tester's binary is called, per platform.
CLI_NAME = "qapla-engine-tester.exe" if sys.platform == "win32" else "qapla-engine-tester"


def _find_cli() -> Path:
    """The engine tester's release build, wherever this machine keeps it.

    Beside this repository, under either of the two names the checkouts use -- ``engine-tester``
    on the development machine, ``qapla-engine-tester`` on the Linux and Windows hosts. Returns
    the first that exists, and otherwise the first candidate, so that the message naming what is
    missing names a plausible place rather than nothing.
    """
    candidates = [engine_catalog.REPO_ROOT.parent / directory / "build" / "release" / CLI_NAME
                  for directory in ("engine-tester", "qapla-engine-tester")]
    return next((path for path in candidates if path.is_file()), candidates[0])


#: The delivery ships the two together, so a machine that has one usually has the other; a
#: machine that has not is simply told so once.
CLI = _find_cli()

#: Ten games at a time, on both sides. At one, a stretch of this time control takes long enough
#: that nobody would run the suite; what is being tested is the handover, not the wait.
CONCURRENCY = 10

#: Ten seconds each plus a tenth per move -- engines that really search, games that really end.
TIME_CONTROL = "10+0.1"

#: Spike is the baseline, Qapla 0.4.0 the challenger and gauntlet engine. A build of its own, so
#: that a newer Qapla in engines/ cannot quietly change what these tests play.
BASELINE = engine_catalog.SPIKE
CHALLENGER = engine_catalog.QAPLA_040
PAIR = [BASELINE, CHALLENGER]

#: How far the SPRT run is taken in each hand. A verdict cannot be the handover point: these two
#: engines are close enough that a band ten Elo wide stayed undecided over 176 games -- measured,
#: nine minutes at this concurrency. The game limit is what makes the test end, and each stretch
#: raises it so the next tool has something left to play.
SPRT_STAGES = (20, 40, 60)


def _run_cli(arguments: List[str], where: Path, minutes: float = 30.0) -> Tuple[int, str]:
    """Runs qapla-engine-tester in the sandbox and returns its exit code with its output.

    The code, not a verdict, because the two runs here read it differently: for a tournament it
    means what it usually means, but an SPRT test that ends undecided also ends non-zero -- and
    an undecided test is exactly what this suite plays, since a verdict between these two engines
    takes hundreds of games. There, the file says whether the run did its work.
    """
    finished = subprocess.run([str(CLI), *arguments], cwd=str(where), capture_output=True,
                              text=True, errors="replace", timeout=minutes * 60, check=False)
    return finished.returncode, finished.stdout + finished.stderr


def _rounds_in(state_file: Path) -> List[int]:
    """Games per round, read out of a saved tournament or SPRT file.

    The file is what the two tools hand each other, so it is what these tests look at. Each round
    played is one ``[round]`` section, and its ``games=`` value carries one character per game --
    ``games=00`` is a round of two. Whether the games were played by the GUI or by the command
    line is not written down anywhere, and does not need to be: what matters is that a stretch
    added its games without losing the ones already there.
    """
    if not state_file.is_file():
        return []
    text = state_file.read_text(encoding="utf-8", errors="replace")
    return [len(match.group(1).strip())
            for section in text.split("[round]")[1:]
            for match in [re.search(r"^games=(.*)$", section, re.MULTILINE)] if match]


def _sprt_decided(session) -> bool:
    """Whether the SPRT test has reached a verdict rather than run out of games.

    Only used to excuse a stretch that played nothing: once a hypothesis is accepted there is
    nothing left to play, and demanding new games would fail a test for succeeding early.
    """
    answer = session.call("get_status", {"type": "sprt"})
    return "accepted" in str(answer.get("content", ""))


def _tournament_one_round_per_tool(session, _results) -> Tuple[bool, str]:
    """Round one in the GUI, round two on the command line, round three in the GUI again.

    Checked in the handed-over file at every step: a round appears where the tool that just ran
    played it, and the rounds played before it are still in there.
    """
    sandbox = session.config_dir
    state = sandbox / "handover.qtour"

    session.call("configure_tournament", {
        "engines": PAIR, "games": 2, "rounds": 1, "time_control": TIME_CONTROL,
        "concurrency": CONCURRENCY, "openings_file": str(sandbox / "openings.pgn"),
        "pgn_file": str(sandbox / "handover.pgn"),
    })
    session.call("start", {"type": "tournament"})
    if session.wait("tournament", 900).get("reason") != "finished":
        return False, "round 1 did not finish in the GUI"

    # Raised before saving, so the file handed to the command line asks for a round that is not
    # played yet. A file that were already complete would prove nothing about continuing one.
    session.call("configure_tournament", {"rounds": 2})
    saved = session.call("save_results", {"type": "tournament", "file": str(state)})
    if not saved.get("ok"):
        return False, f"the GUI did not write {state.name}: {saved.get('content')}"
    after_gui = _rounds_in(state)
    if after_gui != [2]:
        return False, f"the GUI's file holds rounds of {after_gui} games, expected one of 2"

    started = time.monotonic()
    code, output = _run_cli([f"--concurrency={CONCURRENCY}", "--tournament", "type=round-robin",
                             f"file={state.name}"], sandbox)
    if code != 0:
        return False, (f"qapla-engine-tester would not continue the GUI's file (exit {code}):"
                       f"\n{output[-800:]}")
    after_cli = _rounds_in(state)
    if after_cli != [2, 2]:
        return False, (f"after the command line the file holds rounds of {after_cli} games, "
                       f"expected two of 2 -- it did not add round 2 to round 1:\n{output[-800:]}")
    on_the_command_line = time.monotonic() - started

    loaded = session.call("load_results", {"type": "tournament", "file": str(state)})
    if not loaded.get("ok"):
        return False, f"the GUI would not read the file back: {loaded.get('content')}"

    session.call("configure_tournament", {"rounds": 3})
    session.call("start", {"type": "tournament"})
    if session.wait("tournament", 900).get("reason") != "finished":
        return False, "round 3 did not finish in the GUI"

    # Saved again because the third round is in the GUI's hands, not yet in the file -- and a
    # handover that could not be handed on once more would be half a handover.
    saved = session.call("save_results", {"type": "tournament", "file": str(state)})
    if not saved.get("ok"):
        return False, f"the GUI did not write the finished tournament: {saved.get('content')}"
    at_the_end = _rounds_in(state)
    if at_the_end != [2, 2, 2]:
        return False, (f"at the end the file holds rounds of {at_the_end} games, expected three "
                       f"of 2 -- the GUI did not continue what the command line had played")

    return True, (f"three rounds of two games in one file: round 1 in the GUI, round 2 in "
                  f"qapla-engine-tester ({on_the_command_line:.0f}s), round 3 in the GUI again, "
                  f"each tool keeping what the other had played")


def _sprt_continued_in_both(session, _results) -> Tuple[bool, str]:
    """One SPRT test in three hands, its Elo bounds raised at every handover."""
    sandbox = session.config_dir
    state = sandbox / "handover.qsprt"
    first_limit, second_limit, third_limit = SPRT_STAGES

    session.call("configure_sprt", {
        "champion": BASELINE, "challenger": CHALLENGER,
        "elo0": 0, "elo1": 10, "max_games": first_limit,
        "time_control": TIME_CONTROL, "concurrency": CONCURRENCY,
        "openings_file": str(sandbox / "openings.pgn"),
        "pgn_file": str(sandbox / "handover.pgn"),
    })
    session.call("start", {"type": "sprt"})
    if session.wait("sprt", 1800).get("reason") != "finished":
        return False, "the first stretch did not come to an end in the GUI"

    saved = session.call("save_results", {"type": "sprt", "file": str(state)})
    if not saved.get("ok"):
        return False, f"the GUI did not write {state.name}: {saved.get('content')}"
    after_gui = sum(_rounds_in(state))
    if after_gui == 0:
        return False, "the GUI's file holds no games at all"

    # Its exit code is not the question here: a test that runs out of games without accepting a
    # hypothesis ends non-zero, and that is the ordinary outcome at these bounds. What is asked
    # of the command line is that it played on in the GUI's file, and the file answers that.
    code, output = _run_cli([f"--concurrency={CONCURRENCY}", "--sprt", f"file={state.name}",
                             "eloh0=10", "eloh1=20", f"maxgames={second_limit}"], sandbox)
    after_cli = sum(_rounds_in(state))
    if after_cli <= after_gui:
        return False, (f"the command line added no games (exit {code}): {after_gui} in the file "
                       f"before, {after_cli} after\n{output[-800:]}")

    loaded = session.call("load_results", {"type": "sprt", "file": str(state)})
    if not loaded.get("ok"):
        return False, f"the GUI would not read the file back: {loaded.get('content')}"

    session.call("configure_sprt", {"elo0": 20, "elo1": 30, "max_games": third_limit})
    session.call("start", {"type": "sprt"})
    if session.wait("sprt", 1800).get("reason") != "finished":
        return False, "the last stretch did not come to an end in the GUI"

    saved = session.call("save_results", {"type": "sprt", "file": str(state)})
    if not saved.get("ok"):
        return False, f"the GUI did not write the finished test: {saved.get('content')}"
    at_the_end = sum(_rounds_in(state))
    if at_the_end <= after_cli and not _sprt_decided(session):
        return False, (f"the GUI added no games after reading the file back, and the test has "
                       f"not been decided either: {after_cli} games in the file before and after")

    return True, (f"one SPRT test in three hands, all its games in one file: {after_gui} played "
                  f"in the GUI at 0/10, {after_cli - after_gui} in qapla-engine-tester at 10/20, "
                  f"{at_the_end - after_cli} in the GUI again at 20/30")


def get_tests() -> List[Dict[str, Any]]:
    if not CLI.is_file():
        print(f"  {engine_catalog.__name__}: no qapla-engine-tester at {CLI} -- "
              f"the cross-tool tests are not offered")
        return []
    return [
        {
            "name": "crosstool-tournament-round-per-tool",
            "description": "A tournament plays one round in each tool, through the same file",
            "engines": PAIR,
            "slow": True,
            "steps": [],
            "validators": [{"type": "custom", "check": _tournament_one_round_per_tool}],
        },
        {
            "name": "crosstool-sprt-continued-in-both",
            "description": "One SPRT test changes hands twice, its Elo bounds raised each time",
            "engines": PAIR,
            "slow": True,
            "steps": [],
            "validators": [{"type": "custom", "check": _sprt_continued_in_both}],
        },
    ]
