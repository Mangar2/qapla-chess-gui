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

#pragma once

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include "tutorial-test-helpers.h"

namespace QaplaTest::TutorialTest {

    // Step 1: Open Tournament Tab
    inline void executeStep01_OpenTournamentTab(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 1: Open Tournament Tab");
        
        // Click Tournament tab
        ctx->ItemClick("**/QaplaTabBar/###Tournament");
        ctx->Yield();

        // Click Continue and advance to step 2
        clickContinueAndAdvance(ctx, 2);
    }

    // Step 8: Set Concurrency to 4
    inline void executeStep08_SetConcurrency(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 8: Set Concurrency to 4");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Set concurrency to 4
        ctx->ItemInputValue("**/###Concurrency", 4);
        ctx->Yield();
        
        // Verify concurrency is set
        IM_CHECK_EQ(tournamentData.getExternalConcurrency(), 4U);

        // No Continue button here, as we made just one change
        ctx->Yield();
        IM_CHECK(waitForTutorialProgress(ctx, 9, 5.0f));
        
        // Verify Run button highlighted
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedButton_.c_str(), "RunGraceContinue");
    }

    // Step 9: Start Tournament
    inline void executeStep09_StartTournament(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 9: Start Tournament");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Click Run button
        ctx->ItemClick("**/###Tournament/RunGraceContinue");
        ctx->Yield();
        
        // Wait for tournament to start
        float waited = 0.0f;
        while (!tournamentData.isRunning() && waited < 10.0f) {
            ctx->SleepNoSkip(0.1f, 0.1f);
            waited += 0.1f;
        }
        
        IM_CHECK(tournamentData.isRunning());

        clickContinueAndAdvance(ctx, 10);
    }

    // Step 10: Wait for Tournament to Finish
    inline void executeStep10_WaitForFinish(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 10: Wait for Tournament to Finish");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // For testing: Click Stop button instead of waiting for tournament completion
        ctx->ItemClick("**/###Tournament/Stop");
        ctx->Yield();
        
        // Wait for tournament to actually stop
        float waited = 0.0f;
        while (tournamentData.isRunning() && waited < 5.0f) {
            ctx->SleepNoSkip(0.1f, 0.1f);
            waited += 0.1f;
        }
        
        IM_CHECK(!tournamentData.isRunning());

        // Progress advances automatically when finished
        clickContinueAndAdvance(ctx, 11);
        
        // Verify Save As button highlighted
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedButton_.c_str(), "Save As");
    }

    // Step 11: Save Tournament
    inline void executeStep11_SaveTournament(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 11: Save Tournament");
        
        // For testing: Simulate the button click without actually opening the file dialog
        // The tutorial only checks if the button was clicked, not if file was saved
        QaplaWindows::TournamentWindow::showNextTournamentTutorialStep("Save As");
        ctx->Yield();
        
        // Progress advances when button clicked
        clickContinueAndAdvance(ctx, 12);
    }

    // Step 12: Add Third Engine
    inline void executeStep12_AddThirdEngine(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 12: Add Third Engine");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Open Engines section
        ctx->ItemOpen("**/###Engines");
        ctx->Yield();

        // Add third engine - click + button for second available engine
        ctx->ItemClick("**/available_1/###addEngine", 0, ImGuiTestOpFlags_NoError);
        ctx->Yield();
        
        // Verify at least 3 engines selected
        const auto& configs = tournamentData.engineSelect().getEngineConfigurations();
        int selectedCount = 0;
        for (const auto& engineConfig : configs) {
            if (engineConfig.selected) {
                selectedCount++;
            }
        }
        IM_CHECK(selectedCount >= 3);

        // Close Engines section
        ctx->ItemClose("**/###Engines");
        ctx->Yield();

        // Progress advances automatically
        clickContinueAndAdvance(ctx, 13);
        
        // Verify Continue button highlighted
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedButton_.c_str(), "RunGraceContinue");
    }

    // Step 13: Continue Tournament
    inline void executeStep13_ContinueTournament(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 13: Continue Tournament");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Click Continue/Run button
        ctx->ItemClick("**/###Tournament/RunGraceContinue");
        ctx->Yield();
        
        // Wait for tournament to start
        float waited = 0.0f;
        while (!tournamentData.isRunning() && waited < 10.0f) {
            ctx->SleepNoSkip(0.1f, 0.1f);
            waited += 0.1f;
        }
        
        IM_CHECK(tournamentData.isRunning());

        // Progress advances automatically
        clickContinueAndAdvance(ctx, 14);
    }

    // Step 14: Wait for Extended Tournament to Finish
    inline void executeStep14_WaitForExtendedFinish(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 14: Wait for Extended Tournament to Finish");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Wait for tournament to finish (max 60 seconds)
        float waited = 0.0f;
        while (tournamentData.isRunning() && waited < 60.0f) {
            ctx->SleepNoSkip(0.5f, 0.5f);
            waited += 0.5f;
        }
        ctx->ItemClick("**/###Tournament/Stop");
        ctx->Yield(2);
        // Progress advances automatically
        ctx->ItemClick("**/###Continue");
    }

    // Step 15: Tutorial Complete
    inline void executeStep15_TutorialComplete(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 15: Tutorial Complete");
        ctx->Yield(2);
        // Wait for tutorial to finish
        float waited = 0.0f;
        while (QaplaWindows::TournamentWindow::tutorialProgress_ == 15 && waited < 5.0f) {
            ctx->SleepNoSkip(0.1f, 0.1f);
            waited += 0.1f;
        }
        ctx->ItemClick("**/###Close");
        ctx->LogInfo("Tutorial Complete!");
    }

} // namespace QaplaTest::TutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
