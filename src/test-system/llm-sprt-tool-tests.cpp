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

#include "llm-sprt-tool-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

#include "sprt-tournament-chatbot/sprt-tournament-test-helpers.h"
#include "sprt-tournament-data.h"
#include "llm/gui-tool-registry.h"
#include <engine-handling/engine-worker-factory.h>
#include <sprt/sprt-manager.h>

#include <atomic>
#include <thread>

namespace QaplaTest {

    using namespace SprtTournamentChatbot;

    namespace {
        // Same cross-thread handoff pattern as llm-tournament-tool-tests.cpp's
        // callToolAndYield() -- duplicated here rather than shared so this file
        // stays independent (see that file's own copy for the full rationale).
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

    void registerLlmSprtToolTests(ImGuiTestEngine* engine) {
        ImGuiTest* t = nullptr;

        // -----------------------------------------------------------------
        // Test: configure_sprt (engines, settings and its draw_*/resign_* adjudication
        // fields) / get_status (type=sprt), called directly through GuiToolRegistry (no LLM).
        // No SPRT test needs to actually run for this.
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Llm/Sprt/Tools", "ConfigureSprtViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: configure_sprt / adjudication / get_status (type=sprt) via GuiToolRegistry ===");

            cleanupSprtTournamentState();
            IM_CHECK(hasEnginesAvailable());

            auto configs = QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs();
            IM_CHECK(configs.size() >= 2);

            ctx->LogInfo("Step 1: configure_sprt (champion/challenger)");
            auto selectArgs = QaplaTester::Json::JsonValue::object();
            selectArgs["champion"] = configs[0].getName();
            selectArgs["challenger"] = configs[1].getName();
            auto selectResult = callToolAndYield(ctx, "configure_sprt", selectArgs);
            IM_CHECK(selectResult.success);

            auto& sprtData = QaplaWindows::SprtTournamentData::instance();
            auto selected = sprtData.getEngineSelect().getSelectedEngines();
            IM_CHECK(selected.size() == 2);
            for (const auto& e : selected) {
                if (e.getName() == configs[0].getName()) {
                    IM_CHECK(!e.isGauntlet()); // champion = non-gauntlet
                } else if (e.getName() == configs[1].getName()) {
                    IM_CHECK(e.isGauntlet()); // challenger = gauntlet
                }
            }

            // Selecting the same engine for both roles must be rejected.
            auto sameArgs = QaplaTester::Json::JsonValue::object();
            sameArgs["champion"] = configs[0].getName();
            sameArgs["challenger"] = configs[0].getName();
            IM_CHECK(!callToolAndYield(ctx, "configure_sprt", sameArgs).success);

            // Naming only one role is a complete request -- the other keeps its engine, exactly
            // as an unpassed time_control does. This regressed when engine selection moved into
            // configure_sprt: a {champion, time_control} patch was rejected whole, and the model
            // then went asking the user for a challenger that was already configured.
            ctx->LogInfo("Step 1b: configure_sprt with only one role named");
            auto oneRoleArgs = QaplaTester::Json::JsonValue::object();
            oneRoleArgs["champion"] = configs[0].getName();
            oneRoleArgs["time_control"] = "60+0";
            IM_CHECK(callToolAndYield(ctx, "configure_sprt", oneRoleArgs).success);
            auto afterOneRole = sprtData.getEngineSelect().getSelectedEngines();
            IM_CHECK(afterOneRole.size() == 2);
            bool challengerKept = false;
            for (const auto& e : afterOneRole) {
                if (e.isGauntlet() && e.getName() == configs[1].getName()) {
                    challengerKept = true;
                }
            }
            IM_CHECK(challengerKept);

            ctx->LogInfo("Step 2: configure_sprt (settings)");
            auto configureArgs = QaplaTester::Json::JsonValue::object();
            configureArgs["openings_file"] = getTestOpeningPath();
            configureArgs["elo0"] = 0.0;
            configureArgs["elo1"] = 5.0;
            configureArgs["alpha"] = 0.05;
            configureArgs["beta"] = 0.05;
            configureArgs["max_games"] = 4.0;
            configureArgs["model"] = "normalized";
            auto configureResult = callToolAndYield(ctx, "configure_sprt", configureArgs);
            IM_CHECK(configureResult.success);
            IM_CHECK(sprtData.sprtConfig().eloH0 == 0.0F);
            IM_CHECK(sprtData.sprtConfig().eloH1 == 5.0F);
            IM_CHECK(sprtData.sprtConfig().maxGames == 4);

            // An invalid model must be rejected, not silently ignored.
            auto badModelArgs = QaplaTester::Json::JsonValue::object();
            badModelArgs["model"] = "quantum";
            IM_CHECK(!callToolAndYield(ctx, "configure_sprt", badModelArgs).success);

            // Captured before touching SPRT adjudication, to verify below that configuring it
            // never affects classic tournament mode's own (independent) adjudication config --
            // whatever that config's state happens to already be on this machine.
            auto tournamentTypeArgs = QaplaTester::Json::JsonValue::object();
            tournamentTypeArgs["type"] = "tournament";
            auto tournamentStatusBefore = callToolAndYield(ctx, "get_status", tournamentTypeArgs);
            IM_CHECK(tournamentStatusBefore.success);

            ctx->LogInfo("Step 3: configure_sprt's draw_*/resign_* fields");
            auto drawArgs = QaplaTester::Json::JsonValue::object();
            drawArgs["draw_mode"] = "active";
            drawArgs["draw_centipawn_threshold"] = 15.0;
            IM_CHECK(callToolAndYield(ctx, "configure_sprt", drawArgs).success);
            IM_CHECK(sprtData.tournamentAdjudication().drawConfig().active);

            auto resignArgs = QaplaTester::Json::JsonValue::object();
            resignArgs["resign_mode"] = "test";
            resignArgs["resign_two_sided"] = true;
            auto resignResult = callToolAndYield(ctx, "configure_sprt", resignArgs);
            IM_CHECK(resignResult.success);
            IM_CHECK(sprtData.tournamentAdjudication().resignConfig().active);
            IM_CHECK(sprtData.tournamentAdjudication().resignConfig().testOnly);
            IM_CHECK(sprtData.tournamentAdjudication().resignConfig().twoSided);

            ctx->LogInfo("Step 4: configure_sprt's own response already reflects all of the above");
            auto status = resignResult;
            IM_CHECK(status.content.find(configs[0].getName()) != std::string::npos);
            IM_CHECK(status.content.find(configs[1].getName()) != std::string::npos);
            IM_CHECK(status.content.find("Draw adjudication: active") != std::string::npos);
            IM_CHECK(status.content.find("Resign adjudication: test") != std::string::npos);

            // This tool group must never touch classic tournament mode's own configuration --
            // the whole point of the split is that the two stay independent (see the system
            // prompt's tournament-vs-SPRT disambiguation guidance in chatbot-llm-chat.cpp).
            // Compared against the snapshot taken before touching SPRT adjudication above --
            // NOT asserted to be "off", since that depends on whatever's already persisted on
            // this machine, not on anything this test controls.
            auto tournamentStatusAfter = callToolAndYield(ctx, "get_status", tournamentTypeArgs);
            IM_CHECK(tournamentStatusAfter.success);
            IM_CHECK(tournamentStatusAfter.content == tournamentStatusBefore.content);

            // Reset adjudication back to defaults so it can't affect any other test in this suite.
            sprtData.tournamentAdjudication().drawConfig() = {};
            sprtData.tournamentAdjudication().resignConfig() = {};
            sprtData.tournamentAdjudication().updateConfiguration();

            cleanupSprtTournamentState();
            ctx->LogInfo("=== Test ConfigureSprtViaRegistry PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: start/stop/clear_result/get_status (type=sprt), called directly through
        // GuiToolRegistry (no LLM).
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Llm/Sprt/Tools", "StartStopClearShowSprtResultViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: start/stop/clear_result/get_status (type=sprt) via GuiToolRegistry ===");

            cleanupSprtTournamentState();
            IM_CHECK(hasEnginesAvailable());

            // stop(type=sprt) must fail cleanly when nothing is running.
            auto sprtTypeArgs = QaplaTester::Json::JsonValue::object();
            sprtTypeArgs["type"] = "sprt";
            IM_CHECK(!callToolAndYield(ctx, "stop", sprtTypeArgs).success);

            auto configs = QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs();
            IM_CHECK(configs.size() >= 2);

            auto selectArgs = QaplaTester::Json::JsonValue::object();
            selectArgs["champion"] = configs[0].getName();
            selectArgs["challenger"] = configs[1].getName();
            IM_CHECK(callToolAndYield(ctx, "configure_sprt", selectArgs).success);

            auto configureArgs = QaplaTester::Json::JsonValue::object();
            configureArgs["openings_file"] = getTestOpeningPath();
            configureArgs["max_games"] = 4.0;
            IM_CHECK(callToolAndYield(ctx, "configure_sprt", configureArgs).success);

            IM_CHECK(callToolAndYield(ctx, "start", sprtTypeArgs).success);
            IM_CHECK(waitForSprtTournamentRunning(ctx, 20.0f));

            // get_status must succeed while running, even before any game finished.
            auto resultWhileRunning = callToolAndYield(ctx, "get_status", sprtTypeArgs);
            IM_CHECK(resultWhileRunning.success);

            // Let the engines settle into the first move before stopping -- see the identical
            // wait in llm-tournament-tool-tests.cpp / createIncompleteSprtTournamentState()
            // (prevents crash/slow shutdown from a rapid start/stop of the engine process).
            ctx->SleepNoSkip(0.5f, 0.1f);

            // "abrupt" is used here (rather than "graceful") for the same determinism reason as
            // the tournament tool tests: stop(type=sprt) just passes the mode through to
            // SprtTournamentData::stopPool(), whose graceful/abrupt behavior isn't new logic
            // under test here, and graceful would depend on how long this machine's real engines
            // take to finish a game.
            ctx->LogInfo("Step: stop(type=sprt, mode=abrupt)");
            auto stopArgs = QaplaTester::Json::JsonValue::object();
            stopArgs["type"] = "sprt";
            stopArgs["mode"] = "abrupt";
            auto stopResult = callToolAndYield(ctx, "stop", stopArgs);
            IM_CHECK(stopResult.success);
            IM_CHECK(waitForSprtTournamentStopped(ctx, 15.0f));
            IM_CHECK(!QaplaWindows::SprtTournamentData::instance().isRunning());

            ctx->LogInfo("Step: clear_result(type=sprt)");
            auto clearResult = callToolAndYield(ctx, "clear_result", sprtTypeArgs);
            IM_CHECK(clearResult.success);

            auto resultAfterClear = callToolAndYield(ctx, "get_status", sprtTypeArgs);
            IM_CHECK(resultAfterClear.success);
            IM_CHECK(resultAfterClear.content.find("No SPRT results") != std::string::npos);
            IM_CHECK(!static_cast<bool>(resultAfterClear.renderWidget));

            // clear_result on an already-clear state must still succeed, not error.
            IM_CHECK(callToolAndYield(ctx, "clear_result", sprtTypeArgs).success);

            cleanupSprtTournamentState();
            ctx->LogInfo("=== Test StartStopClearShowSprtResultViaRegistry PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test: stopping an SPRT test must not reset its configured concurrency -- regression
        // test for ImGuiConcurrency conflating "configured" (survives stop) with "active" (0
        // while stopped) concurrency; see ImGuiConcurrency's class doc comment.
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "Llm/Sprt/Tools", "ConcurrencySurvivesStopViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: concurrency survives stop(type=sprt) ===");

            cleanupSprtTournamentState();
            IM_CHECK(hasEnginesAvailable());

            auto configs = QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs();
            IM_CHECK(configs.size() >= 2);

            auto selectArgs = QaplaTester::Json::JsonValue::object();
            selectArgs["champion"] = configs[0].getName();
            selectArgs["challenger"] = configs[1].getName();
            IM_CHECK(callToolAndYield(ctx, "configure_sprt", selectArgs).success);

            constexpr uint32_t configuredConcurrency = 5;
            auto configureArgs = QaplaTester::Json::JsonValue::object();
            configureArgs["openings_file"] = getTestOpeningPath();
            configureArgs["max_games"] = 4.0;
            configureArgs["concurrency"] = static_cast<double>(configuredConcurrency);
            IM_CHECK(callToolAndYield(ctx, "configure_sprt", configureArgs).success);
            IM_CHECK_EQ(QaplaWindows::SprtTournamentData::instance().getExternalConcurrency(), configuredConcurrency);

            auto startArgs = QaplaTester::Json::JsonValue::object();
            startArgs["type"] = "sprt";
            IM_CHECK(callToolAndYield(ctx, "start", startArgs).success);
            IM_CHECK(waitForSprtTournamentRunning(ctx, 20.0f));
            IM_CHECK_EQ(QaplaWindows::SprtTournamentData::instance().getExternalConcurrency(), configuredConcurrency);

            ctx->SleepNoSkip(0.5f, 0.1f);
            auto stopArgs = QaplaTester::Json::JsonValue::object();
            stopArgs["type"] = "sprt";
            stopArgs["mode"] = "abrupt";
            IM_CHECK(callToolAndYield(ctx, "stop", stopArgs).success);
            IM_CHECK(waitForSprtTournamentStopped(ctx, 15.0f));

            IM_CHECK_EQ(QaplaWindows::SprtTournamentData::instance().getExternalConcurrency(), configuredConcurrency);

            cleanupSprtTournamentState();
            ctx->LogInfo("=== Test ConcurrencySurvivesStopViaRegistry PASSED ===");
        };
    }

} // namespace QaplaTest
#endif
