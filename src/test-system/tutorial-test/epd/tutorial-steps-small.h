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

namespace QaplaTest::EpdTutorialTest {

    // Step 1: Open EPD Tab
    inline void executeStep01_OpenEpdTab(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 1: Open EPD Tab");
        
        // Click EPD tab
        ctx->ItemClick("**/QaplaTabBar/###Epd");
        ctx->Yield();

        // Click Continue and advance to step 2
        clickContinueAndAdvance(ctx, 2);
    }

    // Step 4: Start Analysis
    inline void executeStep04_StartAnalysis(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 4: Start Analysis");
        ctx->Yield(2);
        auto& epdData = QaplaWindows::EpdData::instance();

        // Click Analyze button (Run/Stop)
        ctx->ItemClick("**/###Epd/RunStop");
        ctx->Yield();
        
        // Wait for analysis to start
        IM_CHECK(waitForAnalysisRunning(ctx, 10.0f));
        IM_CHECK(epdData.isRunning());

        clickContinueAndAdvance(ctx, 5);
    }

    // Step 5: Stop Analysis
    inline void executeStep05_StopAnalysis(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 5: Stop Analysis");
        
        auto& epdData = QaplaWindows::EpdData::instance();

        // A button needs a moment before it takes a click, and how long varies from run to
        // run -- which is why this test failed only in company. Frames, not sleep.
        ctx->SleepNoSkip(1.0f, QaplaTest::Common::FRAME_STEP);

        // Click Stop button (same as Run/Stop)
        ctx->ItemClick("**/###Epd/RunStop");
        ctx->Yield();
        
        // Wait for analysis to stop
        IM_CHECK(waitForAnalysisStopped(ctx, 10.0f));
        IM_CHECK(!epdData.isRunning());

        clickContinueAndAdvance(ctx, 6);
    }

    // Step 6: Continue Analysis
    inline void executeStep06_ContinueAnalysis(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 6: Continue Analysis");
        
        auto& epdData = QaplaWindows::EpdData::instance();

        // Click Continue button (Run/Stop again)
        ctx->ItemClick("**/###Epd/RunStop");
        ctx->Yield();
        
        // Wait for analysis to start
        IM_CHECK(waitForAnalysisRunning(ctx, 10.0f));
        IM_CHECK(epdData.isRunning());

        clickContinueAndAdvance(ctx, 7);
    }

    // Step 7: Grace Stop
    inline void executeStep07_GraceStop(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 7: Grace Stop");

        // A button needs a moment before it takes a click, and how long varies from run to
        // run -- which is why this test failed only in company. Frames, not sleep.
        ctx->SleepNoSkip(1.0f, QaplaTest::Common::FRAME_STEP);

        // Click Grace button
        ctx->ItemClick("**/###Epd/Grace");
        ctx->Yield();

        clickContinueAndAdvance(ctx, 8);
    }

    // Step 8: Wait for Stop and Clear
    inline void executeStep08_WaitAndClear(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 8: Wait for Stop and Clear");
        
        auto& epdData = QaplaWindows::EpdData::instance();

        // The graceful stop lets the positions in flight finish, and with a ten-second limit
        // each that takes a moment. Clear only becomes available once it has stopped.
        IM_CHECK(waitForAnalysisStopped(ctx, 60.0f));
        IM_CHECK(!epdData.isRunning());

        // Clear only becomes clickable once the run has actually produced results, and stopped
        // does not yet mean scored: the positions in flight are still being written down. Two
        // seconds of frames, not of sleep, so the application keeps running while we wait.
        ctx->SleepNoSkip(2.0f, QaplaTest::Common::FRAME_STEP);

        // Click Clear button
        ctx->ItemClick("**/###Epd/Clear");
        ctx->Yield();

        // Waited for as a state, not as five seconds of hope: the tutorial only asks to continue
        // once the results are actually cleared, and under load that took longer than the
        // user-input wait allowed -- which is why this passed alone and failed in a full run.
        IM_CHECK(QaplaTest::Common::waitForCondition(ctx, []() {
            return QaplaWindows::EpdData::instance().state == QaplaWindows::EpdData::State::Cleared;
        }, 30.0f));

        clickContinueAndAdvance(ctx, 9);
    }

    // Step 9: Start Fresh Analysis
    inline void executeStep09_StartFreshAnalysis(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 9: Start Fresh Analysis");
        
        auto& epdData = QaplaWindows::EpdData::instance();

        // Click Analyze button
        ctx->ItemClick("**/###Epd/RunStop");
        ctx->Yield();
        
        // Wait for analysis to start
        IM_CHECK(waitForAnalysisRunning(ctx, 10.0f));
        IM_CHECK(epdData.isRunning());

        clickContinueAndAdvance(ctx, 10);
    }

    // Step 10: Set Concurrency
    inline void executeStep10_SetConcurrency(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 10: Set Concurrency to 10");

        // Set concurrency to 10
        ctx->ItemInputValue("**/###Concurrency", 10);
        ctx->Yield();
        
        // Verify concurrency is set
        IM_CHECK_EQ(QaplaWindows::EpdData::instance().getExternalConcurrency(), 10U);

        // Progress advances automatically
        ctx->Yield();
        ctx->ItemClick("**/###Epd/RunStop");
        ctx->Yield();
    }

    // Step 11: Tutorial Complete
    inline void executeStep11_TutorialComplete(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 11: Tutorial Complete");
        ctx->Yield(2);
        
        // Wait for tutorial to finish
        float waited = 0.0f;
        while (QaplaWindows::EpdWindow::tutorialProgress_ == 11 && waited < 5.0f) {
            ctx->SleepNoSkip(0.1f, 0.1f);
            waited += 0.1f;
        }
        ctx->ItemClick("**/###Close");
        ctx->LogInfo("EPD Tutorial Complete!");
    }

} // namespace QaplaTest::EpdTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
