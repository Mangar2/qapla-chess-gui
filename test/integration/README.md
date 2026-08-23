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
test/integration/run.sh                        # everything
test/integration/run.sh --filter 'tournament-*'
test/integration/run.sh --test session-starts --keep-sandbox
test/integration/run.sh --list
test/integration/run.sh --skip-passed           # continue after a failure
test/integration/run.ps1                        # Windows
```

Before the first run, build the project (`cmake --build --preset default`): the GUI and the
diagnostic engines are build output. The two real engines, `Qapla` and `SpikeEngine`, are
expected in `engines/` at the top of the repository; the runner says so up front if any engine is
missing, rather than failing test by test.

The exit code is 0 only if every test passed, so a release script can gate on it.

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
