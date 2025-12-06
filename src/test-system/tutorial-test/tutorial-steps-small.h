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
        ctx->ItemClick("**/###Tournament");
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

        // Click Continue and advance to step 9
        clickContinueAndAdvance(ctx, 9);
        
        // Verify Run button highlighted
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedButton_.c_str(), "Run/Grace/Continue");
    }

    // Step 9: Start Tournament
    inline void executeStep09_StartTournament(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 9: Start Tournament");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Click Run button
        ctx->ItemClick("**/###Run");
        ctx->Yield();
        
        // Wait for tournament to start
        float waited = 0.0f;
        while (!tournamentData.isRunning() && waited < 10.0f) {
            ctx->SleepNoSkip(0.1f, 0.1f);
            waited += 0.1f;
        }
        
        IM_CHECK(tournamentData.isRunning());

        // Progress advances automatically when running
        IM_CHECK(waitForTutorialProgress(ctx, 10, 5.0f));
    }

    // Step 10: Wait for Tournament to Finish
    inline void executeStep10_WaitForFinish(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 10: Wait for Tournament to Finish");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Wait for tournament to finish (max 60 seconds for small test)
        float waited = 0.0f;
        while (tournamentData.isRunning() && waited < 60.0f) {
            ctx->SleepNoSkip(0.5f, 0.5f);
            waited += 0.5f;
        }
        
        IM_CHECK(!tournamentData.isRunning());
        IM_CHECK(tournamentData.hasTasksScheduled());

        // Progress advances automatically when finished
        IM_CHECK(waitForTutorialProgress(ctx, 11, 5.0f));
        
        // Verify Save As button highlighted
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedButton_.c_str(), "Save As");
    }

    // Step 11: Save Tournament
    inline void executeStep11_SaveTournament(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 11: Save Tournament");
        
        // Click Save As button
        ctx->ItemClick("**/###Save As");
        ctx->Yield();
        
        // Progress advances when button clicked
        IM_CHECK(waitForTutorialProgress(ctx, 12, 5.0f));
    }

    // Step 12: Add Third Engine
    inline void executeStep12_AddThirdEngine(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 12: Add Third Engine");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Add third engine - click + button for second available engine
        ctx->ItemClick("**/###addEngine", 0, ImGuiTestOpFlags_NoError);
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

        // Progress advances automatically
        IM_CHECK(waitForTutorialProgress(ctx, 13, 5.0f));
        
        // Verify Continue button highlighted
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedButton_.c_str(), "Run/Grace/Continue");
    }

    // Step 13: Continue Tournament
    inline void executeStep13_ContinueTournament(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 13: Continue Tournament");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Click Continue/Run button
        ctx->ItemClick("**/###Run");
        ctx->Yield();
        
        // Wait for tournament to start
        float waited = 0.0f;
        while (!tournamentData.isRunning() && waited < 10.0f) {
            ctx->SleepNoSkip(0.1f, 0.1f);
            waited += 0.1f;
        }
        
        IM_CHECK(tournamentData.isRunning());

        // Progress advances automatically
        IM_CHECK(waitForTutorialProgress(ctx, 14, 5.0f));
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
        
        // Progress advances automatically
        IM_CHECK(waitForTutorialProgress(ctx, 15, 5.0f));
    }

    // Step 15: Tutorial Complete
    inline void executeStep15_TutorialComplete(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 15: Tutorial Complete");
        
        // Wait for tutorial to finish
        float waited = 0.0f;
        while (QaplaWindows::TournamentWindow::tutorialProgress_ == 15 && waited < 5.0f) {
            ctx->SleepNoSkip(0.1f, 0.1f);
            waited += 0.1f;
        }
        
        ctx->LogInfo("Tutorial Complete!");
    }

} // namespace QaplaTest::TutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
