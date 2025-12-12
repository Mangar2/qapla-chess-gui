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
#include "tutorial-step-02-global-settings.h"
#include "tutorial-step-03-select-engines.h"
#include "tutorial-step-04-opening.h"
#include "tutorial-step-05-tournament-config.h"
#include "tutorial-step-06-time-control.h"
#include "tutorial-step-07-pgn-file.h"

using namespace QaplaTest::TutorialTest;

namespace QaplaTest {

    static void resetTestData() {
        // Clear all selected engines
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        auto& getEngineSelect = tournamentData.getEngineSelect();
        auto configs = getEngineSelect.getEngineConfigurations();
        for (auto& cfg : configs) {
            cfg.selected = false;
            cfg.config.setPonder(false);
        }
        getEngineSelect.setEngineConfigurations(configs);
        
        // Reset global settings to defaults
        QaplaWindows::ImGuiEngineGlobalSettings::GlobalConfiguration defaultGlobalConfig;
        tournamentData.getGlobalSettings().setGlobalConfiguration(defaultGlobalConfig);
        
        // Reset opening configuration to defaults
        QaplaTester::Openings defaultOpenings;
        tournamentData.tournamentOpening().openings() = defaultOpenings;
        
        // Reset tournament configuration to defaults
        QaplaTester::TournamentConfig defaultTournamentConfig;
        tournamentData.config() = defaultTournamentConfig;
        
        // Reset time control to default
        QaplaWindows::ImGuiEngineGlobalSettings::TimeControlSettings defaultTimeControl;
        tournamentData.getGlobalSettings().setTimeControlSettings(defaultTimeControl);
        
        // Reset PGN configuration to defaults
        QaplaTester::PgnSave::Options defaultPgnOptions;
        tournamentData.tournamentPgn().pgnOptions() = defaultPgnOptions;
    }

    void registerTutorialTests(ImGuiTestEngine* engine) {
        ImGuiTest* tst = nullptr;

        // =================================================================
        // Test: Tournament Tutorial Complete Flow
        // Tests the complete tournament tutorial from start to finish
        // =================================================================
        tst = IM_REGISTER_TEST(engine, "Tutorial/Tournament", "CompleteTutorial");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Tournament Tutorial - Complete Flow ===");
            
            // Precondition: Clean state and engines available
            cleanupTournamentState();
            QaplaWindows::TournamentWindow::clearTournamentTutorialState();
            resetTestData();
            
            IM_CHECK(hasEnginesAvailable());

            // Navigate to Chatbot and start tutorial
            ctx->LogInfo("Starting Tutorial via Chatbot");
            navigateToChatbot(ctx);
            ctx->ItemClick("**/###Tutorial");
            ctx->Yield(2);
            ctx->ItemClick("**/Chatbot/###Tournament");
            ctx->Yield(2);
            
            // Wait for tutorial to start
            IM_CHECK(waitForTutorialProgress(ctx, 1, 5.0f));
            ctx->LogInfo("Tutorial started, progress: %d", QaplaWindows::TournamentWindow::tutorialProgress_);
            // Execute all tutorial steps
            executeStep01_OpenTournamentTab(ctx);
            executeStep02_ConfigureGlobalSettings(ctx);
            executeStep03_SelectEngines(ctx);
            executeStep04_ConfigureOpening(ctx);
            executeStep05_ConfigureTournament(ctx);
            executeStep06_ConfigureTimeControl(ctx);
            executeStep07_SetPgnFile(ctx);
            executeStep08_SetConcurrency(ctx);
            executeStep09_StartTournament(ctx);
            executeStep10_WaitForFinish(ctx);
            executeStep11_SaveTournament(ctx);
            executeStep12_AddThirdEngine(ctx);
            executeStep13_ContinueTournament(ctx);
            executeStep14_WaitForExtendedFinish(ctx);
            executeStep15_TutorialComplete(ctx);

            ctx->LogInfo("=== Test CompleteTutorial PASSED ===");
            
            // Cleanup
            cleanupTournamentState();
            QaplaWindows::TournamentWindow::clearTournamentTutorialState();
        };
    }

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
