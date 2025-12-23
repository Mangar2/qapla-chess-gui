# Unit Tests

This directory contains Catch2 unit tests for:

## qapla-chess-gui Tests
Pure logic tests for non-ImGui code:
- Data classes without GUI dependencies
- Helper functions and utilities
- Algorithms and calculations

**Note:** Classes with ImGui dependencies (`*-window.h`, `imgui-*.h`) should be tested with imgui_test_engine (see `../tutorial-test/` and `../tournament-chatbot/`).

## qapla-engine-tester Tests
Tests for the CLI engine tester library (`extern/qapla-engine-tester/src/`):
- Game management logic
- Tournament algorithms
- EPD analysis
- SPRT calculations
- Adjudication

## Running Tests

```bash
# Build
cmake --build --preset default --target unit-tests

# Run all tests
./build/default/unit-tests.exe

# Run with filters
./build/default/unit-tests.exe "[tournament]"
./build/default/unit-tests.exe "[engine-tester]"
```
