# GUI Integration Tests — Coverage

Every test here starts its own Qapla Chess GUI, with a configuration directory of its own, and
drives it over the HTTP remote control (`--remote-control`). Runtimes are recorded per test in
`test_results.log` after each run.

Run them with `python3 test/integration/test_runner.py`, from anywhere. On Linux without a
desktop session the runner puts itself under `xvfb-run`.

These tests took over from the `Llm/Tournament/Tools`, `Llm/Sprt/Tools`, `Llm/Epd/Tools` and
`Llm/Status/Tools` suites of the on-screen GUI tests, which drove the same tool registry in
process. What remains there is what only a window can be asked: the chat itself, and the tools
that need a person in front of it.

## session (7 tests)

The channel itself, before any chess happens. If these fail, nothing else in the suite means
anything.

| Name | Description |
|---|---|
| session-starts | The GUI comes up, publishes its eleven tools and shuts down again |
| session-empty-sandbox | A test's configuration directory starts as a fresh installation — no engines |
| session-health-needs-no-token | `/health` answers without the token, so liveness can always be checked |
| session-tools-need-the-token | `/tools`, `/status` and `/shutdown` are refused without the shared secret |
| session-unknown-tool | A tool that does not exist answers 404, not a failed call |
| session-local-only-tool-is-refused | `close_application` exists, but not for a remote caller |
| session-status-when-idle | The overview answers even when nothing is running |

## state (7 tests)

`GET /state` reports what `GET /status` says in sentences, as data. Tests that need a value use
it; the sentences are written for a language model and are free to be reworded.

| Name | Description |
|---|---|
| state-reports-every-activity | A fresh session reports all four run types as idle and not running |
| state-knows-when-a-run-could-start | Configuring a tournament flips `ready_to_start`, without starting it |
| state-reports-a-run-while-it-is-going | A running tournament is reported as running, and as running only |
| state-reports-the-standings-as-data | A finished tournament's table comes back as headers and rows |
| state-agrees-with-the-spoken-status | The data answer and the sentence answer report the same fact |
| state-answers-in-english-with-a-german-gui | The remote control does not follow the GUI's language setting |
| state-is-not-a-published-tool | `get_state` serves the endpoint; it is not offered to a model |

## engines_catalog (6 tests)

| Name | Description |
|---|---|
| engines-install-and-list | An installed engine is detected and then listed by name |
| engines-details | Details name the executable and the detected protocol |
| engines-copy-and-delete | An engine can be copied under a new name and deleted again |
| engines-install-nonexistent-path | Installing something that is not there is refused, and nothing is added |
| engines-details-of-unknown-engine | Asking about an engine that was never installed is an error, not an empty answer |
| engines-real-engines-install | Qapla and Spike install and are detected as UCI |

## tournament (9 tests)

| Name | Description |
|---|---|
| tournament-basic | Two engines play a two-game tournament to the end; standings and PGN checked as data |
| tournament-needs-openings | A tournament without an openings file is refused, with the reason |
| tournament-reports-the-standings | The finished run answers with the standings table, not just a verdict |
| tournament-is-reproducible | With `QAPLA_DIAG_SEED` fixed, the same tournament produces the same table |
| tournament-stop-while-running | A long tournament can be stopped, and `/wait` reports `stopped` |
| tournament-concurrency-survives-a-stop | An abrupt stop leaves the configured concurrency alone |
| tournament-adjudication-settings | Draw and resign adjudication arrive, are reported back, and survive a restart |
| tournament-closes-while-a-run-is-going | The application shuts down properly with games still playing |
| tournament-clear-result | Clearing takes the finished run's results away — checked as an absent table, not a sentence |

## epd (6 tests)

The position file is four mates in one, written into every sandbox by the framework — so a real
engine finds all four in hundredths of a second and the test can assert *how many* were solved.

| Name | Description |
|---|---|
| epd-solves-mates-in-one | A real engine analyses four mates in one and finds all of them |
| epd-reports-what-was-not-found | An engine playing at random solves none, and the run still ends |
| epd-concurrency-survives-a-stop | An abrupt stop leaves the configured concurrency alone |
| epd-needs-a-position-file | Starting without a position file is refused, with the reason |
| epd-start-after-a-config-change-is-refused | Re-analysing without clearing is refused, and the refusal says how |
| epd-two-engines-side-by-side | Two engines analyse the same positions and both get a column |

## clop (3 tests)

| Name | Description |
|---|---|
| clop-runs-to-the-end | A short tuning run plays its samples and reports an estimate |
| clop-needs-a-parameter | Tuning nothing is refused: a run without a parameter has no subject |
| clop-stop-while-running | A long tuning run can be stopped from outside |

## parallel (3 tests)

| Name | Description |
|---|---|
| parallel-overview-names-both-runs | A tournament and an SPRT test at once are both reported as running |
| parallel-stopping-one-leaves-the-other | Stopping one run does not stop the other sharing the pool |
| parallel-second-start-of-the-same-type-is-refused | One tournament at a time: starting it twice is an error |

## sprt (4 tests)

| Name | Description |
|---|---|
| sprt-runs-to-the-game-limit | An undecidable SPRT test stops at `max_games` and reports it |
| sprt-has-its-own-configuration | Configuring a tournament leaves the SPRT settings alone |
| sprt-concurrency-survives-a-stop | An abrupt stop leaves the configured concurrency alone |
| sprt-stop-while-running | A long SPRT test can be stopped from outside |

## persistence (4 tests)

What survives the application being closed and started again. The reason `POST /shutdown` exists:
killing the process would skip the shutdown that writes the settings.

| Name | Description |
|---|---|
| persistence-engine-catalog-survives-a-restart | Engines installed in one session are there in the next, and in the ini |
| persistence-tournament-settings-survive-a-restart | A configuration set over HTTP is still configured after a restart |
| persistence-saved-tournament-loads-after-a-restart | A finished tournament written to a file is read back in a new session |
| persistence-load-a-file-that-is-not-there | Loading a missing results file is refused rather than half-applied |

## errors (3 tests, slow)

The broken variants of the diagnostic engine. These take minutes, and unavoidably so: detecting
an engine that will not answer costs the protocol timeout twice over. That is the application
behaving correctly, and the runtime is recorded so that a change making it worse shows up.

| Name | Description |
|---|---|
| errors-engine-that-never-answers | An engine that ignores everything loses its games; the run still ends |
| errors-engine-that-hangs | An engine that hangs on `isready` does not hang the application |
| errors-engine-that-loses-on-time | An engine that overspends its clock forfeits, and the cause is recorded |
