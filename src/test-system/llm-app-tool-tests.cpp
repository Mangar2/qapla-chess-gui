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

#include "test-environment.h"
#include "llm-app-tool-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"

#include "tournament-chatbot/tournament-test-helpers.h"
#include "sprt-tournament-chatbot/sprt-tournament-test-helpers.h"
#include "llm/gui-tool-registry.h"

#include <atomic>
#include <thread>

namespace QaplaTest {

    namespace {
        // Same cross-thread handoff pattern as the other llm-*-tool-tests.cpp files --
        // duplicated here so this file stays independent.
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

    void registerLlmAppToolTests(ImGuiTestEngine* engine) {
        ImGuiTest* t = nullptr;

        t = IM_REGISTER_TEST(engine, "Llm/App/Tools", "OpenPgnFileFromTournamentAndSprtSourcesViaRegistry");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            prepareTestEnvironment(ctx);
            ctx->LogInfo("=== Test: open_pgn_file(source=tournament/sprt) via GuiToolRegistry ===");

            TournamentChatbot::cleanupTournamentState();
            SprtTournamentChatbot::cleanupSprtTournamentState();

            // cleanupTournamentState()/cleanupSprtTournamentState() don't reset pgn_file --
            // it's a real, persisted setting (like any other configure_* field), so an earlier
            // test or a previous real session could easily have already set it. Force both to
            // empty explicitly rather than assuming a fresh-install default (applyPgnFile has
            // no existence check, so an empty string is accepted same as any other path).
            auto clearTournamentPgn = QaplaTester::Json::JsonValue::object();
            clearTournamentPgn["pgn_file"] = "";
            IM_CHECK(callToolAndYield(ctx, "configure_tournament", clearTournamentPgn).success);
            auto clearSprtPgn = QaplaTester::Json::JsonValue::object();
            clearSprtPgn["pgn_file"] = "";
            IM_CHECK(callToolAndYield(ctx, "configure_sprt", clearSprtPgn).success);

            // Neither has a PGN output file configured now -- both sources must fail with a
            // clear, distinguishing reason rather than silently succeeding or crashing.
            auto sprtArgsNoFile = QaplaTester::Json::JsonValue::object();
            sprtArgsNoFile["source"] = "sprt";
            auto sprtResultNoFile = callToolAndYield(ctx, "open_pgn_file", sprtArgsNoFile);
            IM_CHECK(!sprtResultNoFile.success);
            IM_CHECK(sprtResultNoFile.content.find("SPRT") != std::string::npos);

            auto tournamentArgsNoFile = QaplaTester::Json::JsonValue::object();
            tournamentArgsNoFile["source"] = "tournament";
            auto tournamentResultNoFile = callToolAndYield(ctx, "open_pgn_file", tournamentArgsNoFile);
            IM_CHECK(!tournamentResultNoFile.success);
            IM_CHECK(tournamentResultNoFile.content.find("tournament") != std::string::npos);

            // Configure each with an existing file as its PGN output path -- configure_tournament/
            // configure_sprt's pgn_file itself never validates existence (it's normally a save
            // target), so this is exactly how a real "currently being written" path could look
            // while still being openable for viewing right now.
            auto testFilePath = TournamentChatbot::getTestOpeningPath();

            auto configureTournament = QaplaTester::Json::JsonValue::object();
            configureTournament["pgn_file"] = testFilePath;
            IM_CHECK(callToolAndYield(ctx, "configure_tournament", configureTournament).success);

            auto configureSprt = QaplaTester::Json::JsonValue::object();
            configureSprt["pgn_file"] = testFilePath;
            IM_CHECK(callToolAndYield(ctx, "configure_sprt", configureSprt).success);

            auto tournamentArgs = QaplaTester::Json::JsonValue::object();
            tournamentArgs["source"] = "tournament";
            auto tournamentResult = callToolAndYield(ctx, "open_pgn_file", tournamentArgs);
            IM_CHECK(tournamentResult.success);
            IM_CHECK(tournamentResult.content.find(testFilePath) != std::string::npos);

            auto sprtArgs = QaplaTester::Json::JsonValue::object();
            sprtArgs["source"] = "sprt";
            auto sprtResult = callToolAndYield(ctx, "open_pgn_file", sprtArgs);
            IM_CHECK(sprtResult.success);
            IM_CHECK(sprtResult.content.find(testFilePath) != std::string::npos);

            TournamentChatbot::cleanupTournamentState();
            SprtTournamentChatbot::cleanupSprtTournamentState();
            ctx->LogInfo("=== Test OpenPgnFileFromTournamentAndSprtSourcesViaRegistry PASSED ===");
        };
    }

} // namespace QaplaTest
#endif
