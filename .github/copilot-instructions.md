# Copilot Instructions

## Project Overview
**Qapla Chess GUI** is a professional chess interface for **engine developers and testers**, built with C++20, ImGui, OpenGL, and GLFW. The GUI wraps the command-line [qapla-engine-tester](extern/qapla-engine-tester) to provide visual tournament management, SPRT testing, and EPD analysis with real-time game observation.

### Key Architecture
- **Embedded Window System**: All UI components inherit from `EmbeddedWindow` (see [embedded-window.h](src/embedded-window.h)). Windows are composed into layouts via `HorizontalSplitContainer` and `VerticalSplitContainer`.
- **Asynchronous Core**: Tournaments, analysis, and games run in background threads via `GameManagerPool` (singleton or shared_ptr). The GUI polls state changes - never use fixed `Sleep()` calls.
- **Data Classes**: Major features have singleton data classes (e.g., `TournamentData`, `EpdData`, `SprtTournamentData`) that manage state and integrate with `GameManagerPool`.
- **Autosavable Pattern**: Config, EPD results, and i18n use `Autosavable` base class for automatic periodic saves to config directory.

## C++ Code Style
- C++20 - use ranges, format, nodiscard
- Min identifier length: 3 chars
- Only "why" comments; JSDoc for method declarations
- Max complexity: 20, max nesting: 3
- Prevent implicit conversions, use same types
- **Explicit bool conversions**: Never rely on implicit int→bool. Use `!= 0` (preferred) or `static_cast<bool>()`
- Always use curly braces for control statements
- Use `auto` when type is already visible in the line

## Build & Test Workflow
Build uses CMake with presets (see [CMakePresets.json](CMakePresets.json)):
```bash
# Debug build (enables i18n debug mode)
cmake --preset default && cmake --build --preset default

# With test engine (imgui_test_engine)
cmake --preset test && cmake --build --preset test

# Release build
cmake --preset release && cmake --build --preset release
```

Test builds define `IMGUI_ENABLE_TEST_ENGINE` - tests in [src/test-system](src/test-system).

**IMPORTANT**: When modifying or creating tests, always build and verify with the `test` preset:
```bash
cmake --preset test && cmake --build --preset test
```
The default build does NOT include test engine support and will compile successfully even with test-related errors.

## ImGui & UI Conventions
- **Use wrappers from [imgui-controls.h](src/imgui-controls.h)** instead of raw ImGui calls - provides consistent styling, i18n support, and behavior.
- **i18n**: All user-facing text uses `Translator::instance().translate(topic, key)`. In debug mode, missing translations are auto-added to `.lang` files with timestamps.
- **Snackbar notifications**: Use `SnackbarManager::instance().showSuccess()`, `.showError()`, `.showNote()` for user feedback.

## Async State Management
- Operations run in `GameManagerPool` threads - the GUI only observes state via polling in `pollData()` methods.
- **State polling pattern** (see [tournament-test-helpers.h](src/test-system/tournament-chatbot/tournament-test-helpers.h)):
  ```cpp
  bool waitForTournamentRunning(ImGuiTestContext* ctx, float maxWait) {
      float waited = 0.0f;
      while (!tournamentData.isRunning() && waited < maxWait) {
          ctx->SleepNoSkip(0.1f, 0.1f);
          waited += 0.1f;
      }
      return tournamentData.isRunning();
  }
  ```
- State transitions (e.g., Starting → Running) depend on engine response, file I/O, etc. - timing varies.

## imgui_test_engine Tests
- **Critical**: Use `IM_CHECK(condition)` for assertions - `ctx->LogError()` does NOT fail tests!
- **Do NOT check ItemExists before clicking** - `ItemClick` already fails the test if the item doesn't exist:
  ```cpp
  // WRONG - redundant check:
  // IM_CHECK(ctx->ItemExists("**/Button"));
  // ctx->ItemClick("**/Button");
  
  // CORRECT - just click directly:
  ctx->ItemClick("**/Button");
  ```
- Use `IM_CHECK_EQ`, `IM_CHECK_GT`, etc. for comparisons
- Never use skip-patterns like `if (!x) { return; }` - test appears green even if it should fail!
- Hierarchical categories: `IM_REGISTER_TEST(engine, "Module/Feature", "TestName")`
- Always cleanup at start AND end of test
- **Async waiting**: Use `SleepNoSkip()` with state polling, not `Sleep()` (skipped in Fast mode)

## Integration with qapla-engine-tester
The [extern/qapla-engine-tester](extern/qapla-engine-tester) sources are compiled directly into the GUI (see [CMakeLists.txt](CMakeLists.txt) line 93). Key classes:
- `GameManagerPool`: Manages threads and distributes tasks
- `Tournament`, `SprtManager`: Run tournament logic
- `EpdReader`: Analyzes EPD positions
- **Access pattern**: GUI uses `GameManagerPoolAccess` wrapper to support both singleton and shared_ptr instances.
