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
        // Test: Open Tournament Tab
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Regression", "OpenTournamentTab");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            // 1. Open the "View" menu (assuming there is one, adjust as needed)
            // ctx->MenuClick("View/Tournament");

            // 2. Or if it's a tab in a dockspace, try to find it.
            // For now, we just log something to prove it runs.
            ctx->LogInfo("Test 'OpenTournamentTab' is running!");
            
            // Example of checking a window existence
            // ctx->SetRef("Tournament");
            // IM_CHECK(ctx->WindowIsFocused());
        };
    }

} // namespace QaplaTest
#endif
