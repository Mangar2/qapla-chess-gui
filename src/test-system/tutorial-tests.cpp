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

#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

#include "tutorial.h"
#include "tournament-window.h"
#include "tournament-data.h"
#include "engine-worker-factory.h"

namespace QaplaTest {

    namespace {
        // Helper to check if engines are available
        bool hasEnginesAvailable() {
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            auto configs = configManager.getAllConfigs();
            return configs.size() >= 2;
        }

        // Helper to cleanup tournament state
        void cleanupTournamentState() {
            auto& tournamentData = QaplaWindows::TournamentData::instance();
            if (tournamentData.isRunning() || tournamentData.isStarting()) {
                tournamentData.stopPool(false);
            }
            tournamentData.clear(false);
        }

        // Helper to click an item with existence check
        bool safeItemClick(ImGuiTestContext* ctx, const char* ref) {
            if (!ctx->ItemExists(ref)) {
                ctx->LogError("Item not found: %s", ref);
                return false;
            }
            ctx->ItemClick(ref);
            return true;
        }

        // Helper to navigate to Chatbot window
        bool navigateToChatbot(ImGuiTestContext* ctx) {
            if (!safeItemClick(ctx, "**/Chatbot###Chatbot")) {
                return false;
            }
            ctx->Yield(10);
            return true;
        }

        // Helper to wait for tutorial progress to reach a specific step
        bool waitForTutorialProgress(ImGuiTestContext* ctx, uint32_t targetProgress, float maxWaitSeconds = 5.0f) {
            float waited = 0.0f;
            constexpr float sleepInterval = 0.1f;
            while (QaplaWindows::TournamentWindow::tutorialProgress_ < targetProgress && waited < maxWaitSeconds) {
                ctx->SleepNoSkip(sleepInterval, sleepInterval);
                waited += sleepInterval;
            }
            return QaplaWindows::TournamentWindow::tutorialProgress_ >= targetProgress;
        }

        // Helper to wait for tutorial to request user input (when Continue button should appear)
        bool waitForTutorialUserInput(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f) {
            float waited = 0.0f;
            constexpr float sleepInterval = 0.1f;
            auto& tutorial = QaplaWindows::Tutorial::instance();
            while (!tutorial.doWaitForUserInput() && waited < maxWaitSeconds) {
                ctx->SleepNoSkip(sleepInterval, sleepInterval);
                waited += sleepInterval;
            }
            return tutorial.doWaitForUserInput();
        }

        // Helper to wait for highlighted section to change
        bool waitForHighlightedSection(ImGuiTestContext* ctx, const std::string& expectedSection, float maxWaitSeconds = 5.0f) {
            float waited = 0.0f;
            constexpr float sleepInterval = 0.1f;
            while (QaplaWindows::TournamentWindow::highlightedSection_ != expectedSection && waited < maxWaitSeconds) {
                ctx->SleepNoSkip(sleepInterval, sleepInterval);
                waited += sleepInterval;
            }
            return QaplaWindows::TournamentWindow::highlightedSection_ == expectedSection;
        }

        // Helper to wait for "Continue" button to appear (when tutorial waits for user input)
        bool waitForContinueButton(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f) {
            float waited = 0.0f;
            constexpr float sleepInterval = 0.1f;
            while (!ctx->ItemExists("**/Continue") && waited < maxWaitSeconds) {
                ctx->SleepNoSkip(sleepInterval, sleepInterval);
                waited += sleepInterval;
            }
            return ctx->ItemExists("**/Continue");
        }
    }

    void registerTutorialTests(ImGuiTestEngine* engine) {
        ImGuiTest* tst = nullptr;

        // =================================================================
        // Test: Tournament Tutorial via Chatbot - Part 1: Global Settings
        // Tests the tutorial flow from chatbot activation through global settings configuration
        // =================================================================
        tst = IM_REGISTER_TEST(engine, "Tutorial/Tournament", "ChatbotPart1GlobalSettings");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Tournament Tutorial via Chatbot - Part 1 ===");
            
            // Precondition: Clean state and engines available
            cleanupTournamentState();
            QaplaWindows::TournamentWindow::clearTournamentTutorialState();
            
            IM_CHECK(hasEnginesAvailable());

            // Step 1: Navigate to Chatbot
            ctx->LogInfo("Step 1: Navigate to Chatbot");
            IM_CHECK(navigateToChatbot(ctx));
            ctx->Yield(10);

            // Step 2: Click on "Tutorial" button in chatbot menu
            ctx->LogInfo("Step 2: Click Tutorial button");
            IM_CHECK(safeItemClick(ctx, "**/Tutorial"));
            ctx->Yield(10);

            // Step 3: Select "Tournament" tutorial from the list
            ctx->LogInfo("Step 3: Select Tournament tutorial");
            IM_CHECK(safeItemClick(ctx, "**/Tournament"));
            ctx->Yield(10);

            // Tutorial should now be started
            // tutorialProgress_ should advance from 0 to 1
            IM_CHECK(waitForTutorialProgress(ctx, 1, 5.0f));
            ctx->LogInfo("Tutorial started, progress: %d", QaplaWindows::TournamentWindow::tutorialProgress_);

            // Step 4: Open Tournament tab (tutorial is waiting for this)
            ctx->LogInfo("Step 4: Click Tournament tab");
            IM_CHECK(safeItemClick(ctx, "**/Tournament###Tournament"));
            ctx->Yield(10);

            // Step 4a: Wait for tutorial to request user input (doWaitForUserInput() becomes true)
            ctx->LogInfo("Step 4a: Wait for tutorial to request user input");
            IM_CHECK(waitForTutorialUserInput(ctx, 5.0f));
            ctx->LogInfo("Tutorial is waiting for user input, doWaitForUserInput: %s", 
                QaplaWindows::Tutorial::instance().doWaitForUserInput() ? "true" : "false");
            
            // Step 4b: Wait for Continue button to appear and click it
            ctx->LogInfo("Step 4b: Wait for Continue button in chatbot");
            IM_CHECK(waitForContinueButton(ctx, 5.0f));
            IM_CHECK(safeItemClick(ctx, "**/Continue"));
            ctx->Yield(10);
            
            // NOW the progress should advance to step 2
            IM_CHECK(waitForTutorialProgress(ctx, 2, 5.0f));
            ctx->LogInfo("Tutorial advanced after Continue click, progress: %d", QaplaWindows::TournamentWindow::tutorialProgress_);
            
            // Wait for GlobalSettings section to be highlighted
            ctx->LogInfo("Step 4c: Wait for GlobalSettings to be highlighted");
            IM_CHECK(waitForHighlightedSection(ctx, "GlobalSettings", 5.0f));
            ctx->LogInfo("GlobalSettings section highlighted: %s", QaplaWindows::TournamentWindow::highlightedSection_.c_str());
            
            // Verify GlobalSettings section is highlighted
            IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedSection_.c_str(), "GlobalSettings");
            IM_CHECK(QaplaWindows::TournamentWindow::globalSettingsTutorial_.highlight);

            // Step 4d: Open the GlobalSettings CollapsingHeader
            ctx->LogInfo("Step 4d: Open GlobalSettings section");
            ctx->ItemOpen("**/Global Engine Settings###Global Engine Settings");
            ctx->Yield(3);

            // Step 5: Configure global settings as requested by tutorial
            // Tutorial wants: Hash=64 MB, global ponder disabled
            ctx->LogInfo("Step 5: Configure Hash to 64 MB");
            
            auto& tournamentData = QaplaWindows::TournamentData::instance();
            auto& globalConfig = tournamentData.globalSettings().getGlobalConfiguration();
            
            // Set Hash to 64 MB via UI slider
            // Path to Hash slider: ImGui::PushID("Hash (MB)") -> ##value
            ctx->ItemInputValue("**/Hash (MB)", 64);
            ctx->Yield(5);
            
            // Verify Hash is set
            IM_CHECK_EQ(globalConfig.hashSizeMB, 64U);
            ctx->LogInfo("Hash set to: %d MB", globalConfig.hashSizeMB);

            // Step 6: Disable global pondering via checkbox
            ctx->LogInfo("Step 6: Disable global pondering");
            
            // The checkbox for "Enable global pondering" should be checked initially
            // We need to click it to uncheck it
            // Path: ImGui::PushID("Ponder") -> ##enableGlobal checkbox
            if (globalConfig.useGlobalPonder) {
                ctx->ItemUncheck("**/##usePonder");
                ctx->Yield(5);
            }
            
            // Verify global ponder is disabled
            IM_CHECK(!globalConfig.useGlobalPonder);
            ctx->LogInfo("Global pondering disabled: %s", globalConfig.useGlobalPonder ? "false" : "true");

            // Wait for tutorial to detect configuration is complete and request user input
            ctx->LogInfo("Step 6a: Wait for tutorial to request user input after global settings");
            IM_CHECK(waitForTutorialUserInput(ctx, 5.0f));
            ctx->LogInfo("Tutorial detected settings configured, doWaitForUserInput: %s", 
                QaplaWindows::Tutorial::instance().doWaitForUserInput() ? "true" : "false");
            
            // Step 6b: Wait for and click Continue button in chatbot
            ctx->LogInfo("Step 6b: Wait for Continue button in chatbot");
            IM_CHECK(waitForContinueButton(ctx, 5.0f));
            IM_CHECK(safeItemClick(ctx, "**/Continue"));
            ctx->Yield(10);
            
            // NOW progress advances to 3
            IM_CHECK(waitForTutorialProgress(ctx, 3, 5.0f));
            ctx->LogInfo("Tutorial advanced after Continue click, progress: %d", QaplaWindows::TournamentWindow::tutorialProgress_);
            
            // Verify tutorial moved to EngineSelect section
            IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedSection_.c_str(), "EngineSelect");
            IM_CHECK(!QaplaWindows::TournamentWindow::globalSettingsTutorial_.highlight);

            ctx->LogInfo("=== Test ChatbotPart1GlobalSettings PASSED ===");
            
            // Cleanup
            cleanupTournamentState();
            QaplaWindows::TournamentWindow::clearTournamentTutorialState();
        };
    }

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
