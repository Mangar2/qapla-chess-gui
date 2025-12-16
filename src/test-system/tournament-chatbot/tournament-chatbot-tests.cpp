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

#include "tournament-chatbot-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

#include "tournament-test-helpers.h"
#include "tournament-step-actions.h"
#include "tournament-data.h"

namespace QaplaTest {

    using namespace TournamentChatbot;

    void registerTournamentChatbotTests(ImGuiTestEngine* engine) {
        ImGuiTest* tst = nullptr;

        // =====================================================================
        // FLOW TESTS - Complete happy path scenarios
        // =====================================================================

        // -----------------------------------------------------------------
        // Test: Complete New Tournament Flow -> Start -> Stay in Chatbot
        // Covers: Menu->New, GlobalSettings, SelectEngines, LoadEngine, 
        //         Configuration, Opening, PGN, Start, StayInChatbot
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Flow", "NewTournamentComplete");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Complete New Tournament Flow ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            // Navigate to Tournament Chatbot
            IM_CHECK(navigateToTournamentChatbot(ctx));
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

            // Step: LoadEngine - Skip detection
            ctx->LogInfo("Step 4: LoadEngine - Skip Detection");
            IM_CHECK(executeLoadEngineStep(ctx, LoadEngineAction::SkipDetection));
            ctx->Yield(10);

            // Step: Configuration - Continue with defaults
            ctx->LogInfo("Step 5: Configuration - Continue");
            IM_CHECK(executeConfigurationStep(ctx, ConfigurationAction::Continue));
            ctx->Yield(10);

            // Step: Opening - Setup file and continue
            ctx->LogInfo("Step 6: Opening - Continue");
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Continue, true));
            ctx->Yield(10);

            // Step: PGN - Setup file with Append mode (ensures "Continue" button)
            ctx->LogInfo("Step 7: PGN - Continue (Append mode)");
            IM_CHECK(executePgnStep(ctx, PgnAction::Continue, true, true));  // append=true
            ctx->Yield(10);

            // Step: Start - Start tournament
            ctx->LogInfo("Step 8: Start - Start Tournament");
            IM_CHECK(executeStartStep(ctx, StartAction::StartTournament));
            ctx->Yield(10);

            // Verify tournament is running
            auto& tournamentData = QaplaWindows::TournamentData::instance();
            IM_CHECK(tournamentData.isRunning());

            // Step: Stay in Chatbot
            ctx->LogInfo("Step 9: Stay in Chatbot");
            IM_CHECK(executeStartStep(ctx, StartAction::StayInChatbot));

            ctx->LogInfo("=== Test NewTournamentComplete PASSED ===");
            
            cleanupTournamentState();
        };

        // -----------------------------------------------------------------
        // Test: New Tournament Flow -> Start -> Switch to View
        // Covers: Same as above but with SwitchToView ending
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Flow", "NewTournamentSwitchView");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: New Tournament with Switch to View ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            // Quick path through all steps
            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeLoadEngineStep(ctx, LoadEngineAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeConfigurationStep(ctx, ConfigurationAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Continue, true));
            ctx->Yield(10);
            // PGN with Overwrite mode to test "Overwrite & Continue" button
            IM_CHECK(executePgnStep(ctx, PgnAction::OverwriteContinue, true, false));  // append=false
            ctx->Yield(10);
            IM_CHECK(executeStartStep(ctx, StartAction::StartTournament));
            ctx->Yield(10);

            // Different ending: Switch to Tournament view
            ctx->LogInfo("Final: Switch to Tournament View");
            IM_CHECK(executeStartStep(ctx, StartAction::SwitchToView));

            ctx->LogInfo("=== Test NewTournamentSwitchView PASSED ===");
            
            cleanupTournamentState();
        };

        // =====================================================================
        // CANCEL TESTS - Cancel at various steps
        // Äquivalenzklasse: Early cancel, Middle cancel, Late cancel
        // =====================================================================

        // -----------------------------------------------------------------
        // Test: Cancel at Menu (earliest possible cancel)
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Cancel", "AtMenu");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Menu ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            // Cancel immediately at Menu
            IM_CHECK(executeMenuStep(ctx, MenuAction::Cancel));
            ctx->Yield(5);

            ctx->LogInfo("=== Test AtMenu PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: Cancel at GlobalSettings
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Cancel", "AtGlobalSettings");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at GlobalSettings ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);

            // Cancel at GlobalSettings
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Cancel));
            ctx->Yield(5);

            ctx->LogInfo("=== Test AtGlobalSettings PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: Cancel at SelectEngines
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Cancel", "AtSelectEngines");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at SelectEngines ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);

            // Cancel without selecting engines
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Cancel, false));
            ctx->Yield(5);

            ctx->LogInfo("=== Test AtSelectEngines PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: Cancel at Configuration (middle cancel)
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Cancel", "AtConfiguration");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Configuration ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeLoadEngineStep(ctx, LoadEngineAction::Continue));
            ctx->Yield(10);

            // Cancel at Configuration
            IM_CHECK(executeConfigurationStep(ctx, ConfigurationAction::Cancel));
            ctx->Yield(5);

            ctx->LogInfo("=== Test AtConfiguration PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: Cancel at Opening
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Cancel", "AtOpening");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Opening ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeLoadEngineStep(ctx, LoadEngineAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeConfigurationStep(ctx, ConfigurationAction::Continue));
            ctx->Yield(10);

            // Cancel at Opening (don't setup file)
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Cancel, false));
            ctx->Yield(5);

            ctx->LogInfo("=== Test AtOpening PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: Cancel at PGN (late cancel)
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Cancel", "AtPgn");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at PGN ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeLoadEngineStep(ctx, LoadEngineAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeConfigurationStep(ctx, ConfigurationAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Continue, true));
            ctx->Yield(10);

            // Cancel at PGN (don't setup file)
            IM_CHECK(executePgnStep(ctx, PgnAction::Cancel, false));
            ctx->Yield(5);

            ctx->LogInfo("=== Test AtPgn PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: Cancel at Start (latest cancel)
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Cancel", "AtStart");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Start ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            IM_CHECK(executeMenuStep(ctx, MenuAction::NewTournament));
            ctx->Yield(10);
            IM_CHECK(executeGlobalSettingsStep(ctx, GlobalSettingsAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeSelectEnginesStep(ctx, SelectEnginesAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executeLoadEngineStep(ctx, LoadEngineAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeConfigurationStep(ctx, ConfigurationAction::Continue));
            ctx->Yield(10);
            IM_CHECK(executeOpeningStep(ctx, OpeningAction::Continue, true));
            ctx->Yield(10);
            IM_CHECK(executePgnStep(ctx, PgnAction::Continue, true));
            ctx->Yield(10);

            // Cancel at Start (before actually starting)
            IM_CHECK(executeStartStep(ctx, StartAction::Cancel));
            ctx->Yield(5);

            // Verify tournament did NOT start
            auto& tournamentData = QaplaWindows::TournamentData::instance();
            IM_CHECK(!tournamentData.isRunning());

            ctx->LogInfo("=== Test AtStart PASSED ===");
        };

        // =====================================================================
        // CONTINUE EXISTING TESTS - Scenarios with existing tournament data
        // =====================================================================

        // -----------------------------------------------------------------
        // Test: Continue existing tournament - Yes
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Continue", "ExistingYes");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Continue Existing Tournament - Yes ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            // Create incomplete tournament state
            createIncompleteTournamentState(ctx);
            ctx->Yield(10);

            // Verify precondition
            auto& tournamentData = QaplaWindows::TournamentData::instance();
            IM_CHECK(tournamentData.hasTasksScheduled());
            IM_CHECK(!tournamentData.isRunning());

            // Navigate to chatbot - should see ContinueExisting step
            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            // Choose to continue existing tournament
            ctx->LogInfo("Step: Continue existing tournament");
            IM_CHECK(executeContinueExistingStep(ctx, ContinueExistingAction::YesContinue));
            ctx->Yield(10);

            // Should be at Start step - start the tournament
            IM_CHECK(executeStartStep(ctx, StartAction::StartTournament));
            ctx->Yield(10);

            // Verify tournament is running
            IM_CHECK(tournamentData.isRunning());

            // Stay in chatbot
            IM_CHECK(executeStartStep(ctx, StartAction::StayInChatbot));

            ctx->LogInfo("=== Test ExistingYes PASSED ===");
            
            cleanupTournamentState();
        };

        // -----------------------------------------------------------------
        // Test: Continue existing tournament - No (start fresh)
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Continue", "ExistingNo");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Continue Existing Tournament - No ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            createIncompleteTournamentState(ctx);
            ctx->Yield(10);

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            // Choose NOT to continue - go to Menu
            ctx->LogInfo("Step: Say No to continue");
            IM_CHECK(executeContinueExistingStep(ctx, ContinueExistingAction::No));
            ctx->Yield(10);

            // Should be at Menu step - cancel to exit
            IM_CHECK(executeMenuStep(ctx, MenuAction::Cancel));
            ctx->Yield(5);

            ctx->LogInfo("=== Test ExistingNo PASSED ===");
            
            cleanupTournamentState();
        };

        // -----------------------------------------------------------------
        // Test: Continue existing tournament - Cancel
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Continue", "ExistingCancel");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Continue Existing Tournament - Cancel ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            createIncompleteTournamentState(ctx);
            ctx->Yield(10);

            // Save initial state to verify it's preserved
            auto& tournamentData = QaplaWindows::TournamentData::instance();
            bool hadTasksBefore = tournamentData.hasTasksScheduled();

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            // Cancel at ContinueExisting step
            ctx->LogInfo("Step: Cancel at ContinueExisting");
            IM_CHECK(executeContinueExistingStep(ctx, ContinueExistingAction::Cancel));
            ctx->Yield(5);

            // Verify incomplete state is preserved
            IM_CHECK_EQ(tournamentData.hasTasksScheduled(), hadTasksBefore);

            ctx->LogInfo("=== Test ExistingCancel PASSED ===");
            
            cleanupTournamentState();
        };

        // =====================================================================
        // STOP RUNNING TESTS - Scenarios with running tournament
        // =====================================================================

        // -----------------------------------------------------------------
        // Test: Stop running tournament - Yes
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/StopRunning", "EndTournament");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Stop Running Tournament - End ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            // Create and start a tournament
            createIncompleteTournamentState(ctx);
            
            auto& tournamentData = QaplaWindows::TournamentData::instance();
            tournamentData.startTournament();
            tournamentData.setPoolConcurrency(1, true, true);
            
            // Wait for tournament to start
            IM_CHECK(waitForTournamentRunning(ctx, 10.0f));

            // Navigate to chatbot - should see StopRunning step
            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            // End the tournament
            ctx->LogInfo("Step: End tournament");
            IM_CHECK(executeStopRunningStep(ctx, StopRunningAction::EndTournament));
            ctx->Yield(10);

            // Verify tournament stopped
            IM_CHECK(!tournamentData.isRunning());

            // Should now be at Menu - cancel to exit
            IM_CHECK(executeMenuStep(ctx, MenuAction::Cancel));
            ctx->Yield(5);

            ctx->LogInfo("=== Test EndTournament PASSED ===");
            
            cleanupTournamentState();
        };

        // -----------------------------------------------------------------
        // Test: Stop running tournament - Cancel (keep running)
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/StopRunning", "KeepRunning");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Stop Running Tournament - Keep Running ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            createIncompleteTournamentState(ctx);
            
            auto& tournamentData = QaplaWindows::TournamentData::instance();
            tournamentData.startTournament();
            tournamentData.setPoolConcurrency(1, true, true);
            
            IM_CHECK(waitForTournamentRunning(ctx, 10.0f));

            IM_CHECK(navigateToTournamentChatbot(ctx));
            ctx->Yield(10);

            // Cancel - keep tournament running
            ctx->LogInfo("Step: Cancel to keep running");
            IM_CHECK(executeStopRunningStep(ctx, StopRunningAction::Cancel));
            ctx->Yield(5);

            // Verify tournament is still running
            IM_CHECK(tournamentData.isRunning());

            ctx->LogInfo("=== Test KeepRunning PASSED ===");
            
            cleanupTournamentState();
        };

        // =====================================================================
        // OPTIONS TESTS - More/Less Options toggles
        // =====================================================================

        // -----------------------------------------------------------------
        // Test: Toggle Options in GlobalSettings, Configuration, Opening, PGN
        // Äquivalenzklasse: Tests all options buttons in one flow
        // -----------------------------------------------------------------
        tst = IM_REGISTER_TEST(engine, "Tournament/Chatbot/Options", "ToggleAllOptions");
        tst->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Toggle All Options ===");
            
            cleanupTournamentState();
            resetChatbotToInitialState(ctx);
            IM_CHECK(hasEnginesAvailable());

            IM_CHECK(navigateToTournamentChatbot(ctx));
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
            IM_CHECK(executeLoadEngineStep(ctx, LoadEngineAction::Continue));
            ctx->Yield(10);

            // Configuration: Toggle More/Less
            ctx->LogInfo("Configuration: Toggle More Options");
            IM_CHECK(executeConfigurationStep(ctx, ConfigurationAction::MoreOptions));
            ctx->Yield(5);
            IM_CHECK(executeConfigurationStep(ctx, ConfigurationAction::LessOptions));
            ctx->Yield(5);
            IM_CHECK(executeConfigurationStep(ctx, ConfigurationAction::Continue));
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
            
            cleanupTournamentState();
        };

    }

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
