# The GUI froze: bug protocol

Kept as a protocol rather than a report: for a bug that shows itself once in a while, only what
was measured counts. Every entry below says whether it was observed, measured, or is unknown.
Nothing is written here as a cause until a measurement says so.

## What was observed

* **Volker, on the machine:** the GUI stopped answering. The macOS spinning wheel appeared -- the
  window server's sign that the application is not processing events.
* It happened during a **GUI test run**, not during an integration run.

## What was measured (2026-08-25, macOS, releasetest build)

One frozen process, inspected with lldb before it was ended. Backtraces of all 20 threads kept.

* UI thread: `ImGui::Render()` -> `ImGui::EndFrame()` -> `ImGuiTestEngine_PreEndFrameHook()` ->
  `Coroutine_ImplStdThread_Run()` -> `condition_variable::wait()`. It is waiting for the test
  coroutine to hand control back. No graphics call is involved.
* Test coroutine thread: `cleanupTournamentState()` -> `TournamentData::clear(bool)` ->
  `GameManagerPool::waitForTask()` -> `future.wait()`.
* `frames.count` from the remote control stood still (4945) while the process was frozen.
* The two futures `waitForTask()` was waiting on were unfulfilled -- a deduction from the fact
  that it had not returned, not a measurement of the managers.

Nothing was measured about the managers themselves. The release build carries no debug
information, so `managerState_` and `finishedPromiseValid_` could not be read.

## What is unknown

* The state of the game managers at that moment -- the value that would name the cause.
* Whether the fault is in the GUI, in the engine tester, or in the test code. The call chain
  crosses all three.
* Which of two quite different shapes it is:
  1. a manager never reports itself finished (a message missing or lost), or
  2. `waitForTask()`'s loop keeps finding new pending work, so it never returns.
* How often it happens. One event is not a rate.
* Whether the debug build is affected. Three clean runs say nothing: at a rate of one in eight,
  three clean runs have a probability of about two thirds.
* Whether the state of the display plays any role. Ruled out for this freeze by the backtrace,
  unmeasured otherwise.
* Whether the 307-second frame in `render` seen once during an **integration** run (release
  build, no test engine, so no coroutine) is the same thing or something else.

## Measurements

### 1. How long a GUI run takes in the debug build, and at what frame rate

Two runs in a row, debug build, GUI suite only.

macOS, debug build, `--remote-desktop` (as every test run now uses), 2026-08-25.

| Run | Result | Wall clock | Frames | Frames/s | Stalls | Worst frame |
|-----|--------|-----------:|-------:|---------:|-------:|-------------|
| 1 | 40 of 41 | 605 s | 14971 | 24.7 | 1 | 76 ms in `render` |
| 2 | 41 of 41 | 592 s | 14850 | 25.1 | 3 | 75 ms in `render` |

So a GUI run in the debug build takes about ten minutes and draws at about 25 frames a second,
which is inside the 8-32 band remote desktop mode asks for. Neither run froze, and the worst
frame in either was 76 ms -- four orders of magnitude away from the frozen process, which sat in
one frame for 35 minutes.

The failing test in run 1 is a different, older problem, measured below.

## Side finding: why Tutorial/EPD fails in a full run

Measured in run 1 above, with a diagnostic added for it:

    Continue clicked, wanted 8: window progress 7, tutorial counter 7,
    waiting for user input 1, epd state 4

Both counters agree at 7, and the tutorial is still waiting for user input. Had the click
reached `Tutorial::requestNextTutorialStep(name, false)`, that flag would be false and the
counter 8. So the click did not take effect -- the button was found (the wait for it passed) but
clicking it changed nothing.

This is not the window-clears-its-counter problem that the tournament tutorial had; there the two
counters disagreed. What is still unknown here is *which* item the click landed on: `**/###Continue`
matches any window, and a full run has more windows open than a single test does.

### 2. What a CLOP run costs the UI thread when it starts

Three runs of `clop-runs-to-the-end`, release build, worst frame each time:

| Run | Worst frame | Section |
|-----|------------:|---------|
| 1 | 63 ms | `tool:start` |
| 2 | 53 ms | `tool:start` |
| 3 | 54 ms | `tool:start` |

Every run, the same section, no outlier and no growth. `ClopData::start()` reads and parses the
openings file and starts the optimizer's threads, and that is what it costs. Steady initialisation,
not a blockage.

The stall threshold was 50 ms, so this failed three times out of three for doing its job. It is
now 100 ms -- ten frames a second -- which still leaves four orders of magnitude to the thing the
watch exists for: the frozen GUI sat in one frame for 35 minutes.

### 3. The state of the manager the pool waits for (2026-08-26)

Reproduced on the first of ten planned release runs, caught alive: the `releasetest` build was
rebuilt with `-O2 -g` first -- same optimisation, so the same timing, but with readable variables.
The harness inspects a run that stops making progress before it ends it.

Same stacks as the first freeze, now with source lines:

* UI thread waits for the test coroutine inside `ImGui::EndFrame()`.
* Test coroutine: `cleanupTournamentState()` (tournament-test-helpers.cpp:77) ->
  `TournamentData::clear(false)` (tournament-data.cpp:728) -> `GameManagerPool::waitForTask()`
  (game-manager-pool.cpp:313) -> `future.wait()`.
* `frames.count` frozen at 5105, current frame 178 seconds and counting, section `render`.

The pool held exactly one manager, and this is its state, read from the frozen process:

| Field | Value | Meaning |
|-------|-------|---------|
| `managerState_` | 6 = `NotRunning` | torn down |
| `taskProvider_` | `nullptr` | no provider |
| `eventQueue_` | empty | nothing will ever wake it |
| `stopThread_` | false | its thread is alive |
| `finishedPromiseValid_` | **true** | **the promise was never fulfilled** |
| pool's `maxConcurrency_` | 0 | nothing is being started any more |

So the manager is finished without having reported itself finished, and the pool waits for it
forever. Because `TournamentData::clear()` waits on the UI thread, the application freezes.

What the code allows, given that state:

* `managerState_` is written in only two places: `tearDown()` (to `NotRunning`) and `executeTask()`
  (to the task type), plus the compare-exchange in `start()`. So the manager reached `NotRunning`
  through `tearDown()` -- which signals.
* `initializeFinishedFuture()` is called in exactly one place: `GameManager::start()`. So a
  `start()` must have run *after* that teardown, creating a fresh promise nobody fulfils.
* `tearDown()` returns early when the manager is already not running, without signalling:

      if (!isRunning()) { return; }

**Still unknown: who called `start()` at that point.** `GameManagerPool::startManagers()` is the
only caller, and it starts every idle manager with a null provider, which matches the measured
`taskProvider_ == nullptr`. It is reached from `setConcurrency(count, nice, start = true)` and
from the schedulers. Which of them ran, and why after everything had been stopped, is not measured.

### 4. Reproduced on demand, over HTTP (2026-08-26)

A standalone probe, no GUI suite: configure a tournament, start it, wait until it reports itself
running, then stop it. Progress is judged by the frame counter, not by whether the call returns --
a frozen UI thread still answers `/health`.

**Proven:**

* **A single abrupt stop right after the start freezes the application.** One `stop` call, no
  second stop, no clear. It froze at attempt 5 of 30; earlier variants of the probe froze at
  attempts 2, 3 and 11. The freeze is inside the first stop: `POST /tools/stop` never returns.

      runApp -> Callback::invokeAll()                  the frame loop
        -> GuiToolRegistry::processQueue()
          -> Actions::stopActivity(tournament, Abrupt)
            -> TournamentData::stopPoolAbruptlyAndWait()   tournament-data.cpp:758
              -> GameManagerPool::waitForTask()            game-manager-pool.cpp:313

  So the double stop of the GUI test's cleanup is not needed: the second call never even reached
  the queue, because the first was already holding the UI thread.

* **Waiting three seconds after the start makes it go away.** Same probe with `--pause 3`: 30
  attempts, no freeze. The fault is bound to the window right after the start, while the run is
  still settling.

* **Writing to stdout or stderr hides it.** The first tracing attempt wrote one line per event to
  stderr with a flush. With it: 25 attempts clean. Without it, same binary and same fixes: frozen
  at attempt 2. Any instrumentation of this bug has to stay in memory -- a ring buffer read
  afterwards -- or it will observe a fault that is no longer there.

* **The two fixes committed so far do not fix it.** `finishedPromiseValid_` as an atomic, and
  `start()` waiting up to 50 ms for the previous run, are both in the build that froze at attempt
  2. They remain correct on their own account; they are not the cause.

The state of the manager the pool waits for is the same as in the first freeze, five days of
guessing earlier: `NotRunning`, no provider, empty queue, `finishedPromiseValid_ == true`.
