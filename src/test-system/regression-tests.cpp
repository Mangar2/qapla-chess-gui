/**
 * @license
 * ... (License header) ...
 */

#include "test-system/regression-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

namespace QaplaTest {

    void registerRegressionTests(ImGuiTestEngine* engine) {
        ImGuiTest* t = nullptr;

        // -----------------------------------------------------------------
        // Test: Smoke Test - Start and Stop Game
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Regression", "SmokeTest");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("Starting Smoke Test");

            // Click Play
            // We use wildcards because the button is deeply nested:
            // RootOverlay -> main -> main.right -> Board 1 -> hsplit.board_moves.left -> Play
            ctx->LogInfo("Clicking Play...");
            ctx->ItemClick("**/###Play");
            
            // Wait for game to run a bit
            ctx->Sleep(2.0f);

            // Click Stop
            ctx->LogInfo("Clicking Stop...");
            ctx->ItemClick("**/###Stop");

            ctx->LogInfo("Smoke Test Completed");
        };
    }

} // namespace QaplaTest
#endif
