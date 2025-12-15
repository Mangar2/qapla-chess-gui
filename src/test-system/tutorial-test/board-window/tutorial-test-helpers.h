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

namespace QaplaTest::BoardWindowTutorialTest {

    using namespace TutorialTestCommon;

    /**
     * @brief Cleans up board window tutorial state by resetting the static progress counter
     */
    inline void cleanupBoardWindowState() {
        QaplaWindows::BoardWindow::tutorialBoardProgress_ = 0;
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
