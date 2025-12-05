# Copilot Instructions

## C++ Code Style
- C++20, use ranges, format, nodiscard
- Min identifier length: 3 chars
- Only "why" comments, JSDoc for method declarations
- Max complexity: 20, max nesting: 3
- Prevent implicit conversions, use same types
- Always use curly braces for control statements
- Use `auto` when type is already visible in the line

## Architecture
- The chess GUI is inherently **asynchronous** - operations like tournaments, analysis, and games run in background threads
- Always wait for state changes via **state checks in a loop**, not fixed Sleep() calls
- Use polling helpers like `waitForAnalysisRunning()`, `waitForAnalysisStopped()` that check actual state
- State transitions may take variable time depending on engine response, file I/O, etc.

## ImGui
- Use controls from `imgui-controls.h`, not raw ImGui calls

## imgui_test_engine Tests
- Use `IM_CHECK(condition)` for all assertions - `ctx->LogError()` does NOT fail the test!
- Check item existence before clicking:
  ```cpp
  IM_CHECK(ctx->ItemExists("**/Button"));
  ctx->ItemClick("**/Button");
  ```
- Use `IM_CHECK_EQ`, `IM_CHECK_GT`, etc. for comparisons
- Never use skip-patterns like `if (!x) { return; }` - test appears green!
- Hierarchical categories: `IM_REGISTER_TEST(engine, "Module/Feature", "TestName")`
- Always cleanup at start AND end of test
- **Async waiting**: Use `SleepNoSkip()` with state polling, not `Sleep()` (skipped in Fast mode):
  ```cpp
  // Good: Poll state with SleepNoSkip
  while (!epdData.isRunning() && waited < timeout) {
      ctx->SleepNoSkip(0.1f, 0.1f);
      waited += 0.1f;
  }
  // Bad: Fixed sleep (skipped in Fast mode)
  ctx->Sleep(2.0f);
  ```
