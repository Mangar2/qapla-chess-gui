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
#include "tutorial-step-02-select-engines.h"
#include "tutorial-step-03-configuration.h"

using namespace QaplaTest::EpdTutorialTest;

namespace QaplaTest {

    static void resetEpdTestData() {
        auto& epdData = QaplaWindows::EpdData::instance();
        
        // Clear all selected engines
        auto& engineSelect = epdData.engineSelect();
        auto configs = engineSelect.getEngineConfigurations();
        for (auto& cfg : configs) {
            cfg.selected = false;
        }
        engineSelect.setEngineConfigurations(configs);
        
        // Reset configuration to defaults
        auto& config = epdData.config();
        config.seenPlies = 0;
        config.maxTimeInS = 60;
        config.minTimeInS = 0;
        config.filepath = "";
        
        // Reset concurrency to default
        epdData.setExternalConcurrency(1);
        epdData.setPoolConcurrency(1, true);
    }

    void registerEpdTutorialTests(ImGuiTestEngine* engine) {
        ImGuiTest* tst = nullptr;

        // =================================================================
        // Test: EPD Tutorial Complete Flow
        // Tests the complete EPD tutorial from start to finish
        // =================================================================
        tst = IM_REGISTER_TEST(engine, "Tutorial/EPD", "CompleteTutorial");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: EPD Tutorial - Complete Flow ===");
            
            // Precondition: Clean state and engines available
            cleanupEpdState();
            QaplaWindows::EpdWindow::clearEpdTutorialState();
            resetEpdTestData();
            
            IM_CHECK(hasEnginesAvailable());

            // Navigate to Chatbot and start tutorial
            ctx->LogInfo("Starting Tutorial via Chatbot");
            navigateToChatbot(ctx);
            ctx->ItemClick("**/###Tutorial");
            ctx->Yield(2);
            ctx->ItemClick("**/Chatbot/###EPD Analysis");
            ctx->Yield(2);
            
            // Wait for tutorial to start
            IM_CHECK(waitForTutorialProgress(ctx, 1, 5.0f));
            ctx->LogInfo("Tutorial started, progress: %d", QaplaWindows::EpdWindow::tutorialProgress_);
            
            // Execute all tutorial steps
            executeStep01_OpenEpdTab(ctx);
            executeStep02_SelectEngines(ctx);
            executeStep03_ConfigureAnalysis(ctx);
            executeStep04_StartAnalysis(ctx);
            executeStep05_StopAnalysis(ctx);
            executeStep06_ContinueAnalysis(ctx);
            executeStep07_GraceStop(ctx);
            executeStep08_WaitAndClear(ctx);
            executeStep09_StartFreshAnalysis(ctx);
            executeStep10_SetConcurrency(ctx);
            executeStep11_TutorialComplete(ctx);

            ctx->LogInfo("=== Test CompleteTutorial PASSED ===");
            
            // Cleanup
            cleanupEpdState();
            QaplaWindows::EpdWindow::clearEpdTutorialState();
        };
    }

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
