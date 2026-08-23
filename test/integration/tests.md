# GUI Integration Tests — Coverage

Every test here starts its own Qapla Chess GUI, with a configuration directory of its own, and
drives it over the HTTP remote control (`--remote-control`). Runtimes are recorded per test in
`test_results.log` after each run.

Run them with `test/integration/run.sh` (`run.ps1` on Windows), or
`python3 test/integration/test_runner.py` from the repository root.

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

## engines_catalog (6 tests)

| Name | Description |
|---|---|
| engines-install-and-list | An installed engine is detected and then listed by name |
| engines-details | Details name the executable and the detected protocol |
| engines-copy-and-delete | An engine can be copied under a new name and deleted again |
| engines-install-nonexistent-path | Installing something that is not there is refused, and nothing is added |
| engines-details-of-unknown-engine | Asking about an engine that was never installed is an error, not an empty answer |
| engines-real-engines-install | Qapla and Spike install and are detected as UCI |

## tournament (6 tests)

| Name | Description |
|---|---|
| tournament-basic | Two engines play a two-game tournament to the end; PGN is written |
| tournament-needs-openings | A tournament without an openings file is refused, with the reason |
| tournament-reports-the-standings | The finished run answers with the standings table, not just a verdict |
| tournament-is-reproducible | With `QAPLA_DIAG_SEED` fixed, the same tournament produces the same table |
| tournament-stop-while-running | A long tournament can be stopped, and `/wait` reports `stopped` |
| tournament-clear-result | Clearing takes the finished run's results away |

## sprt (3 tests)

| Name | Description |
|---|---|
| sprt-runs-to-the-game-limit | An undecidable SPRT test stops at `max_games` and reports it |
| sprt-has-its-own-configuration | Configuring a tournament leaves the SPRT settings alone |
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
