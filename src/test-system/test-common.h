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
 */

#pragma once

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include <functional>
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

namespace QaplaTest::Common {

    /**
     * @brief Frame step for SleepNoSkip - corresponds to ~60fps
     * This ensures proper real-time waiting in polling loops.
     */
    constexpr float FRAME_STEP = 1.0f / 60.0f;

    /**
     * @brief Default sleep interval for polling loops (100ms real time)
     */
    constexpr float DEFAULT_SLEEP_INTERVAL = 0.1f;

    /**
     * @brief Waits for a condition to become true with proper real-time waiting.
     *
     * The function repeatedly evaluates the given condition and uses
     * ImGui test engine's SleepNoSkip to advance real time in a
     * deterministic way that is compatible with fast/skip modes.
     *
     * @param ctx The ImGui test context.
     * @param condition Callback that returns true when the condition is met.
     * @param maxWaitSeconds Maximum wait time in real seconds.
     * @param sleepIntervalSeconds Sleep interval in real seconds between checks.
     * @return true if the condition was met within the timeout.
     */
    inline bool waitForCondition(
        ImGuiTestContext* ctx,
        const std::function<bool()>& condition,
        float maxWaitSeconds = 5.0f,
        float sleepIntervalSeconds = DEFAULT_SLEEP_INTERVAL
    ) {
        float waited = 0.0f;

        if (condition()) {
            return true;
        }

        while (waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepIntervalSeconds, FRAME_STEP);
            waited += sleepIntervalSeconds;

            if (condition()) {
                return true;
            }
        }

        return false;
    }

} // namespace QaplaTest::Common

#endif // IMGUI_ENABLE_TEST_ENGINE
