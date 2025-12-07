/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "test-system/test-manager.h"
#include "test-system/regression-tests.h"
#include "test-system/epd-chatbot-tests.h"
#include "test-system/tournament-chatbot/tournament-chatbot-tests.h"
#include "test-system/sprt-tournament-chatbot/sprt-tournament-chatbot-tests.h"
#include "test-system/tutorial-test/tournament/tutorial-tests.h"
#include "test-system/tutorial-test/epd/tutorial-tests.h"
#include <glad/glad.h>

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_ui.h"
#endif

namespace QaplaTest {

    void TestManager::init() {
        // Initialize Test Engine
#ifdef IMGUI_ENABLE_TEST_ENGINE
        engine_ = ImGuiTestEngine_CreateContext();
        ImGuiTestEngineIO& io = ImGuiTestEngine_GetIO(engine_);
        io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
        io.ConfigRunSpeed = ImGuiTestRunSpeed_Normal;
        io.ConfigNoThrottle = false;
        io.ScreenCaptureFunc = []([[maybe_unused]] ImGuiID viewport_id, int x, int y, int w, int h, unsigned int* pixels, [[maybe_unused]] void* user_data) {
            GLint last_buffer;
            glGetIntegerv(GL_READ_BUFFER, &last_buffer);
            glReadBuffer(GL_FRONT);
            glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glReadBuffer(last_buffer);

            // Flip Y
            int line_size = w * 4;
            unsigned int* temp_line = new unsigned int[w];
            for (int i = 0; i < h / 2; i++) {
                memcpy(temp_line, pixels + i * w, line_size);
                memcpy(pixels + i * w, pixels + (h - 1 - i) * w, line_size);
                memcpy(pixels + (h - 1 - i) * w, temp_line, line_size);
            }
            delete[] temp_line;
            return true;
        };

        ImGuiTestEngine_Start(engine_, ImGui::GetCurrentContext());
        ImGuiTestEngine_InstallDefaultCrashHandler();

        registerRegressionTests(engine_);
        registerEpdChatbotTests(engine_);
        registerTournamentChatbotTests(engine_);
        registerSprtTournamentChatbotTests(engine_);
        registerTutorialTests(engine_);
        registerEpdTutorialTests(engine_);
#endif
    }

    void TestManager::onPostSwap() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (engine_ != nullptr) {
            ImGuiTestEngine_PostSwap(engine_);
        }
#endif
    }

    void TestManager::drawDebugWindows() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (engine_ != nullptr) {
            ImGuiTestEngine_ShowTestEngineWindows(engine_, nullptr);
        }
#endif
    }

    void TestManager::stop() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (engine_ != nullptr) {
            ImGuiTestEngine_Stop(engine_);
        }
#endif
    }

    void TestManager::destroy() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (engine_ != nullptr) {
            ImGuiTestEngine_DestroyContext(engine_);
            engine_ = nullptr;
        }
#endif
    }

} // namespace QaplaTest
