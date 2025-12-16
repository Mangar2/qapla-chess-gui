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

using namespace QaplaTest::BoardWindowTutorialTest;

namespace QaplaTest {

    void registerBoardWindowTutorialTests(ImGuiTestEngine* engine) {
        ImGuiTest* tst = nullptr;

        // =================================================================
        // Test: Board Window Tutorial Complete Flow
        // Tests the complete board window tutorial from start to finish
        // This tutorial runs via snackbar (not in chatbot) - started from
        // chatbot but runs directly on the board with snackbar notifications
        // =================================================================
        tst = IM_REGISTER_TEST(engine, "Tutorial/BoardWindow", "CompleteTutorial");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Board Window Tutorial - Complete Flow ===");
            
            // Precondition: Clean state and engines available
            cleanupBoardWindowState();
            resetChatbotToInitialState(ctx);
            
            IM_CHECK(hasEnginesAvailable());
            ctx->LogInfo("Engines available: OK");

            // Navigate to Chatbot and start tutorial selection
            ctx->LogInfo("Starting Tutorial via Chatbot");
            navigateToChatbot(ctx);
            ctx->ItemClick("**/###Tutorial");
            ctx->Yield(2);
            
            // Select "Board Window" tutorial from the list
            ctx->ItemClick("**/Chatbot/###Board Window");
            ctx->Yield(2);
            
            // Click "Switch to Board & Start" button (tutorial runs via snackbar, not in chatbot)
            ctx->ItemClick("**/###Switch to Board & Start");
            ctx->Yield(2);
            
            // Wait for tutorial to start (progress should be 1)
            IM_CHECK(waitForTutorialProgress(ctx, 1, 5.0f));
            ctx->LogInfo("Tutorial started, progress: %d", QaplaWindows::BoardWindow::tutorialBoardProgress_);
            
            // Execute all tutorial steps on the board with snackbar guidance
            executeStep01_ClickPlay(ctx);
            executeStep02_MakeCounterMove(ctx);
            executeStep03_ClickPlayAgain(ctx);
            executeStep04_StopAndManualMove(ctx);
            executeStep05_ClickAnalyze(ctx);
            executeStep06_StopAnalysis(ctx);
            executeStep07_ClickAuto(ctx);
            executeStep08_StopAutoPlay(ctx);
            executeStep09_TutorialComplete(ctx);

            ctx->LogInfo("=== Test CompleteTutorial PASSED ===");
            
            // Cleanup
            cleanupBoardWindowState();
        };
    }

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
