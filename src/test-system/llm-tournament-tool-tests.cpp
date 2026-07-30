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

        // -----------------------------------------------------------------
        // Test: stop_tournament / clear_tournament_result / show_tournament_result,
        // called directly through GuiToolRegistry (no LLM).
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Llm/Tournament/Tools", "StopClearAndShowResultViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: stop/clear/show_tournament_result via GuiToolRegistry ===");

            cleanupTournamentState();
            IM_CHECK(hasEnginesAvailable());

            // stop_tournament must fail cleanly when nothing is running.
            auto stopWhenIdle = callToolAndYield(ctx, "stop_tournament", QaplaTester::Json::JsonValue::object());
            IM_CHECK(!stopWhenIdle.success);

            auto configs = QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs();
            IM_CHECK(configs.size() >= 2);

            auto engineArgs = QaplaTester::Json::JsonValue::object();
            auto engineNames = QaplaTester::Json::JsonValue::array();
            engineNames.push_back(configs[0].getName());
            engineNames.push_back(configs[1].getName());
            engineArgs["engines"] = engineNames;
            IM_CHECK(callToolAndYield(ctx, "select_engines", engineArgs).success);

            auto configureArgs = QaplaTester::Json::JsonValue::object();
            configureArgs["openings_file"] = getTestOpeningPath();
            configureArgs["games"] = 4.0;
            configureArgs["rounds"] = 1.0;
            IM_CHECK(callToolAndYield(ctx, "configure_tournament", configureArgs).success);

            IM_CHECK(callToolAndYield(ctx, "start_tournament", QaplaTester::Json::JsonValue::object()).success);
            IM_CHECK(waitForTournamentRunning(ctx, 20.0f));

            // show_tournament_result must succeed while running, even before any game finished --
            // with a real engine game in progress, scoredEngines() (and so renderWidget, see
            // GuiToolResult::renderWidget) may legitimately still be empty this soon after start,
            // so only .success is checked here. The renderWidget plumbing itself (set -> carried
            // through ToolCallEvent -> lands callable on the ChatEntry) is covered deterministically
            // by the "carries a tool's renderWidget through to its ChatEntry" Catch2 test.
            auto resultWhileRunning = callToolAndYield(ctx, "show_tournament_result", QaplaTester::Json::JsonValue::object());
            IM_CHECK(resultWhileRunning.success);

            // Let the engines settle into the first move before stopping -- see the identical
            // wait in createIncompleteTournamentState() (prevents crash/slow shutdown from a
            // rapid start/stop of the engine process).
            ctx->SleepNoSkip(0.5f, 0.1f);

            // "abrupt" is used here (rather than "graceful") so the test doesn't depend on
            // how long the real engines configured on this machine take to finish a game --
            // stop_tournament just passes the mode through to TournamentData::stopPool(),
            // whose graceful/abrupt behavior itself isn't new logic under test here.
            ctx->LogInfo("Step: stop_tournament(mode=abrupt)");
            auto stopArgs = QaplaTester::Json::JsonValue::object();
            stopArgs["mode"] = "abrupt";
            auto stopResult = callToolAndYield(ctx, "stop_tournament", stopArgs);
            IM_CHECK(stopResult.success);
            IM_CHECK(waitForTournamentStopped(ctx, 15.0f));
            IM_CHECK(!QaplaWindows::TournamentData::instance().isRunning());

            ctx->LogInfo("Step: clear_tournament_result");
            auto clearResult = callToolAndYield(ctx, "clear_tournament_result", QaplaTester::Json::JsonValue::object());
            IM_CHECK(clearResult.success);

            auto resultAfterClear = callToolAndYield(ctx, "show_tournament_result", QaplaTester::Json::JsonValue::object());
            IM_CHECK(resultAfterClear.success);
            IM_CHECK(resultAfterClear.content.find("No tournament results") != std::string::npos);
            IM_CHECK(!static_cast<bool>(resultAfterClear.renderWidget));

            // clear_tournament_result on an already-clear state must still succeed, not error.
            auto clearAgain = callToolAndYield(ctx, "clear_tournament_result", QaplaTester::Json::JsonValue::object());
            IM_CHECK(clearAgain.success);

            cleanupTournamentState();
            ctx->LogInfo("=== Test StopClearAndShowResultViaRegistry PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: configure_draw_adjudication / configure_resign_adjudication,
        // called directly through GuiToolRegistry (no LLM). No tournament
        // needs to actually run for this -- both tools just mutate
        // TournamentData's adjudication config and persist it.
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Llm/Tournament/Tools", "ConfigureAdjudicationViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: configure_draw_adjudication / configure_resign_adjudication via GuiToolRegistry ===");

            auto& tournamentData = QaplaWindows::TournamentData::instance();
            // Reset to known defaults -- these tests never touch adjudication otherwise, but
            // don't rely on that; start from the same default-constructed state either way.
            tournamentData.drawConfig() = {};
            tournamentData.resignConfig() = {};

            ctx->LogInfo("Step 1: configure_draw_adjudication");
            auto drawArgs = QaplaTester::Json::JsonValue::object();
            drawArgs["mode"] = "active";
            drawArgs["min_full_moves"] = 60.0;
            drawArgs["required_consecutive_moves"] = 10.0;
            drawArgs["centipawn_threshold"] = 15.0;
            auto drawResult = callToolAndYield(ctx, "configure_draw_adjudication", drawArgs);
            IM_CHECK(drawResult.success);
            IM_CHECK(tournamentData.drawConfig().active);
            IM_CHECK(!tournamentData.drawConfig().testOnly);
            IM_CHECK(tournamentData.drawConfig().minFullMoves == 60);
            IM_CHECK(tournamentData.drawConfig().requiredConsecutiveMoves == 10);
            IM_CHECK(tournamentData.drawConfig().centipawnThreshold == 15);

            ctx->LogInfo("Step 2: configure_resign_adjudication");
            auto resignArgs = QaplaTester::Json::JsonValue::object();
            resignArgs["mode"] = "test";
            resignArgs["required_consecutive_moves"] = 8.0;
            resignArgs["centipawn_threshold"] = 600.0;
            resignArgs["two_sided"] = true;
            auto resignResult = callToolAndYield(ctx, "configure_resign_adjudication", resignArgs);
            IM_CHECK(resignResult.success);
            IM_CHECK(tournamentData.resignConfig().active);
            IM_CHECK(tournamentData.resignConfig().testOnly);
            IM_CHECK(tournamentData.resignConfig().requiredConsecutiveMoves == 8);
            IM_CHECK(tournamentData.resignConfig().centipawnThreshold == 600);
            IM_CHECK(tournamentData.resignConfig().twoSided);

            ctx->LogInfo("Step 3: get_tournament_status reflects both");
            auto status = callToolAndYield(ctx, "get_tournament_status", QaplaTester::Json::JsonValue::object());
            IM_CHECK(status.success);
            IM_CHECK(status.content.find("Draw adjudication: active") != std::string::npos);
            IM_CHECK(status.content.find("Resign adjudication: test") != std::string::npos);

            ctx->LogInfo("Step 4: an invalid mode is rejected, not silently ignored");
            auto badArgs = QaplaTester::Json::JsonValue::object();
            badArgs["mode"] = "sometimes";
            auto badResult = callToolAndYield(ctx, "configure_draw_adjudication", badArgs);
            IM_CHECK(!badResult.success);
            // Rejecting the bad mode must not have reset the field it failed to parse.
            IM_CHECK(tournamentData.drawConfig().active);

            ctx->LogInfo("Step 5: mode=off round-trips back to disabled");
            auto offArgs = QaplaTester::Json::JsonValue::object();
            offArgs["mode"] = "off";
            IM_CHECK(callToolAndYield(ctx, "configure_draw_adjudication", offArgs).success);
            IM_CHECK(!tournamentData.drawConfig().active);
            offArgs["mode"] = "off";
            IM_CHECK(callToolAndYield(ctx, "configure_resign_adjudication", offArgs).success);
            IM_CHECK(!tournamentData.resignConfig().active);

            // Leave adjudication back at defaults so it can't affect any other test in this suite.
            tournamentData.drawConfig() = {};
            tournamentData.resignConfig() = {};
            ctx->LogInfo("=== Test ConfigureAdjudicationViaRegistry PASSED ===");
        };
    }

} // namespace QaplaTest
#endif
