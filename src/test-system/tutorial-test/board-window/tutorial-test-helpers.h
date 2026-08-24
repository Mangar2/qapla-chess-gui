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
#include "board-window.h"
#include "chatbot/chatbot-window.h"

namespace QaplaTest::BoardWindowTutorialTest {

    using namespace TutorialTestCommon;

    /**
     * @brief Cleans up board window tutorial state by resetting the static progress counter
     */
    /**
     * @brief Picks two engines for the board itself, through its Config dialog.
     *
     * A board has an engine selection of its own; the catalog, the tournament and the SPRT view
     * each have theirs, and none of them feeds this one. Without it the Play button has nothing
     * to ask for a move, which looks exactly like an engine that will not answer.
     */
    inline bool selectEnginesOnBoard(ImGuiTestContext* ctx) {
        // The Config button belongs to the engine pane beside the board, not to the board
        // itself, so it is looked for anywhere rather than under a guessed path.
        ctx->ItemClick("**/Config");
        ctx->Yield(10);

        // Inside the dialog window, so the checkboxes are looked for there rather than anywhere:
        // an item found in a popup that is not the current reference cannot be hovered.
        ctx->SetRef("Select Engines");
        ctx->ItemClick("**/engineSettings/$$0/##select");
        ctx->Yield(2);
        ctx->ItemClick("**/engineSettings/$$1/##select");
        ctx->Yield(2);
        ctx->ItemClick("**/###Ok");
        ctx->SetRef("");
        ctx->Yield(10);
        return true;
    }

    inline void cleanupBoardWindowState() {
        QaplaWindows::BoardWindow::tutorialBoardProgress_ = 0;
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
     * @brief Waits for the board window tutorial progress to reach a specific step
     * @param ctx The ImGui test context
     * @param targetProgress The target progress value to wait for
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if target progress was reached
     */
    inline bool waitForTutorialProgress(ImGuiTestContext* ctx, uint32_t targetProgress, float maxWaitSeconds = 5.0f) {
        bool result = TutorialTestCommon::waitForTutorialProgress(
            ctx, QaplaWindows::BoardWindow::tutorialBoardProgress_, targetProgress, maxWaitSeconds);
        
        if (targetProgress == 0) {
            return QaplaWindows::BoardWindow::tutorialBoardProgress_ == 0;
        }
        return result;
    }

    /**
     * @brief Switches to the board view (Board 1 tab)
     * @param ctx The ImGui test context
     */
    inline void switchToBoardView(ImGuiTestContext* ctx) {
        ctx->ItemClick("**/QaplaTabBar/###Board 1");
        ctx->Yield();
    }

} // namespace QaplaTest::BoardWindowTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
