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
