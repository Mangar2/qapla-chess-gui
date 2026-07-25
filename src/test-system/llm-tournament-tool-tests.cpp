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

#include "llm-tournament-tool-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

#include "tournament-chatbot/tournament-test-helpers.h"
#include "tournament-data.h"
#include "llm/gui-tool-registry.h"
#include <engine-handling/engine-worker-factory.h>

#include <atomic>
#include <thread>

namespace QaplaTest {

    using namespace TournamentChatbot;

    namespace {
        // Calls a GUI tool on a worker thread while yielding the test
        // engine, so the app's normal poll loop (which drains
        // GuiToolRegistry's queue every frame) keeps running -- exactly the
        // cross-thread handoff a real chat turn goes through, just without
        // an LLM in the loop.
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

    void registerLlmTournamentToolTests(ImGuiTestEngine* engine) {
        ImGuiTest* t = nullptr;

        // -----------------------------------------------------------------
        // Test: select_engines + configure_tournament + start_tournament,
        // called directly through GuiToolRegistry (no LLM) ⇒
        // TournamentData::instance().isRunning() == true, i.e. the
        // TournamentWindow shows the running tournament exactly as if
        // started manually (see docs/llm-chatbot-plan.md Step 4).
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Llm/Tournament/Tools", "StartTournamentDirectlyViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: start_tournament via GuiToolRegistry ===");

            cleanupTournamentState();
            IM_CHECK(hasEnginesAvailable());

            auto configs = QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs();
            IM_CHECK(configs.size() >= 2);

            ctx->LogInfo("Step 1: select_engines");
            auto engineArgs = QaplaTester::Json::JsonValue::object();
            auto engineNames = QaplaTester::Json::JsonValue::array();
            engineNames.push_back(configs[0].getName());
            engineNames.push_back(configs[1].getName());
            engineArgs["engines"] = engineNames;
            auto selectResult = callToolAndYield(ctx, "select_engines", engineArgs);
            IM_CHECK(selectResult.success);

            ctx->LogInfo("Step 2: configure_tournament");
            auto configureArgs = QaplaTester::Json::JsonValue::object();
            configureArgs["openings_file"] = getTestOpeningPath();
            configureArgs["games"] = 1.0;
            configureArgs["rounds"] = 1.0;
            auto configureResult = callToolAndYield(ctx, "configure_tournament", configureArgs);
            IM_CHECK(configureResult.success);

            ctx->LogInfo("Step 3: start_tournament");
            auto startResult = callToolAndYield(ctx, "start_tournament", QaplaTester::Json::JsonValue::object());
            IM_CHECK(startResult.success);

            IM_CHECK(waitForTournamentRunning(ctx, 20.0f));
            IM_CHECK(QaplaWindows::TournamentData::instance().isRunning());
            ctx->LogInfo("Tournament is running, exactly as manual start would show it.");

            cleanupTournamentState();
            ctx->LogInfo("=== Test StartTournamentDirectlyViaRegistry PASSED ===");
        };
    }

} // namespace QaplaTest
#endif
