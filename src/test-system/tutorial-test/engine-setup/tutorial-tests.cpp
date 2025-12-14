/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "tutorial-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include "tutorial-test-helpers.h"
#include "tutorial-steps-small.h"

using namespace QaplaTest::EngineSetupTutorialTest;

namespace QaplaTest {

    static void resetEngineSetupTestData() {
        // Clear all engines to start from clean state
        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
        auto configs = configManager.getAllConfigs();
        for (const auto& cfg : configs) {
            configManager.removeConfig(cfg);
        }
    }

    void registerEngineSetupTutorialTests(ImGuiTestEngine* engine) {
        ImGuiTest* tst = nullptr;

        // =================================================================
        // Test: Engine Setup Tutorial Complete Flow
        // Tests the complete engine setup tutorial from start to finish
        // =================================================================
        tst = IM_REGISTER_TEST(engine, "Tutorial/EngineSetup", "CompleteTutorial");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Engine Setup Tutorial - Complete Flow ===");
            
            // Precondition: Clean state
            cleanupEngineSetupState();
            QaplaWindows::EngineSetupWindow::clearEngineSetupTutorialState();
            resetEngineSetupTestData();

            // Navigate to Chatbot and start tutorial
            ctx->LogInfo("Starting Tutorial via Chatbot");
            navigateToChatbot(ctx);
            ctx->ItemClick("**/###Tutorial");
            ctx->Yield(2);
            ctx->ItemClick("**/Chatbot/###Add Engines");
            ctx->Yield(2);
            
            // Wait for tutorial to start
            IM_CHECK(waitForTutorialProgress(ctx, 1, 5.0f));
            ctx->LogInfo("Tutorial started, progress: %d", QaplaWindows::EngineSetupWindow::tutorialProgress_);
            
            // Execute all tutorial steps
            executeStep01_OpenEnginesTab(ctx);
            executeStep02a_AddFakeEngines(ctx);
            executeStep02b_DetectFakeEngines(ctx);
            executeStep02c_RemoveFakeEngines(ctx);
            executeStep03_AddRealEngines(ctx);

            ctx->LogInfo("=== Test CompleteTutorial PASSED ===");
            
            // Cleanup
            cleanupEngineSetupState();
            QaplaWindows::EngineSetupWindow::clearEngineSetupTutorialState();
        };
    } // registerEngineSetupTutorialTests

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
