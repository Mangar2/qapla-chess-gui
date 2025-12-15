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

namespace QaplaTest::BoardWindowTutorialTest {

    /**
     * @brief Step 1: Click Play button - engine makes first move
     * Tutorial message: "Click the 'Play' button to make the first engine (white) play a move."
     * Completion: Engine plays a move (halfmoves > 0)
     */
    inline void executeStep01_ClickPlay(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 1: Click Play button");
        
        // Wait for snackbar tutorial message to appear
        IM_CHECK(waitForSnackbarTutorialMessage(ctx, 5.0f));
        
        // Click Play button on the board
        ctx->ItemClick("**/Board/Play");
        ctx->Yield();

        // Wait for progress to advance to step 2 (engine made a move)
        IM_CHECK(waitForTutorialProgress(ctx, 2, 30.0f));
        ctx->LogInfo("Step 1 completed: Engine played a move");
    }

    /**
     * @brief Step 2: User makes a counter-move by clicking on the board
     * Tutorial message: "Now make a counter-move..."
     * Completion: User makes a move (halfmoves > 2)
     */
    inline void executeStep02_MakeCounterMove(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 2: Make counter-move");
        
        // Wait for snackbar tutorial message
        IM_CHECK(waitForSnackbarTutorialMessage(ctx, 5.0f));
        
        // Make a pawn move e7-e5 by clicking on board squares
        // Square IDs are "cell_N" where N = file + rank * 8
        ctx->ItemClick("**/cell_36");  // Click e5 (target)
        ctx->Yield();
        
        // Wait for progress to advance to step 3 (engine responds automatically)
        IM_CHECK(waitForTutorialProgress(ctx, 3, 30.0f));
        ctx->LogInfo("Step 2 completed: Counter-move made");
    }

    /**
     * @brief Step 3: Click Play again - engine switches to black side
     * Tutorial message: "Click 'Play' again..."
     * Completion: Engine plays for black (halfmoves > 3)
     */
    inline void executeStep03_ClickPlayAgain(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 3: Click Play again");
        
        // Wait for snackbar tutorial message
        IM_CHECK(waitForSnackbarTutorialMessage(ctx, 5.0f));
        
        // Click Play button again
        ctx->ItemClick("**/Board/Play");
        ctx->Yield();

        // Wait for progress to advance to step 4 (engine plays for black)
        IM_CHECK(waitForTutorialProgress(ctx, 4, 30.0f));
        ctx->LogInfo("Step 3 completed: Engine switched sides");
    }

    /**
     * @brief Step 4: Click Stop and make a manual move
     * Tutorial message: "Click 'Stop' to end the engine play. After that, make another move manually."
     * Completion: User clicks Stop and makes a move (halfmoves > 4)
     */
    inline void executeStep04_StopAndManualMove(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 4: Click Stop and make manual move");
        
        // Wait for snackbar tutorial message
        IM_CHECK(waitForSnackbarTutorialMessage(ctx, 5.0f));
        
        // Click Stop button
        ctx->ItemClick("**/Board/Stop");
        ctx->Yield();
        // White side to move after "play move"
        // Make a manual pawn move a2-a4
        // a4 = file 0 + rank 3 * 8 = 24
        ctx->ItemClick("**/cell_24");  // Click a4 (target)
        ctx->Yield();

        // Wait for progress to advance to step 5
        IM_CHECK(waitForTutorialProgress(ctx, 5, 5.0f));
        ctx->LogInfo("Step 4 completed: Stopped and made manual move");
    }

    /**
     * @brief Step 5: Click Analyze - both engines analyze the position
     * Tutorial message: "Click 'Analyze' to make both engines analyze the position..."
     * Completion: User clicks Analyze
     */
    inline void executeStep05_ClickAnalyze(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 5: Click Analyze");
        
        // Wait for snackbar tutorial message
        IM_CHECK(waitForSnackbarTutorialMessage(ctx, 5.0f));
        
        // Click Analyze button
        ctx->ItemClick("**/Board/Analyze");
        ctx->Yield();

        // Wait for progress to advance to step 6
        IM_CHECK(waitForTutorialProgress(ctx, 6, 5.0f));
        ctx->LogInfo("Step 5 completed: Analysis started");
    }

    /**
     * @brief Step 6: Click Stop to end analysis
     * Tutorial message: "Click 'Stop' again to end the analysis."
     * Completion: User clicks Stop
     */
    inline void executeStep06_StopAnalysis(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 6: Stop analysis");
        
        // Wait for snackbar tutorial message
        IM_CHECK(waitForSnackbarTutorialMessage(ctx, 5.0f));
        
        // Click Stop button
        ctx->ItemClick("**/Board/Stop");
        ctx->Yield();

        // Wait for progress to advance to step 7
        IM_CHECK(waitForTutorialProgress(ctx, 7, 5.0f));
        ctx->LogInfo("Step 6 completed: Analysis stopped");
    }

    /**
     * @brief Step 7: Click Auto - both engines play against each other
     * Tutorial message: "Click 'Auto' to make both engines play against each other automatically."
     * Completion: User clicks Auto
     */
    inline void executeStep07_ClickAuto(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 7: Click Auto");
        
        // Wait for snackbar tutorial message
        IM_CHECK(waitForSnackbarTutorialMessage(ctx, 5.0f));
        
        // Click Auto button
        ctx->ItemClick("**/Board/Auto");
        ctx->Yield();

        // Wait for progress to advance to step 8
        IM_CHECK(waitForTutorialProgress(ctx, 8, 5.0f));
        ctx->LogInfo("Step 7 completed: Auto-play started");
    }

    /**
     * @brief Step 8: Click Stop to end auto-play
     * Tutorial message: "Click 'Stop' one more time to end the auto-play."
     * Completion: User clicks Stop
     */
    inline void executeStep08_StopAutoPlay(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 8: Stop auto-play");
        
        // Wait for snackbar tutorial message
        IM_CHECK(waitForSnackbarTutorialMessage(ctx, 5.0f));
        
        // Click Stop button
        ctx->ItemClick("**/Board/Stop");
        ctx->Yield();

        // Wait for progress to advance to step 9 (completion)
        IM_CHECK(waitForTutorialProgress(ctx, 9, 5.0f));
        ctx->LogInfo("Step 8 completed: Auto-play stopped");
    }

    /**
     * @brief Step 9: Tutorial completion - wait for success message to be shown
     * Tutorial message: "Board Controls Complete! Well done!..."
     * Completion: Tutorial finishes
     */
    inline void executeStep09_TutorialComplete(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 9: Verify tutorial completion");
        
        // Wait for snackbar tutorial message (success message)
        IM_CHECK(waitForSnackbarTutorialMessage(ctx, 5.0f));
        
        // Verify tutorial is finished (not running anymore - counter is 0 or > max)
        auto& tutorial = QaplaWindows::Tutorial::instance();
        const auto& entry = tutorial.getEntry(QaplaWindows::Tutorial::TutorialName::BoardWindow);
        IM_CHECK(!entry.running());
        
        ctx->LogInfo("Step 9 completed: Tutorial finished successfully");
    }

} // namespace QaplaTest::BoardWindowTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
