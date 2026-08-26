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
#include "callback-manager.h"
#include "interactive-board-window.h"
#include "chatbot/chatbot-window.h"

namespace QaplaTest::BoardWindowTutorialTest {

    using namespace TutorialTestCommon;

    /**
     * @brief Cleans up board window tutorial state by resetting the static progress counter
     */
    /**
     * @brief Gives the board two playing engines, on the board's own data.
     *
     * A board keeps an engine selection of its own; the catalog, the tournament and the SPRT view
     * each have theirs and none of them feeds this one. Without it the Play button has nothing to
     * ask for a move, which looks exactly like an engine that will not answer -- and every step of
     * the tutorial then waits for a move that is never coming.
     *
     * Set here rather than clicked through the "Select Engines" dialog: this is a precondition of
     * the test, not the thing it is testing, and setActiveEngines() is the same call the dialog's
     * "Use" button makes.
     */
    inline bool selectEnginesOnBoard(ImGuiTestContext* ctx) {
        auto& manager = QaplaWindows::InteractiveBoardWindow::getInstanceManager();
        const auto ids = manager.getKeys();
        if (ids.empty()) {
            ctx->LogError("No board to give engines to.");
            return false;
        }
        auto* board = QaplaWindows::InteractiveBoardWindow::getBoard(ids.front());
        if (board == nullptr) {
            ctx->LogError("Board %u is registered but not there.", ids.front());
            return false;
        }

        std::vector<QaplaTester::EngineConfig> engines;
        for (const auto& config : QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs()) {
            auto selected = config;
            selected.setSelected(true);
            engines.push_back(selected);
            if (engines.size() == 2) {
                break;
            }
        }
        if (engines.size() < 2) {
            ctx->LogError("The catalog holds %zu engines, the board needs two.", engines.size());
            return false;
        }

        board->getEngineSelect().setEngineConfigurations(engines);
        board->setActiveEngines();
        ctx->Yield(2);
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
    /**
     * @brief Brings the first board to the front and waits until it is really being drawn.
     *
     * Both ways at once, because neither alone was enough: the message is what the application
     * uses ("Switch to Board & Start" sends exactly this), and the click is what a person does.
     * On Windows the tab was there while the board window was not -- the tab had not become the
     * selected one -- and every step of the tutorial then failed looking for a button on a board
     * nobody was drawing.
     *
     * @return true once the board is on screen.
     */
    [[nodiscard]] inline bool switchToBoardView(ImGuiTestContext* ctx) {
        QaplaWindows::StaticCallbacks::message().invokeAll("switch_to_board_1");
        ctx->Yield(2);
        if (ctx->ItemExists("**/QaplaTabBar/###Board 1")) {
            ctx->ItemClick("**/QaplaTabBar/###Board 1");
        }
        ctx->Yield(2);
        return QaplaTest::Common::waitForCondition(ctx, [ctx]() {
            return ctx->ItemExists("**/Board/Play");
        }, 10.0f);
    }

} // namespace QaplaTest::BoardWindowTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
