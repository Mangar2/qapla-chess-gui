# GUI Integration Tests

Tests that drive a **running** Qapla Chess GUI from outside the process, over the HTTP remote
control. They sit between the unit tests and the on-screen GUI tests:

| Layer | Checks | Tool |
|---|---|---|
| Unit | Logic without a GUI | Catch2, `src/test-system/unit/` |
| **Integration (here)** | Whole flows against the running application: configure, start, wait, read the result, restart | Python + HTTP |
| GUI | What only mouse and keyboard can check: board, dialogs, tutorial | ImGui Test Engine |

## Running them

```bash
python3 test/integration/test_runner.py                        # everything
python3 test/integration/test_runner.py --filter 'tournament-*'
python3 test/integration/test_runner.py --test session-starts --keep-sandbox
python3 test/integration/test_runner.py --list
python3 test/integration/test_runner.py --skip-passed          # continue after a failure
```

One entry point on all three platforms, and no wrapper scripts: Windows would need a second one
saying the same thing, and two files that have to agree eventually stop agreeing. On Linux
without a desktop session the runner starts itself again under `xvfb-run`, since the tests really
do open a window.

`--config` picks the build: `default` (debug) unless told otherwise, `release` for what ships.
Both were measured across a full run -- 52/52 in 5m27s on debug, and the same suite on release is
no faster, because what the slow tests spend their time on is protocol timeouts, not computation.

Before the first run, build the project (`cmake --build --preset default`): the GUI and the
diagnostic engines are build output. The two real engines, `Qapla` and `SpikeEngine`, are
expected in `engines/` at the top of the repository; the runner says so up front if any engine is
missing, rather than failing test by test.

The exit code is 0 only if every test passed, so a release script can gate on it.

On a busy machine, pass `--timeout-scale 3`. The tests drive a real application playing real
games, and several of its operations -- detecting an engine, starting a CLOP run -- run on the UI
thread, so under load they can outlast the timeouts that are generous when the machine is idle.
Three consecutive runs on an idle machine came out 52/52, 52/52 and, with heavy other work going
on, 51/52.

## What each test does

1. A fresh, empty configuration directory is made under `sandbox/<test name>/`.
2. The GUI is started with `--config-dir=<that> --remote-control --remote-control-port=0`, and
   the port it bound is read back from `remote-control.port` in that directory.
3. The engines the test asks for are installed through `manage_engines` — an empty catalog is
   what a fresh installation has.
4. The test's steps run: tool calls, `/wait` for a run to finish, restarts.
5. The validators check the answers and the files.
6. `POST /shutdown` closes the application the way its window button would, so that what the
   session stored is really written. A sandbox is kept when the test failed and thrown away when
   it passed.

**The UI thread is watched throughout.** A frame whose work takes longer than 50 ms — under 20
frames a second — is a frame in which the window is not drawing and the tool queue is not being
served, so the application is answering nobody. Every one of those is work that belongs on a
thread of its own. Any test whose session stalls fails, and the failure names the culprit
(`worst 43157 ms in tool:manage_engines`) rather than saying the GUI was slow.

A test that runs into a stall which is known and not yet fixed says so in one line:

```python
"allow_ui_stalls": "installing an engine detects it synchronously on the UI thread ...",
```

Those lines are the list of what is still to be fixed. There are three of them, all the same
cause. Deleting them is how the fix gets checked.

Two properties are the reason for that shape:

* **Nothing reaches your own configuration.** Every session runs out of its own directory, and the
  runner compares your real one before and after the whole run.
* **Every test starts from nothing.** No engine list, no window layout, no half-finished
  tournament from an earlier run — so a test cannot pass by accident because of what came before.

## Writing a test

A test is a dictionary; a module publishes them through `get_tests()`, and any directory holding
`<name>/<name>_tests.py` is picked up automatically.

```python
{
    "name": "tournament-basic",
    "description": "Two engines play a two-game tournament to the end",
    "engines": [ec.DIAG, ec.DIAG_B],          # installed before the steps run
    "steps": [
        {"call": "configure_tournament", "args": {"games": 2, "openings_file": "{openings}"}},
        {"call": "start", "args": {"type": "tournament"}},
        {"wait": "tournament", "timeout": 120, "id": "run"},
    ],
    "validators": [
        {"type": "waitReason", "step": "run", "expected": "finished"},
        {"type": "fileExists", "path": "games.pgn"},
    ],
}
```

**Steps** — `call` (a tool), `wait` (an activity), `status`, `restart` (close and start again on
the same directory), `raw` (an endpoint directly, for tests about the channel). Every step must
succeed unless it says `"expect_ok": False` or names an `"expect_status"`.

**Placeholders** in arguments are filled in per test: `{sandbox}`, `{openings}`, `{pgn}`,
`{results}`, `{sprt_results}`, and `{engine_diag_a}` and friends for engine executables.

**Validators** — `ok`, `failed`, `content`, `contentLacks`, `waitReason`, `httpStatus`,
`fileExists`, `fileMissing`, `fileContent`, and `custom` for a Python check that gets the session.

`content` matches the sentence a tool answers with. Those sentences are written for a language
model and will change wording; where a test needs a number rather than a phrase, that is a reason
to give the remote control a structured answer, not a reason to write a cleverer regular
expression.

## Files

| File | What it is |
|---|---|
| `test_runner.py` | Finds the modules, runs them, keeps the protocol, checks your configuration was untouched |
| `test_framework.py` | Runs one test: sandbox, session, steps, validators |
| `gui_session.py` | One GUI process and its lifetime |
| `http_client.py` | The remote control endpoints, standard library only |
| `engines.py` | Which engine binaries exist and where they come from |
| `sandbox.py` | The per-test configuration directory |
| `tests.md` | What is covered, module by module |

The layout follows `extern/qapla-engine-tester/test/integration/`, so both suites read the same
way. The code is separate because a test here is a conversation with a program that keeps
running, not a single process invocation whose files are inspected afterwards.
