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

#include "../tutorial-test-common.h"
#include "tutorial.h"
#include "epd-window.h"
#include "epd-data.h"
#include "chatbot/chatbot-window.h"

namespace QaplaTest::EpdTutorialTest {

    using namespace TutorialTestCommon;

    /**
     * @brief Cleans up EPD state
     */
    inline void cleanupEpdState() {
        auto& epdData = QaplaWindows::EpdData::instance();
        if (epdData.isRunning() || epdData.isStarting()) {
            epdData.stopPool(false);
        }
        epdData.clear();
    }
    /**
     * @brief Resets the chatbot window to its initial state.
     * 
     * Clears all active and completed threads, returning to the main menu.
     * Call this at the start of each chatbot test to ensure clean state.
     * 
     * @param ctx The ImGui test context for logging.
     */
    inline void resetChatbotToInitialState(ImGuiTestContext* ctx) {
        ctx->LogInfo("Resetting chatbot to initial state");
        QaplaWindows::ChatBot::ChatbotWindow::instance()->reset();
        ctx->Yield();
    }
    /**
     * @brief Waits for tutorial progress to reach a specific step
     * @param ctx The ImGui test context
     * @param targetProgress The target progress value to wait for
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if target progress was reached
     */
    inline bool waitForTutorialProgress(ImGuiTestContext* ctx, uint32_t targetProgress, float maxWaitSeconds = 5.0f) {
        return TutorialTestCommon::waitForTutorialProgress(
            ctx, QaplaWindows::EpdWindow::tutorialProgress_, targetProgress, maxWaitSeconds);
    }

    /**
     * @brief Waits for tutorial to request user input (when Continue button should appear)
     * @param ctx The ImGui test context
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if tutorial is waiting for user input
     */
    inline bool waitForTutorialUserInput(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f) {
        auto& tutorial = QaplaWindows::Tutorial::instance();
        return waitForCondition(ctx, [&tutorial]() {
            return tutorial.doWaitForUserInput();
        }, maxWaitSeconds);
    }

    /**
     * @brief Waits for highlighted section to change
     * @param ctx The ImGui test context
     * @param expectedSection The expected section name
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if expected section is highlighted
     */
    inline bool waitForHighlightedSection(ImGuiTestContext* ctx, const std::string& expectedSection, float maxWaitSeconds = 5.0f) {
        return waitForCondition(ctx, [&expectedSection]() {
            return QaplaWindows::EpdWindow::highlightedSection_ == expectedSection;
        }, maxWaitSeconds);
    }

    /**
     * @brief Waits for "Continue" button to appear
     * @param ctx The ImGui test context
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if Continue button exists
     */
    inline bool waitForContinueButton(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f) {
        return waitForCondition(ctx, [ctx]() {
            return ctx->ItemExists("**/###Continue");
        }, maxWaitSeconds);
    }

    /**
     * @brief Clicks Continue button and waits for progress
     * @param ctx The ImGui test context
     * @param expectedProgress The expected progress after clicking
     */
    inline void clickContinueAndAdvance(ImGuiTestContext* ctx, uint32_t expectedProgress) {
        IM_CHECK(waitForTutorialUserInput(ctx, 5.0f));
        IM_CHECK(waitForContinueButton(ctx, 5.0f));
        ctx->ItemClick("**/###Continue");
        ctx->Yield();
        IM_CHECK(waitForTutorialProgress(ctx, expectedProgress, 5.0f));
    }

    /**
     * @brief Waits for EPD analysis to be running
     * @param ctx The ImGui test context
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if analysis is running
     */
    inline bool waitForAnalysisRunning(ImGuiTestContext* ctx, float maxWaitSeconds = 10.0f) {
        auto& epdData = QaplaWindows::EpdData::instance();
        return waitForCondition(ctx, [&epdData]() {
            return epdData.isRunning();
        }, maxWaitSeconds);
    }

    /**
     * @brief Waits for EPD analysis to be stopped
     * @param ctx The ImGui test context
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if analysis is stopped
     */
    inline bool waitForAnalysisStopped(ImGuiTestContext* ctx, float maxWaitSeconds = 10.0f) {
        auto& epdData = QaplaWindows::EpdData::instance();
        return waitForCondition(ctx, [&epdData]() {
            return !epdData.isRunning();
        }, maxWaitSeconds);
    }

} // namespace QaplaTest::EpdTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
