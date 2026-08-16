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

#include "llm-status-tool-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

#include "tournament-chatbot/tournament-test-helpers.h"
#include "sprt-tournament-chatbot/sprt-tournament-test-helpers.h"
#include "sprt-tournament-data.h"
#include "llm/gui-tool-registry.h"
#include <engine-handling/engine-worker-factory.h>

#include <atomic>
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
    }

    void registerLlmStatusToolTests(ImGuiTestEngine* engine) {
        ImGuiTest* t = nullptr;

        // -----------------------------------------------------------------
        // Test: get_status without a "type", called directly through GuiToolRegistry (no LLM).
        // Reproduces the exact scenario reported by the user: asking "is a tournament
        // running?" while only an SPRT test is actually running must not say "no" without
        // qualification -- see src/llm/tools/gui-tools-activity.cpp.
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Llm/Status/Tools", "GetRunningStatusViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: get_status via GuiToolRegistry ===");

            TournamentChatbot::cleanupTournamentState();
            SprtTournamentChatbot::cleanupSprtTournamentState();
            IM_CHECK(SprtTournamentChatbot::hasEnginesAvailable());

            ctx->LogInfo("Step 1: nothing running");
            auto idleStatus = callToolAndYield(ctx, "get_status", QaplaTester::Json::JsonValue::object());
            IM_CHECK(idleStatus.success);
            IM_CHECK(idleStatus.content.find("Nothing is currently running") != std::string::npos);
            IM_CHECK(idleStatus.content.find("no tournament") != std::string::npos);
            IM_CHECK(idleStatus.content.find("no SPRT test") != std::string::npos);

            auto configs = QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs();
            IM_CHECK(configs.size() >= 2);

            ctx->LogInfo("Step 2: start an SPRT test (tournament stays idle)");
            auto selectArgs = QaplaTester::Json::JsonValue::object();
            selectArgs["champion"] = configs[0].getName();
            selectArgs["challenger"] = configs[1].getName();
            IM_CHECK(callToolAndYield(ctx, "configure_sprt", selectArgs).success);

            auto configureArgs = QaplaTester::Json::JsonValue::object();
            configureArgs["openings_file"] = SprtTournamentChatbot::getTestOpeningPath();
            configureArgs["max_games"] = 4.0;
            IM_CHECK(callToolAndYield(ctx, "configure_sprt", configureArgs).success);

            auto sprtTypeArgs = QaplaTester::Json::JsonValue::object();
            sprtTypeArgs["type"] = "sprt";
            IM_CHECK(callToolAndYield(ctx, "start", sprtTypeArgs).success);
            IM_CHECK(SprtTournamentChatbot::waitForSprtTournamentRunning(ctx, 20.0f));

            ctx->LogInfo("Step 3: get_status must report SPRT running, no mention of a tournament");
            auto runningStatus = callToolAndYield(ctx, "get_status", QaplaTester::Json::JsonValue::object());
            IM_CHECK(runningStatus.success);
            IM_CHECK(runningStatus.content.find("a tournament") == std::string::npos);
            IM_CHECK(runningStatus.content.find("an SPRT test is running") != std::string::npos ||
                     runningStatus.content.find("an SPRT test is starting") != std::string::npos);

            // Let the engines settle before stopping -- see the identical wait elsewhere in
            // this session's GUI tests (prevents crash/slow shutdown from a rapid start/stop).
            ctx->SleepNoSkip(0.5f, 0.1f);
            QaplaWindows::SprtTournamentData::instance().stopPool(false);
            IM_CHECK(SprtTournamentChatbot::waitForSprtTournamentStopped(ctx, 15.0f));

            TournamentChatbot::cleanupTournamentState();
            SprtTournamentChatbot::cleanupSprtTournamentState();
            ctx->LogInfo("=== Test GetRunningStatusViaRegistry PASSED ===");
        };
    }

} // namespace QaplaTest
#endif
