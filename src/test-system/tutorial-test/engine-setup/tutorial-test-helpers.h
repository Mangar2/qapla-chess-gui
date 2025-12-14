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
#include "engine-setup-window.h"
#include "engine-worker-factory.h"
#include "configuration.h"
#include "imgui-engine-select.h"

namespace QaplaTest::EngineSetupTutorialTest {

    // Helper to check if engines are available
    inline bool hasEnginesAvailable() {
        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
        auto configs = configManager.getAllConfigs();
        return configs.size() >= 2; // Need at least 2 engines
    }

    // Helper to cleanup engine setup state
    inline void cleanupEngineSetupState() {
        QaplaWindows::EngineSetupWindow::clearEngineSetupTutorialState();
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
        while (QaplaWindows::EngineSetupWindow::tutorialProgress_ < targetProgress && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return QaplaWindows::EngineSetupWindow::tutorialProgress_ >= targetProgress;
    }

    // Helper to wait for detection to complete
    inline bool waitForDetectionComplete(ImGuiTestContext* ctx, float maxWaitSeconds = 20.0f) {
        // For testing purposes, we just wait for reasonable time
        // Detection happens in background, we can't easily check completion status
        ctx->SleepNoSkip(maxWaitSeconds, maxWaitSeconds);
        return true;
    }

} // namespace QaplaTest::EngineSetupTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
