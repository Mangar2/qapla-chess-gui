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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include "llm-epd-tool-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

#include "epd-data.h"
#include "llm/gui-tool-registry.h"
#include <engine-handling/engine-worker-factory.h>

#include <atomic>
#include <filesystem>
#include <thread>

namespace QaplaTest {

    namespace {
        // Same cross-thread handoff pattern as llm-tournament-tool-tests.cpp's
        // callToolAndYield() -- duplicated here so this file stays independent.
        QaplaLlm::GuiToolResult callToolAndYield(
            ImGuiTestContext* ctx, const std::string& name, const QaplaTester::Json::JsonValue& arguments) {
            std::atomic<bool> done{false};
            QaplaLlm::GuiToolResult result;

            std::thread worker([&]() {
                result = QaplaLlm::GuiToolRegistry::instance().callTool(name, arguments.stringify());
                done.store(true, std::memory_order_release);
            });
            while (!done.load(std::memory_order_acquire)) {
                ctx->Yield();
            }
            worker.join();

            return result;
        }

        // Same test fixture tournament/SPRT tests use -- it's a valid EPD file too.
        std::string getTestEpdPath() {
            auto currentPath = std::filesystem::current_path();
            auto testDataPath = currentPath / "src" / "test-system" / "test-data" / "wmtest.epd";
            return testDataPath.string();
        }

        bool hasEnginesAvailable() {
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            return configManager.getAllConfigs().size() >= 1;
        }

        void cleanupEpdState() {
            auto& epdData = QaplaWindows::EpdData::instance();
            if (epdData.isRunning() || epdData.isStarting()) {
                epdData.stopPool(false);
            }
            epdData.clear();
        }

        bool waitForAnalysisRunning(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f) {
            auto& epdData = QaplaWindows::EpdData::instance();
            float waited = 0.0f;
            constexpr float sleepInterval = 0.1f;
            while (!epdData.isRunning() && waited < maxWaitSeconds) {
                ctx->SleepNoSkip(sleepInterval, sleepInterval);
                waited += sleepInterval;
            }
            return epdData.isRunning();
        }

        bool waitForAnalysisStopped(ImGuiTestContext* ctx, float maxWaitSeconds = 10.0f) {
            auto& epdData = QaplaWindows::EpdData::instance();
            float waited = 0.0f;
            constexpr float sleepInterval = 0.1f;
            while (!epdData.isStopped() && waited < maxWaitSeconds) {
                ctx->SleepNoSkip(sleepInterval, sleepInterval);
                waited += sleepInterval;
            }
            return epdData.isStopped();
        }
    }

    void registerLlmEpdToolTests(ImGuiTestEngine* engine) {
        ImGuiTest* t = nullptr;

        // -----------------------------------------------------------------
        // Test: select_epd_engines / configure_epd / get_epd_status, called directly
        // through GuiToolRegistry (no LLM). No analysis needs to actually run for this.
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Llm/Epd/Tools", "ConfigureEpdViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: select_epd_engines / configure_epd / get_epd_status via GuiToolRegistry ===");

            cleanupEpdState();
            IM_CHECK(hasEnginesAvailable());

            auto configs = QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs();
            IM_CHECK(!configs.empty());

            ctx->LogInfo("Step 1: select_epd_engines");
            auto selectArgs = QaplaTester::Json::JsonValue::object();
            auto engineNames = QaplaTester::Json::JsonValue::array();
            engineNames.push_back(configs[0].getName());
            selectArgs["engines"] = engineNames;
            auto selectResult = callToolAndYield(ctx, "select_epd_engines", selectArgs);
            IM_CHECK(selectResult.success);

            auto& epdData = QaplaWindows::EpdData::instance();
            IM_CHECK(epdData.getEngineSelect().getSelectedEngines().size() == 1);

            // Unknown engine name must be rejected, not silently ignored.
            auto badArgs = QaplaTester::Json::JsonValue::object();
            auto badNames = QaplaTester::Json::JsonValue::array();
            badNames.push_back("Definitely Not An Installed Engine");
            badArgs["engines"] = badNames;
            IM_CHECK(!callToolAndYield(ctx, "select_epd_engines", badArgs).success);

            ctx->LogInfo("Step 2: configure_epd");
            auto configureArgs = QaplaTester::Json::JsonValue::object();
            configureArgs["epd_file"] = getTestEpdPath();
            configureArgs["max_time_seconds"] = 5.0;
            configureArgs["min_time_seconds"] = 1.0;
            configureArgs["seen_plies"] = 2.0;
            configureArgs["concurrency"] = 1.0;
            auto configureResult = callToolAndYield(ctx, "configure_epd", configureArgs);
            IM_CHECK(configureResult.success);
            IM_CHECK(epdData.config().filepath == getTestEpdPath());
            IM_CHECK(epdData.config().maxTimeInS == 5);
            IM_CHECK(epdData.config().minTimeInS == 1);
            IM_CHECK(epdData.config().seenPlies == 2);

            // A nonexistent EPD file must be rejected, not silently accepted.
            auto badFileArgs = QaplaTester::Json::JsonValue::object();
            badFileArgs["epd_file"] = "/no/such/file.epd";
            auto badFileResult = callToolAndYield(ctx, "configure_epd", badFileArgs);
            IM_CHECK(!badFileResult.success);
            // Rejecting the bad path must not have reset the previously-configured one.
            IM_CHECK(epdData.config().filepath == getTestEpdPath());

            ctx->LogInfo("Step 3: get_epd_status reflects all of the above");
            auto status = callToolAndYield(ctx, "get_epd_status", QaplaTester::Json::JsonValue::object());
            IM_CHECK(status.success);
            IM_CHECK(status.content.find(configs[0].getName()) != std::string::npos);
            IM_CHECK(status.content.find(getTestEpdPath()) != std::string::npos);

            cleanupEpdState();
            ctx->LogInfo("=== Test ConfigureEpdViaRegistry PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: start_epd_analysis / stop_epd_analysis / clear_epd_result /
        // show_epd_result, called directly through GuiToolRegistry (no LLM).
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Llm/Epd/Tools", "StartStopClearShowEpdResultViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: start/stop/clear/show_epd_result via GuiToolRegistry ===");

            cleanupEpdState();
            IM_CHECK(hasEnginesAvailable());

            // stop_epd_analysis must fail cleanly when nothing is running.
            IM_CHECK(!callToolAndYield(ctx, "stop_epd_analysis", QaplaTester::Json::JsonValue::object()).success);

            auto configs = QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs();
            IM_CHECK(!configs.empty());

            auto selectArgs = QaplaTester::Json::JsonValue::object();
            auto engineNames = QaplaTester::Json::JsonValue::array();
            engineNames.push_back(configs[0].getName());
            selectArgs["engines"] = engineNames;
            IM_CHECK(callToolAndYield(ctx, "select_epd_engines", selectArgs).success);

            auto configureArgs = QaplaTester::Json::JsonValue::object();
            configureArgs["epd_file"] = getTestEpdPath();
            configureArgs["max_time_seconds"] = 5.0;
            configureArgs["min_time_seconds"] = 5.0;
            IM_CHECK(callToolAndYield(ctx, "configure_epd", configureArgs).success);

            IM_CHECK(callToolAndYield(ctx, "start_epd_analysis", QaplaTester::Json::JsonValue::object()).success);
            IM_CHECK(waitForAnalysisRunning(ctx, 20.0f));

            // show_epd_result must succeed while running, even before any position finished.
            auto resultWhileRunning = callToolAndYield(ctx, "show_epd_result", QaplaTester::Json::JsonValue::object());
            IM_CHECK(resultWhileRunning.success);

            // Let the engine settle into the first position before stopping -- see the
            // identical wait in llm-tournament-tool-tests.cpp / llm-sprt-tool-tests.cpp
            // (prevents crash/slow shutdown from a rapid start/stop of the engine process).
            ctx->SleepNoSkip(0.5f, 0.1f);

            ctx->LogInfo("Step: stop_epd_analysis(mode=abrupt)");
            auto stopArgs = QaplaTester::Json::JsonValue::object();
            stopArgs["mode"] = "abrupt";
            auto stopResult = callToolAndYield(ctx, "stop_epd_analysis", stopArgs);
            IM_CHECK(stopResult.success);
            IM_CHECK(waitForAnalysisStopped(ctx, 15.0f));
            IM_CHECK(!QaplaWindows::EpdData::instance().isRunning());

            ctx->LogInfo("Step: clear_epd_result");
            auto clearResult = callToolAndYield(ctx, "clear_epd_result", QaplaTester::Json::JsonValue::object());
            IM_CHECK(clearResult.success);

            auto resultAfterClear = callToolAndYield(ctx, "show_epd_result", QaplaTester::Json::JsonValue::object());
            IM_CHECK(resultAfterClear.success);
            IM_CHECK(resultAfterClear.content.find("No EPD analysis results") != std::string::npos);
            IM_CHECK(!static_cast<bool>(resultAfterClear.renderWidget));

            // clear_epd_result on an already-clear state must still succeed, not error.
            IM_CHECK(callToolAndYield(ctx, "clear_epd_result", QaplaTester::Json::JsonValue::object()).success);

            cleanupEpdState();
            ctx->LogInfo("=== Test StartStopClearShowEpdResultViaRegistry PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: start_epd_analysis after a config change post-stop must fail with
        // AI-actionable guidance to call clear_epd_result first (see EpdData::mayAnalyze()'s
        // configChanged()+Stopped precondition -- unique to EPD, no tournament/SPRT
        // equivalent, previously undocumented from the AI's perspective).
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Llm/Epd/Tools", "StartAfterConfigChangeGuidesTowardClearViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: start_epd_analysis after config change guides toward clear_epd_result ===");

            cleanupEpdState();
            IM_CHECK(hasEnginesAvailable());

            auto configs = QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs();
            IM_CHECK(!configs.empty());

            auto selectArgs = QaplaTester::Json::JsonValue::object();
            auto engineNames = QaplaTester::Json::JsonValue::array();
            engineNames.push_back(configs[0].getName());
            selectArgs["engines"] = engineNames;
            IM_CHECK(callToolAndYield(ctx, "select_epd_engines", selectArgs).success);

            auto configureArgs = QaplaTester::Json::JsonValue::object();
            configureArgs["epd_file"] = getTestEpdPath();
            configureArgs["max_time_seconds"] = 5.0;
            configureArgs["min_time_seconds"] = 5.0;
            IM_CHECK(callToolAndYield(ctx, "configure_epd", configureArgs).success);

            IM_CHECK(callToolAndYield(ctx, "start_epd_analysis", QaplaTester::Json::JsonValue::object()).success);
            IM_CHECK(waitForAnalysisRunning(ctx, 20.0f));
            ctx->SleepNoSkip(0.5f, 0.1f); // see the identical wait above

            auto stopArgs = QaplaTester::Json::JsonValue::object();
            stopArgs["mode"] = "abrupt";
            IM_CHECK(callToolAndYield(ctx, "stop_epd_analysis", stopArgs).success);
            IM_CHECK(waitForAnalysisStopped(ctx, 15.0f));

            // Change the configuration without clearing first -- state is Stopped, not Cleared.
            auto changeArgs = QaplaTester::Json::JsonValue::object();
            changeArgs["max_time_seconds"] = 6.0;
            IM_CHECK(callToolAndYield(ctx, "configure_epd", changeArgs).success);

            auto blockedResult = callToolAndYield(ctx, "start_epd_analysis", QaplaTester::Json::JsonValue::object());
            IM_CHECK(!blockedResult.success);
            IM_CHECK(blockedResult.content.find("clear_epd_result") != std::string::npos);

            // Following that exact guidance must actually unblock a fresh start.
            IM_CHECK(callToolAndYield(ctx, "clear_epd_result", QaplaTester::Json::JsonValue::object()).success);
            IM_CHECK(callToolAndYield(ctx, "start_epd_analysis", QaplaTester::Json::JsonValue::object()).success);
            IM_CHECK(waitForAnalysisRunning(ctx, 20.0f));
            ctx->SleepNoSkip(0.5f, 0.1f);

            cleanupEpdState();
            ctx->LogInfo("=== Test StartAfterConfigChangeGuidesTowardClearViaRegistry PASSED ===");
        };
    }

} // namespace QaplaTest
#endif
