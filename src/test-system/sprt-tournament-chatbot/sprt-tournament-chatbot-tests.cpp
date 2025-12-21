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

#include "sprt-tournament-chatbot-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

#include "sprt-tournament-test-helpers.h"
#include "sprt-tournament-step-actions.h"
#include "sprt-tournament-data.h"
#include "engine-worker-factory.h"
#include "imgui-engine-select.h"

namespace QaplaTest {

    using namespace SprtTournamentChatbot;

    void registerSprtTournamentChatbotTests(ImGuiTestEngine* engine) {
        ImGuiTest* tst = nullptr;

        // =====================================================================
        // FLOW TESTS - Complete happy path scenarios
        // =====================================================================

        // -----------------------------------------------------------------
        // Test: Complete New SPRT Tournament Flow -> Start -> Stay in Chatbot
        // Covers: Menu->New, GlobalSettings, SelectEngines, LoadEngine, 
        //         SprtConfiguration, Opening, PGN, Start, StayInChatbot
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "SprtTournament/Chatbot/Flow", "NewSprtTournamentComplete");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Complete New SPRT Tournament Flow ===");
            
            cleanupSprtTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            // Navigate to SPRT Tournament Chatbot
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);

            // Step: Menu - Create new tournament
            ctx->LogInfo("Step 1: Menu - New Tournament");
            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);

            // Step: GlobalSettings - Continue with defaults
            ctx->LogInfo("Step 2: GlobalSettings - Continue");
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);

            // Step: SelectEngines - Select two engines and continue
            ctx->LogInfo("Step 3: SelectEngines - Select & Continue");
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);

            // Step: ConfigureEngines - Load and select gauntlet
            ctx->LogInfo("Step 4: ConfigureEngines - Load & Select Gauntlet");
            IM_CHECK(executeConfigureEnginesStep(ctx));

            // Step: SprtConfiguration - Continue with defaults
            ctx->LogInfo("Step 5: SprtConfiguration - Continue");
            IM_CHECK(executeSprtConfigurationStep(ctx, SprtConfigurationAction::Continue));
            ctx->Yield(10);

            // Step: Opening - Setup file and continue
            ctx->LogInfo("Step 6: Opening - Continue");
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Continue, true));
            ctx->Yield(10);

            // Step: PGN - Setup file with Append mode
            ctx->LogInfo("Step 7: PGN - Continue (Append mode)");
            IM_CHECK(executePgnStep(ctx, PgnAction::Continue, true, true));
            ctx->Yield(10);

            // Step: Start - Start tournament
            ctx->LogInfo("Step 8: Start - Start SPRT Tournament");
            IM_CHECK(executeStartStep(ctx, StartAction::StartTournament));
            ctx->Yield(10);

            // Verify tournament is running
            auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
            IM_CHECK(sprtTournamentData.isRunning());

            // Step: Stay in Chatbot
            ctx->LogInfo("Step 9: Stay in Chatbot");
            IM_CHECK(executeStartStep(ctx, StartAction::StayInChatbot));

            ctx->LogInfo("=== Test NewSprtTournamentComplete PASSED ===");
            
            cleanupSprtTournamentState();
        };

        // -----------------------------------------------------------------
        // Test: New SPRT Tournament Flow -> Start -> Switch to View
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "SprtTournament/Chatbot/Flow", "NewSprtTournamentSwitchView");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: New SPRT Tournament with Switch to View ===");
            
            cleanupSprtTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);

            // Quick path through all steps
            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeConfigureEnginesStep(ctx));
            IM_CHECK(executeSprtConfigurationStep(ctx, SprtConfigurationAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Continue, true));
            ctx->Yield(10);
            // PGN with Overwrite mode
            IM_CHECK(executePgnStep(ctx, PgnAction::OverwriteContinue, true, false));
            ctx->Yield(10);
            IM_CHECK(executeStartStep(ctx, StartAction::StartTournament));
            ctx->Yield(10);

            // Different ending: Switch to SPRT Tournament view
            ctx->LogInfo("Final: Switch to SPRT Tournament View");
            IM_CHECK(executeStartStep(ctx, StartAction::SwitchToView));

            ctx->LogInfo("=== Test NewSprtTournamentSwitchView PASSED ===");
            
            cleanupSprtTournamentState();
        };

        // =====================================================================
        // CANCEL TESTS - Combined test for all cancel buttons
        // =====================================================================

        // -----------------------------------------------------------------
        // Test: Cancel at all steps (combined test)
        // Tests cancel functionality at Menu, GlobalSettings, SelectEngines, 
        // LoadEngine, SprtConfiguration, Opening, PGN, and Start
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "SprtTournament/Chatbot/Cancel", "AllCancelButtons");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: All Cancel Buttons ===");
            
            cleanupSprtTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            // Cancel at Menu
            ctx->LogInfo("Testing Cancel at Menu");
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);
            IM_CHECK(executeMenuStep(ctx, MenuAction::Cancel));
            ctx->Yield(5);

            // Cancel at GlobalSettings
            ctx->LogInfo("Testing Cancel at GlobalSettings");
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);
            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Cancel));
            ctx->Yield(5);

            // Cancel at SelectEngines
            ctx->LogInfo("Testing Cancel at SelectEngines");
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);
            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Cancel, false));
            ctx->Yield(5);

            // Cancel at LoadEngine
            ctx->LogInfo("Testing Cancel at LoadEngine");
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);
            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeLoadEngineStep(ctx, LoadEngineAction::Cancel));
            ctx->Yield(5);

            // Cancel at SprtConfiguration
            ctx->LogInfo("Testing Cancel at SprtConfiguration");
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);
            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeConfigureEnginesStep(ctx));
            IM_CHECK(executeSprtConfigurationStep(ctx, SprtConfigurationAction::Cancel));
            ctx->Yield(5);

            // Cancel at Opening
            ctx->LogInfo("Testing Cancel at Opening");
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);
            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeConfigureEnginesStep(ctx));
            IM_CHECK(executeSprtConfigurationStep(ctx, SprtConfigurationAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Cancel, false));
            ctx->Yield(5);

            // Cancel at PGN
            ctx->LogInfo("Testing Cancel at PGN");
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);
            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeConfigureEnginesStep(ctx));
            IM_CHECK(executeSprtConfigurationStep(ctx, SprtConfigurationAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executePgnStep(ctx, PgnAction::Cancel, false));
            ctx->Yield(5);

            // Cancel at Start
            ctx->LogInfo("Testing Cancel at Start");
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);
            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeConfigureEnginesStep(ctx));
            IM_CHECK(executeSprtConfigurationStep(ctx, SprtConfigurationAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executePgnStep(ctx, PgnAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeStartStep(ctx, StartAction::Cancel));
            ctx->Yield(5);

            // Verify tournament did NOT start
            auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
            IM_CHECK(!sprtTournamentData.isRunning());

            ctx->LogInfo("=== Test AllCancelButtons PASSED ===");
            
            cleanupSprtTournamentState();
        };

        // =====================================================================
        // CONTINUE EXISTING TESTS - Scenarios with existing tournament data
        // =====================================================================

        // -----------------------------------------------------------------
        // Test: Continue existing SPRT tournament - Yes and then No
        // Combined test: First continue, then restart chatbot and choose No
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "SprtTournament/Chatbot/Continue", "ExistingYesAndNo");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Continue Existing SPRT Tournament - Yes and No ===");
            
            cleanupSprtTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            // Create incomplete tournament state WITH actual results
            createIncompleteSprtTournamentState(ctx);
            ctx->Yield(10);

            // Verify precondition
            auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
            IM_CHECK(sprtTournamentData.hasResults());
            IM_CHECK(!sprtTournamentData.isRunning());

            // === PART 1: Choose to continue existing tournament ===
            ctx->LogInfo("Part 1: Testing 'Yes, continue'");
            
            // Navigate to chatbot - should see ContinueExisting step
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);

            // Choose to continue existing tournament
            ctx->LogInfo("Step: Continue existing tournament");
            IM_CHECK(executeContinueExistingStep(ctx, ContinueExistingAction::YesContinue));
            ctx->Yield(10);

            // Should be at Start step - start the tournament
            IM_CHECK(executeStartStep(ctx, StartAction::StartTournament));
            ctx->Yield(10);

            // Verify tournament is running
            IM_CHECK(sprtTournamentData.isRunning());

            // Stay in chatbot
            IM_CHECK(executeStartStep(ctx, StartAction::StayInChatbot));
            ctx->Yield(10);

            // Stop the tournament
            sprtTournamentData.stopPool(false);
            IM_CHECK(waitForSprtTournamentStopped(ctx, 10.0f));

            // === PART 2: Choose NOT to continue - start fresh ===
            ctx->LogInfo("Part 2: Testing 'No'");
            
            // Verify we still have results
            IM_CHECK(sprtTournamentData.hasResults());

            // Navigate to chatbot again
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);

            // Choose NOT to continue - go to Menu
            ctx->LogInfo("Step: Say No to continue");
            IM_CHECK(executeContinueExistingStep(ctx, ContinueExistingAction::No));
            ctx->Yield(10);

            // Should be at Menu step - cancel to exit
            IM_CHECK(executeMenuStep(ctx, MenuAction::Cancel));
            ctx->Yield(5);

            ctx->LogInfo("=== Test ExistingYesAndNo PASSED ===");
            
            cleanupSprtTournamentState();
        };

        // -----------------------------------------------------------------
        // Test: Continue existing SPRT tournament - Cancel
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "SprtTournament/Chatbot/Continue", "ExistingCancel");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Continue Existing SPRT Tournament - Cancel ===");
            
            cleanupSprtTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            createIncompleteSprtTournamentState(ctx);
            ctx->Yield(10);

            // Save initial state to verify it's preserved
            auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
            bool hadResultsBefore = sprtTournamentData.hasResults();

            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);

            // Cancel at ContinueExisting step
            ctx->LogInfo("Step: Cancel at ContinueExisting");
            IM_CHECK(executeContinueExistingStep(ctx, ContinueExistingAction::Cancel));
            ctx->Yield(5);

            // Verify incomplete state is preserved
            IM_CHECK_EQ(sprtTournamentData.hasResults(), hadResultsBefore);

            ctx->LogInfo("=== Test ExistingCancel PASSED ===");
            
            cleanupSprtTournamentState();
        };

        // =====================================================================
        // STOP RUNNING TESTS - Scenarios with running tournament
        // =====================================================================

        // -----------------------------------------------------------------
        // Test: Stop running SPRT tournament - Yes
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "SprtTournament/Chatbot/StopRunning", "EndTournament");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Stop Running SPRT Tournament - End ===");
            
            cleanupSprtTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            // Create and start a tournament
            createIncompleteSprtTournamentState(ctx);
            
            auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
            sprtTournamentData.startTournament();
            sprtTournamentData.setPoolConcurrency(1, true, true);
            
            // Wait for tournament to start
            IM_CHECK(waitForSprtTournamentRunning(ctx, 10.0f));

            // Navigate to chatbot - should see StopRunning step
            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);

            // End the tournament
            ctx->LogInfo("Step: End tournament");
            IM_CHECK(executeStopRunningStep(ctx, StopRunningAction::EndTournament));
            ctx->Yield(10);

            // Verify tournament stopped
            IM_CHECK(!sprtTournamentData.isRunning());

            // Should now be at Menu - cancel to exit
            IM_CHECK(executeMenuStep(ctx, MenuAction::Cancel));
            ctx->Yield(5);

            ctx->LogInfo("=== Test EndTournament PASSED ===");
            
            cleanupSprtTournamentState();
        };

        // -----------------------------------------------------------------
        // Test: Stop running SPRT tournament - Cancel (keep running)
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "SprtTournament/Chatbot/StopRunning", "KeepRunning");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Stop Running SPRT Tournament - Keep Running ===");
            
            cleanupSprtTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            createIncompleteSprtTournamentState(ctx);
            
            auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
            sprtTournamentData.startTournament();
            sprtTournamentData.setPoolConcurrency(1, true, true);
            
            IM_CHECK(waitForSprtTournamentRunning(ctx, 10.0f));

            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);

            // Cancel - keep tournament running
            ctx->LogInfo("Step: Cancel to keep running");
            IM_CHECK(executeStopRunningStep(ctx, StopRunningAction::Cancel));
            ctx->Yield(5);

            // Verify tournament is still running
            IM_CHECK(sprtTournamentData.isRunning());

            ctx->LogInfo("=== Test KeepRunning PASSED ===");
            
            cleanupSprtTournamentState();
        };

        // =====================================================================
        // OPTIONS TESTS - More/Less Options toggles
        // =====================================================================

        // -----------------------------------------------------------------
        // Test: Toggle Options in GlobalSettings, SprtConfiguration, Opening, PGN
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "SprtTournament/Chatbot/Options", "ToggleAllOptions");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Toggle All Options ===");
            
            cleanupSprtTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            IM_CHECK(navigateToSprtTournamentChatbot(ctx));
            ctx->Yield(10);

            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);

            // GlobalSettings: Toggle More/Less
            ctx->LogInfo("GlobalSettings: Toggle More Options");
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::MoreOptions));
            ctx->Yield(5);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::LessOptions));
            ctx->Yield(5);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);

            // SelectEngines
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeConfigureEnginesStep(ctx));

            // SprtConfiguration: Toggle More/Less
            ctx->LogInfo("SprtConfiguration: Toggle More Options");
            IM_CHECK(executeSprtConfigurationStep(ctx, SprtConfigurationAction::MoreOptions));
            ctx->Yield(5);
            IM_CHECK(executeSprtConfigurationStep(ctx, SprtConfigurationAction::LessOptions));
            ctx->Yield(5);
            IM_CHECK(executeSprtConfigurationStep(ctx, SprtConfigurationAction::Continue));
            ctx->Yield(10);

            // Opening: Toggle More/Less, Show/Hide Trace
            ctx->LogInfo("Opening: Toggle More Options and Trace");
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::MoreOptions, true));
            ctx->Yield(5);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::LessOptions, false));
            ctx->Yield(5);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::ShowTrace, false));
            ctx->Yield(5);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::HideTrace, false));
            ctx->Yield(5);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Continue, false));
            ctx->Yield(10);

            // PGN: Toggle More/Less
            ctx->LogInfo("PGN: Toggle More Options");
            IM_CHECK(executePgnStep(ctx, PgnAction::MoreOptions, true));
            ctx->Yield(5);
            IM_CHECK(executePgnStep(ctx, PgnAction::LessOptions, false));
            ctx->Yield(5);
            IM_CHECK(executePgnStep(ctx, PgnAction::Continue, false));
            ctx->Yield(5);

            // Cancel at Start
            IM_CHECK(executeStartStep(ctx, StartAction::Cancel));

            ctx->LogInfo("=== Test ToggleAllOptions PASSED ===");
            
            cleanupSprtTournamentState();
        };

    }

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
