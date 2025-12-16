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
#include "engine-setup-window.h"
#include "configuration.h"
#include "imgui-engine-select.h"
#include "chatbot/chatbot-window.h"

namespace QaplaTest::EngineSetupTutorialTest {

    using namespace TutorialTestCommon;

    /**
     * @brief Cleans up engine setup tutorial state
     */
    inline void cleanupEngineSetupState() {
        QaplaWindows::EngineSetupWindow::clearEngineSetupTutorialState();
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
        bool result = TutorialTestCommon::waitForTutorialProgress(
            ctx, QaplaWindows::EngineSetupWindow::tutorialProgress_, targetProgress, maxWaitSeconds);
        
        return result;
    }

    /**
     * @brief Waits for detection to complete
     * @param ctx The ImGui test context
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true (always, detection happens in background)
     */
    inline bool waitForDetectionComplete(ImGuiTestContext* ctx, float maxWaitSeconds = 20.0f) {
        return waitForCondition(ctx, []() {
            auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
            return !capabilities.isDetecting();
        }, maxWaitSeconds);
    }

} // namespace QaplaTest::EngineSetupTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
