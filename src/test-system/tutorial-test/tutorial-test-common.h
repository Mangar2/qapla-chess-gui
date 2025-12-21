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

#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "tutorial.h"
#include "snackbar.h"
#include "engine-worker-factory.h"
#include "../test-common.h"

namespace QaplaTest::TutorialTestCommon {

    using QaplaTest::Common::waitForCondition;

    /**
     * @brief Checks if at least two engines are available
     * @return true if at least 2 engines are configured
     */
    inline bool hasEnginesAvailable() {
        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
        auto configs = configManager.getAllConfigs();
        return configs.size() >= 2;
    }

    /**
     * @brief Navigates to the Chatbot window
     * @param ctx The ImGui test context
     */
    inline void navigateToChatbot(ImGuiTestContext* ctx) {
        ctx->ItemClick("**/###Chatbot");
        ctx->Yield();
    }

    /**
     * @brief Waits for a tutorial progress counter to reach a specific step
     * @param ctx The ImGui test context
     * @param progressCounter Reference to the tutorial progress counter
     * @param targetProgress The target progress value to wait for
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if target progress was reached
     */
    inline bool waitForTutorialProgress(ImGuiTestContext* ctx, const uint32_t& progressCounter, 
                                        uint32_t targetProgress, float maxWaitSeconds = 5.0f) {
        return waitForCondition(ctx, [&progressCounter, targetProgress]() {
            return progressCounter >= targetProgress;
        }, maxWaitSeconds);
    }

    /**
     * @brief Waits for the snackbar tutorial message to be visible
     * @param ctx The ImGui test context
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if a tutorial snackbar message became visible
     */
    inline bool waitForSnackbarTutorialMessage(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f) {
        return waitForCondition(ctx, []() {
            return QaplaWindows::SnackbarManager::instance().isTutorialMessageVisible();
        }, maxWaitSeconds);
    }

    /**
     * @brief Waits for tutorial to request user input (when Continue button should appear)
     * @param ctx The ImGui test context
     * @param maxWaitSeconds Maximum wait time in real seconds
     * @return true if tutorial is waiting for user input
     */
    inline bool waitForTutorialUserInput(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f) {
        return waitForCondition(ctx, []() {
            return QaplaWindows::Tutorial::instance().doWaitForUserInput();
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
     * @brief Clicks Continue button after waiting for it to appear
     * @param ctx The ImGui test context
     */
    inline void clickContinue(ImGuiTestContext* ctx) {
        IM_CHECK(waitForTutorialUserInput(ctx, 5.0f));
        IM_CHECK(waitForContinueButton(ctx, 5.0f));
        ctx->ItemClick("**/###Continue");
        ctx->Yield();
    }

} // namespace QaplaTest::TutorialTestCommon

#endif // IMGUI_ENABLE_TEST_ENGINE
