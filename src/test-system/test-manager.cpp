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
#include "test-system/tutorial-test/engine-setup/tutorial-tests.h"
#include "test-system/tutorial-test/board-window/tutorial-tests.h"
#include "test-system/llm-chat-tests.h"
#include "test-system/llm-app-tool-tests.h"
#include "os-helpers.h"

#include <filesystem>
#include <glad/glad.h>

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_exporters.h"
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

        // An unattended run has nobody to read the on-screen log, so it goes to the terminal and
        // to a file. Without this, a failure reports itself as "tested=4 success=0" and the one
        // thing worth knowing -- which step did not work -- stays in a window that has closed.
        if (QaplaHelpers::OsHelpers::getEnv("QAPLA_AUTO_RUN_TESTS")) {
            io.ConfigLogToTTY = true;
            exportResultsPath_ =
                (std::filesystem::path(QaplaHelpers::OsHelpers::getConfigDirectory())
                    / "gui-test-results.xml").string();
            io.ExportResultsFilename = exportResultsPath_.c_str();
            io.ExportResultsFormat = ImGuiTestEngineExportFormat_JUnitXml;
        }
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

        registerEpdChatbotTests(engine_);
        registerTournamentChatbotTests(engine_);
        registerSprtTournamentChatbotTests(engine_);
        registerTutorialTests(engine_);
        registerEpdTutorialTests(engine_);
        registerEngineSetupTutorialTests(engine_);
        registerBoardWindowTutorialTests(engine_);
        registerLlmChatTests(engine_);
        // The tool suites for tournament, SPRT, EPD and the running-status overview used to sit
        // here. They drove GuiToolRegistry::callTool from a worker thread while this loop drained
        // the queue -- which is what test/integration does now, out of process and over HTTP,
        // with a configuration directory of its own per test. Keeping both would have meant
        // maintaining the same checks twice, and these were the part of this suite that did not
        // report the same result twice running. What is left here is what only a window can be
        // asked: the chat itself, and the tools that need a person in front of it.
        registerLlmAppToolTests(engine_);
#endif
    }

    void TestManager::queueAllTests() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (engine_ != nullptr) {
            const auto filter = QaplaHelpers::OsHelpers::getEnv("QAPLA_TEST_FILTER");
            ImGuiTestEngine_QueueTests(engine_, ImGuiTestGroup_Tests,
                filter ? filter->c_str() : nullptr, ImGuiTestRunFlags_RunFromCommandLine);
        }
#endif
    }

    bool TestManager::isQueueEmpty() const {
#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (engine_ != nullptr) {
            return ImGuiTestEngine_IsTestQueueEmpty(engine_);
        }
#endif
        return true;
    }

    void TestManager::getResultSummary(int& tested, int& success, int& inQueue) const {
        tested = 0;
        success = 0;
        inQueue = 0;
#ifdef IMGUI_ENABLE_TEST_ENGINE
        if (engine_ != nullptr) {
            ImGuiTestEngineResultSummary summary;
            ImGuiTestEngine_GetResultSummary(engine_, &summary);
            tested = summary.CountTested;
            success = summary.CountSuccess;
            inQueue = summary.CountInQueue;
        }
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
