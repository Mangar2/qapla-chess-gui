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

#include "analysis-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include "../test-common.h"
#include "../tutorial-test/tutorial-test-common.h"
#include "chatbot/chatbot-window.h"
#include "imgui-game-list.h"
#include "../test-environment.h"
#include "analysis-data.h"

#include <engine-handling/engine-worker-factory.h>
#include <opening/pgn-io.h>

#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <fstream>

namespace QaplaTest {

namespace {

/** @brief Two short decided games, written where the test can read them back. */
std::string writeTestPgn() {
    const auto path = std::filesystem::temp_directory_path() / "qapla-analysis-gui-test-in.pgn";
    std::ofstream out(path, std::ios::trunc);
    out << "[Event \"Test\"]\n[White \"White Player\"]\n[Black \"Black Player\"]\n"
           "[Result \"1-0\"]\n\n1. e4 e5 2. Bc4 Nc6 3. Qh5 Nf6 4. Qxf7# 1-0\n\n";
    out << "[Event \"Test\"]\n[White \"Other White\"]\n[Black \"Other Black\"]\n"
           "[Result \"0-1\"]\n\n1. f3 e5 2. g4 Qh4# 0-1\n\n";
    return path.string();
}

} // namespace

void registerAnalysisTests(ImGuiTestEngine* engine) {
    ImGuiTest* tst = IM_REGISTER_TEST(engine, "Analysis", "BackwardRunFinishes");
    tst->TestFunc = [](ImGuiTestContext* ctx) {
        prepareTestEnvironment(ctx);

        auto& data = QaplaWindows::AnalysisData::instance();

        std::vector<QaplaTester::EngineConfig> engines;
        for (auto config : QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs()) {
            config.setSelected(true);
            engines.push_back(config);
            break;
        }
        IM_CHECK(!engines.empty());
        ctx->LogInfo("engine %s cmd %s", engines.front().getName().c_str(),
            engines.front().getCmd().c_str());
        data.getEngineSelect().setEngineConfigurations(engines);
        IM_CHECK_EQ(data.getEngineSelect().getSelectedEngines().size(), size_t(1));

        const auto outputPath =
            (std::filesystem::temp_directory_path() / "qapla-analysis-gui-test-out.pgn").string();
        std::filesystem::remove(outputPath);
        data.config().moveTimeMs = 50;
        data.outputPgn().pgnOptions().file = outputPath;
        data.outputPgn().pgnOptions().append = false;
        data.outputPgn().pgnOptions().onlyFinishedGames = false;

        QaplaTester::PgnIO reader;
        auto games = reader.loadGames(writeTestPgn(), true);
        ctx->LogInfo("games read: %zu", games.size());
        IM_CHECK(games.size() == 2);
        data.setGames(games);
        data.setExternalConcurrency(2);

        data.analyse();
        ctx->Yield(2);
        ctx->LogInfo("after analyse: busy=%d total=%zu", data.isBusy() ? 1 : 0, data.getTotalCount());
        IM_CHECK(data.isBusy());
        IM_CHECK(data.getTotalCount() == 2);

        // A board per game being recomputed, the way the tournament and the EPD run show theirs,
        // and it names the game by its players and how it ended -- not by the engine that is
        // recomputing it, and not by a round the game never belonged to.
        auto namesAPlayer = [](const std::vector<std::string>& labels) {
            return std::ranges::any_of(labels, [](const std::string& label) {
                return (label.find("White Player") != std::string::npos
                        && label.find("Black Player") != std::string::npos
                        && label.find("1-0") != std::string::npos)
                    || (label.find("Other White") != std::string::npos
                        && label.find("Other Black") != std::string::npos
                        && label.find("0-1") != std::string::npos);
            });
        };
        const bool boardsShown = QaplaTest::Common::waitForCondition(ctx, [&data, &namesAPlayer]() {
            return data.hasRunningBoards() && namesAPlayer(data.runningBoardLabels());
        }, 20.0F);
        for (const auto& label : data.runningBoardLabels()) {
            ctx->LogInfo("board: %s", label.c_str());
        }
        IM_CHECK(boardsShown);

        const bool done = QaplaTest::Common::waitForCondition(ctx, [&data]() {
            return data.getTotalCount() > 0 && data.getFinishedCount() >= data.getTotalCount();
        }, 60.0F);
        ctx->LogInfo("finished %zu of %zu, busy=%d", data.getFinishedCount(), data.getTotalCount(),
            data.isBusy() ? 1 : 0);
        IM_CHECK(done);
    };

    tst = IM_REGISTER_TEST(engine, "Analysis", "BackwardRunThroughTheChatbot");
    tst->TestFunc = [](ImGuiTestContext* ctx) {
        prepareTestEnvironment(ctx);

        auto& data = QaplaWindows::AnalysisData::instance();
        const auto inputPath = writeTestPgn();
        const auto outputPath =
            (std::filesystem::temp_directory_path() / "qapla-analysis-chat-out.pgn").string();
        std::filesystem::remove(outputPath);
        data.config().inputFile = inputPath;
        data.config().moveTimeMs = 50;
        if (const char* pgnOverride = std::getenv("QAPLA_TEST_ANALYSIS_PGN")) {
            data.config().inputFile = pgnOverride;
        }
        if (const char* moveTime = std::getenv("QAPLA_TEST_ANALYSIS_MOVETIME")) {
            data.config().moveTimeMs = std::stoull(moveTime);
        }
        if (const char* concurrency = std::getenv("QAPLA_TEST_ANALYSIS_CONCURRENCY")) {
            data.setExternalConcurrency(static_cast<uint32_t>(std::stoul(concurrency)));
        }
        data.outputPgn().pgnOptions().file = outputPath;
        data.outputPgn().pgnOptions().append = false;
        data.outputPgn().pgnOptions().onlyFinishedGames = false;

        std::vector<QaplaTester::EngineConfig> engines;
        for (auto config : QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs()) {
            config.setSelected(true);
            engines.push_back(config);
            break;
        }
        IM_CHECK(!engines.empty());
        data.getEngineSelect().setEngineConfigurations(engines);

        QaplaWindows::ChatBot::ChatbotWindow::instance()->reset();
        ctx->Yield(2);
        TutorialTestCommon::navigateToChatbot(ctx);
        ctx->ItemClick("**/###Backward Analysis");
        ctx->Yield(3);

        ctx->ItemClick("**/###Load Games");
        ctx->LogInfo("load started");
        IM_CHECK(QaplaTest::Common::waitForCondition(ctx, []() {
            auto* list = QaplaWindows::ImGuiGameList::instance();
            return list != nullptr && !list->isLoading() && list->getFilteredGameCount() > 0;
        }, 60.0F));
        ctx->Yield(3);
        ctx->LogInfo("filtered games: %zu",
            QaplaWindows::ImGuiGameList::instance()->getFilteredGameCount());

        // Filter step, engine selection, engine loading, time, output -- all confirmed as they
        // are, the values were put in place above.
        for (int step = 0; step < 5; ++step) {
            ctx->Yield(2);
            if (ctx->ItemExists("**/###Continue")) {
                ctx->ItemClick("**/###Continue");
            } else if (ctx->ItemExists("**/###Skip Detection")) {
                ctx->ItemClick("**/###Skip Detection");
            } else {
                ctx->LogWarning("no Continue at step %d", step);
            }
        }
        ctx->Yield(3);

        IM_CHECK(ctx->ItemExists("**/###Start Analysis"));
        ctx->ItemClick("**/###Start Analysis");
        ctx->Yield(3);
        ctx->LogInfo("after start: busy=%d total=%zu games=%zu", data.isBusy() ? 1 : 0,
            data.getTotalCount(), data.getGameCount());
        IM_CHECK(data.isBusy());

        const bool ran = QaplaTest::Common::waitForCondition(ctx, [&data]() {
            return data.getTotalCount() > 0 && data.getFinishedCount() >= data.getTotalCount();
        }, 60.0F);
        ctx->LogInfo("finished %zu of %zu", data.getFinishedCount(), data.getTotalCount());
        IM_CHECK(ran);
    };
}

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
