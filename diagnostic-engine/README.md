# Diagnostic UCI Engine

A minimal UCI chess engine designed to diagnose Linux-specific issues with tournament starts in the Qapla Chess GUI.

## Purpose

This diagnostic engine logs:
- All UCI commands received (with timestamps)
- All responses sent
- All signals received (SIGTERM, SIGINT, SIGPIPE, etc.)
- Process ID and lifecycle events

It helps identify why engines disconnect during tournament starts on Linux.

## Building

### Standalone Build

```bash
cd diagnostic-engine
mkdir build
cd build
cmake ..
cmake --build .
```

The executable will be in `build/bin/diagnostic-engine`.

### Integrated Build

The diagnostic engine is automatically built when you build the main GUI:

```bash
cd /home/mangar/dev/qapla-chess-gui
cmake --build --preset=default
```

## Usage

### Manual Testing

```bash
./diagnostic-engine
```

Then type UCI commands:
```
uci
isready
position startpos
go infinite
stop
quit
```

### With Qapla Chess GUI

1. Build the diagnostic engine
2. In the GUI, add a new engine and point it to the `diagnostic-engine` executable
3. Start a tournament or game
4. Check the log files created in the current directory

## Log Files

Log files are created with the pattern:
```
diagnostic-engine-YYYY-MM-DD_HH-MM-SS-pidXXXXX.log
```

Each log entry includes:
- Timestamp with millisecond precision
- Entry type (INPUT, OUTPUT, SIGNAL, SYSTEM, GAME, SEARCH, etc.)
- Message content

## Expected Behavior

The diagnostic engine:
- Responds to `uci` with engine identification and `uciok`
- Responds to `isready` with `readyok`
- Accepts `ucinewgame` without crashing
- Accepts `position` commands
- Responds to `go` commands with `bestmove e2e4`
- Exits gracefully on `quit`
- Logs all signals received
- Never crashes unless killed by external signal

## Troubleshooting

If the diagnostic engine crashes during tournament start:
1. Check the log file for the last command received
2. Check for any signals received
3. Note the exact sequence of commands before the crash
4. Compare with expected UCI protocol flow

This will help identify if the issue is:
- In the engine (unlikely, as this is minimal)
- In the GUI's command sequencing
- In the process management layer
- Related to signal handling
