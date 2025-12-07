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
#include "epd-window.h"
#include "epd-data.h"
#include "engine-worker-factory.h"

namespace QaplaTest::EpdTutorialTest {

    // Helper to check if engines are available
    inline bool hasEnginesAvailable() {
        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
        auto configs = configManager.getAllConfigs();
        return configs.size() >= 2; // Need at least 2 for EPD analysis
    }

    // Helper to cleanup EPD state
    inline void cleanupEpdState() {
        auto& epdData = QaplaWindows::EpdData::instance();
        if (epdData.isRunning() || epdData.isStarting()) {
            epdData.stopPool(false);
        }
        epdData.clear();
    }

    // Helper to navigate to Chatbot window
    inline void navigateToChatbot(ImGuiTestContext* ctx) {
        ctx->ItemClick("**/###Chatbot");
        ctx->Yield();
    }

    // Helper to wait for tutorial progress to reach a specific step
    inline bool waitForTutorialProgress(ImGuiTestContext* ctx, uint32_t targetProgress, float maxWaitSeconds = 5.0f) {
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        while (QaplaWindows::EpdWindow::tutorialProgress_ < targetProgress && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return QaplaWindows::EpdWindow::tutorialProgress_ >= targetProgress;
    }

    // Helper to wait for tutorial to request user input (when Continue button should appear)
    inline bool waitForTutorialUserInput(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f) {
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        auto& tutorial = QaplaWindows::Tutorial::instance();
        while (!tutorial.doWaitForUserInput() && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return tutorial.doWaitForUserInput();
    }

    // Helper to wait for highlighted section to change
    inline bool waitForHighlightedSection(ImGuiTestContext* ctx, const std::string& expectedSection, float maxWaitSeconds = 5.0f) {
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        while (QaplaWindows::EpdWindow::highlightedSection_ != expectedSection && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return QaplaWindows::EpdWindow::highlightedSection_ == expectedSection;
    }

    // Helper to wait for "Continue" button to appear (when tutorial waits for user input)
    inline bool waitForContinueButton(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f) {
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        while (!ctx->ItemExists("**/###Continue") && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return ctx->ItemExists("**/###Continue");
    }

    // Helper to click Continue button and wait for progress
    inline void clickContinueAndAdvance(ImGuiTestContext* ctx, uint32_t expectedProgress) {
        IM_CHECK(waitForTutorialUserInput(ctx, 5.0f));
        IM_CHECK(waitForContinueButton(ctx, 5.0f));
        ctx->ItemClick("**/###Continue");
        ctx->Yield();
        IM_CHECK(waitForTutorialProgress(ctx, expectedProgress, 5.0f));
    }

    // Helper to wait for EPD analysis to be running
    inline bool waitForAnalysisRunning(ImGuiTestContext* ctx, float maxWaitSeconds = 10.0f) {
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        auto& epdData = QaplaWindows::EpdData::instance();
        while (!epdData.isRunning() && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return epdData.isRunning();
    }

    // Helper to wait for EPD analysis to be stopped
    inline bool waitForAnalysisStopped(ImGuiTestContext* ctx, float maxWaitSeconds = 10.0f) {
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        auto& epdData = QaplaWindows::EpdData::instance();
        while (epdData.isRunning() && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return !epdData.isRunning();
    }

} // namespace QaplaTest::EpdTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
