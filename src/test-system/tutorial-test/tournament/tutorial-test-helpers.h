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
#include "tournament-window.h"
#include "tournament-data.h"
#include "chatbot/chatbot-window.h"

namespace QaplaTest::TutorialTest {

    using namespace TutorialTestCommon;

    /**
     * @brief Cleans up tournament state
     */
    inline void cleanupTournamentState() {
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        if (tournamentData.isRunning() || tournamentData.isStarting()) {
            tournamentData.stopPool(false);
        }
        tournamentData.clear(false);
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
     * @brief Gets the tutorial progress, as the tutorial itself counts it.
     *
     * Not TournamentWindow::tutorialProgress_: the window clears that back to zero the moment the
     * tutorial ends (see clearTournamentTutorialState()), so the last two steps cannot be observed
     * through it -- the counter runs 13, 14, 15, 0 within about two frames. That is what the
     * "progress of zero means finished" rule here used to paper over, and it papered over rather
     * more: while the tutorial had not started at all, every wait for a step passed at once and
     * the whole test reported success without a single step having run.
     *
     * The entry's own counter is monotone from start to finish and is what the tutorial advances.
     *
     * @param targetProgress Unused; kept so the callers read like the other suites.
     * @return The tutorial progress value
     */
    inline uint32_t getTutorialProgress([[maybe_unused]] uint32_t targetProgress) {
        return QaplaWindows::Tutorial::instance()
            .getEntry(QaplaWindows::Tutorial::TutorialName::Tournament).counter;
    }

    /**
     * @brief Waits for tutorial progress to reach a specific step
     * @param ctx The ImGui test context
     * @param targetProgress The target progress value to wait for
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if target progress was reached
     */
    inline bool waitForTutorialProgress(ImGuiTestContext* ctx, uint32_t targetProgress, float maxWaitSeconds = 5.0f) {
        return waitForCondition(ctx, [targetProgress]() {
            return getTutorialProgress(targetProgress) >= targetProgress;
        }, maxWaitSeconds);
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
            return QaplaWindows::TournamentWindow::highlightedSection_ == expectedSection;
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
        const bool advanced = waitForTutorialProgress(ctx, expectedProgress, 5.0f);
        if (!advanced) {
            // Which of the two it is decides where to look: the click not landing is a test
            // problem, the tutorial not moving after it landed is the window's.
            ctx->LogWarning("Continue clicked but progress stayed at %u (wanted %u), waiting for "
                            "user input: %d",
                getTutorialProgress(expectedProgress), expectedProgress,
                QaplaWindows::Tutorial::instance().doWaitForUserInput() ? 1 : 0);
        }
        IM_CHECK(advanced);
    }

} // namespace QaplaTest::TutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
