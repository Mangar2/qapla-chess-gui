# Diagnostic UCI Engine

A configurable UCI chess engine designed to diagnose issues with tournament starts and engine communication in the Qapla Chess GUI.

## Features

This diagnostic engine supports three operational modes and plays random legal moves:

### Engine Modes

The engine mode is determined by the executable name:

1. **LOG Mode** (default, e.g., `diagnostic-engine.exe`)
   - Logs all UCI commands, responses, and signals with timestamps
   - Plays random legal moves using the integrated chess position logic
   - Creates detailed log files for debugging
   - Full UCI protocol implementation

2. **NOINIT Mode** (e.g., `diagnostic-engine-noinit.exe`)
   - Minimal mode that ignores all input except `quit`
   - No logging, no output
   - Used to test launcher behavior with non-responsive engines

3. **LOOP Mode** (e.g., `diagnostic-engine-loop.exe`)
   - Same as LOG mode, but hangs in an infinite loop when receiving `isready`
   - Used to diagnose timeout handling

4. **LOSSONTIME Mode** (e.g., `diagnostic-engine-lossontime.exe`)
   - Progressively wastes increasing amounts of time on each move
   - Move 1: sleeps for 5% of available time
   - Move 2: sleeps for 10% of available time
   - Move 3: sleeps for 15% of available time
   - Continues incrementing by 5% until it exceeds available time and loses on time
   - No debug output to avoid interference with time measurements
   - Used to test time management and time forfeit handling

### Chess Functionality

- **Position Handling**: Supports `position startpos` and `position fen <fen>` commands
- **Move Parsing**: Handles move lists in UCI format (e.g., `position startpos moves e2e4 e7e5`)
- **Random Legal Moves**: On `go` commands, immediately returns a randomly selected legal move
- **Full Move Generation**: Uses the Qapla engine's move generator for accurate legal move detection

## Purpose

This diagnostic engine helps identify:
- UCI protocol communication issues
- Engine startup and shutdown problems
- Signal handling on different platforms (Windows/Linux)
- Tournament launcher behavior with various engine responses
- Timeout and hang scenarios

## Building

### Standalone Build

```bash
cd diagnostic-engine
mkdir build
cd build
cmake ..
cmake --build .
```

The executable will be in `build/bin/diagnostic-engine.exe` (or `diagnostic-engine` on Linux).

### Integrated Build

The diagnostic engine is automatically built when you build the main GUI:

```bash
cmake --build --preset=default
```

Or on Windows:

```powershell
cmake --build build/default --target diagnostic-engine
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
go
position startpos moves e2e4 e7e5
go
quit
```

### File-based Testing

Create a test file with UCI commands:
```bash
# test-commands.txt
uci
isready
position startpos
go
quit
```

Run the engine with the test file:
```bash
# Linux/macOS
cat test-commands.txt | ./diagnostic-engine

# Windows PowerShell
Get-Content test-commands.txt | .\diagnostic-engine.exe
```

### Creating Different Modes

To use different modes, rename or copy the executable:

```bash
# Windows
Copy-Item diagnostic-engine.exe diagnostic-engine-noinit.exe
Copy-Item diagnostic-engine.exe diagnostic-engine-loop.exe
Copy-Item diagnostic-engine.exe diagnostic-engine-lossontime.exe

# Linux/macOS
cp diagnostic-engine diagnostic-engine-noinit
cp diagnostic-engine diagnostic-engine-loop
cp diagnostic-engine diagnostic-engine-lossontime
```

### Testing with GUI

Add the engine to your GUI's engine list and configure tournaments as needed. Check the generated log files for detailed execution traces.

## Log Files

In LOG mode, the engine creates timestamped log files:
- Format: `diagnostic-engine-YYYY-MM-DD_HH-MM-SS-mmm-pidXXXXX.log`
- Location: Current working directory
- Content: All UCI I/O, system events, and signals

Example log entry:
```
[2025-11-15 22:53:34.940] INPUT: #4 'go'
[2025-11-15 22:53:34.940] SEARCH: Search command: go
[2025-11-15 22:53:34.940] SEARCH: Randomly selected move 12 of 20: d2d4
[2025-11-15 22:53:34.940] OUTPUT: bestmove d2d4
```

## Platform Support

- **Windows**: Full support with Visual Studio or Clang
- **Linux**: Full support with GCC or Clang
- **macOS**: Full support

Signal handling is platform-aware:
- Windows: SIGTERM, SIGINT, SIGABRT, SIGSEGV
- Linux/macOS: Additionally handles SIGHUP, SIGPIPE

## Technical Details

### Dependencies

- **GameState**: Integrated from qapla-engine-tester for position management
- **Move Generator**: Full legal move generation using the Qapla chess engine
- **Standard Library**: C++20 features (filesystem, optional, etc.)

### Architecture

- Modular design with separate functions for UCI command handling
- Exception-safe initialization with detailed error reporting
- Lazy GameState initialization after logging is established
- Platform-agnostic code with conditional compilation for OS-specific features

## Example Output

```
id name Diagnostic Engine 1.0
id author Qapla Chess GUI Team
option name Ponder type check default false
option name Hash type spin default 128 min 1 max 4096
uciok
readyok
info depth 1 score cp 0 nodes 20 nps 1000 time 1
bestmove d2d4
```

## Troubleshooting

- **Engine crashes on startup**: Check the log file for initialization errors
- **No log file created**: Engine is running in NOINIT mode (check executable name)
- **Engine hangs on isready**: Engine is running in LOOP mode (intentional behavior)
- **Illegal moves**: Position parsing issue - check FEN string in log file
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
