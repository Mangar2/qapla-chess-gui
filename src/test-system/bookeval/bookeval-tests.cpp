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

#include "bookeval-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include "../test-common.h"
#include "../tutorial-test/tutorial-test-common.h"
#include "../test-environment.h"
#include "chatbot/chatbot-window.h"
#include "book-eval-data.h"

#include <book-eval/book-tree.h>
#include <engine-handling/engine-worker-factory.h>
#include <qapla-engine/book9/book.h>

#include <filesystem>
#include <string>
#include <vector>

namespace QaplaTest {

namespace {

/**
 * Three lines: e2e4 e7e5 g1f3 b8c6 f1c4 g8f6 e1g1, e2e4 c7c5 and d2d4 d7d5 c2c4 - eleven moves,
 * played from nine positions.
 */
constexpr size_t POSITIONS = 9;

std::string writeTestBook(const std::string& name) {
    const auto path = (std::filesystem::temp_directory_path() / name).string();
    QaplaBook9::Book<> book;
    const std::vector<std::vector<std::string>> lines = {
        { "e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6", "e1g1" },
        { "e2e4", "c7c5" },
        { "d2d4", "d7d5", "c2c4" } };
    for (const auto& line : lines) {
        std::vector<QaplaBook9::BookMove> moves;
        for (const auto& lan : line) {
            auto square = [&](size_t at) {
                return QaplaBasics::Square((lan[at] - 'a') + (lan[at + 1] - '1') * 8);
            };
            moves.push_back(QaplaBook9::BookMove{ .from = square(0), .to = square(2) });
        }
        book.addLine(moves);
    }
    book.writeToFile(path);
    return path;
}

std::string freshOutFile(const std::string& name) {
    const auto path = (std::filesystem::temp_directory_path() / name).string();
    std::filesystem::remove(path);
    return path;
}

void selectFirstEngine(QaplaWindows::BookEvalData& data) {
    std::vector<QaplaTester::EngineConfig> engines;
    for (auto config : QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs()) {
        config.setSelected(true);
        engines.push_back(config);
        break;
    }
    IM_CHECK(!engines.empty());
    data.getEngineSelect().setEngineConfigurations(engines);
}

/** @brief Counts the positions of the evaluated book that have their score. */
size_t scoredPositions(const std::string& path) {
    const auto tree = QaplaTester::BookTree::read(path);
    size_t scored = 0;
    for (uint32_t position = 0; position < tree.positionCount(); ++position) {
        if (tree.isPlayedFrom(position) && tree.score(position) != QaplaTester::BookScore::NONE) {
            ++scored;
        }
    }
    return scored;
}

} // namespace

void registerBookEvalTests(ImGuiTestEngine* engine) {
    ImGuiTest* tst = IM_REGISTER_TEST(engine, "BookEval", "WholeBookThroughTheChatbot");
    tst->TestFunc = [](ImGuiTestContext* ctx) {
        prepareTestEnvironment(ctx);
        auto& data = QaplaWindows::BookEvalData::instance();
        data.config().bookFile = writeTestBook("qapla-bookeval-gui-in.bok");
        data.config().outFile = freshOutFile("qapla-bookeval-gui-out.bok");
        data.config().limit = QaplaWindows::BookEvalData::LIMIT_DEPTH;
        data.config().limitValue = 4;
        data.setExternalConcurrency(2);
        selectFirstEngine(data);

        QaplaWindows::ChatBot::ChatbotWindow::instance()->reset();
        ctx->Yield(2);
        TutorialTestCommon::navigateToChatbot(ctx);
        ctx->ItemClick("**/###Book Evaluation");
        ctx->Yield(3);

        // Books, engine selection, engine loading, limit -- confirmed as they were set above.
        for (int step = 0; step < 4; ++step) {
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
        IM_CHECK(ctx->ItemExists("**/###Start Evaluation"));
        ctx->ItemClick("**/###Start Evaluation");
        ctx->Yield(2);

        const bool done = QaplaTest::Common::waitForCondition(ctx, [&data]() {
            return !data.isBusy() && data.isComplete();
        }, 60.0F);
        ctx->LogInfo("evaluated %zu of %zu", data.getEvaluatedCount(), data.getPositionCount());
        IM_CHECK(done);
        IM_CHECK_EQ(data.getPositionCount(), POSITIONS);
        IM_CHECK_EQ(scoredPositions(data.config().outFile), POSITIONS);
        IM_CHECK(ctx->ItemExists("**/###Close"));
    };

    tst = IM_REGISTER_TEST(engine, "BookEval", "StoppedRunIsContinued");
    tst->TestFunc = [](ImGuiTestContext* ctx) {
        prepareTestEnvironment(ctx);
        auto& data = QaplaWindows::BookEvalData::instance();
        data.config().bookFile = writeTestBook("qapla-bookeval-gui-stop-in.bok");
        data.config().outFile = freshOutFile("qapla-bookeval-gui-stop-out.bok");
        // Slow enough to be stopped halfway: one engine, half a second per position.
        data.config().limit = QaplaWindows::BookEvalData::LIMIT_MOVETIME;
        data.config().limitValue = 500;
        data.setExternalConcurrency(1);
        selectFirstEngine(data);

        data.evaluate();
        ctx->Yield(2);
        IM_CHECK(data.isBusy());
        IM_CHECK(QaplaTest::Common::waitForCondition(ctx, [&data]() {
            return data.getEvaluatedCount() >= 2;
        }, 30.0F));
        data.stop(false);
        IM_CHECK(QaplaTest::Common::waitForCondition(ctx, [&data]() { return !data.isBusy(); }, 30.0F));
        const auto afterStop = scoredPositions(data.config().outFile);
        ctx->LogInfo("stopped with %zu of %zu", afterStop, POSITIONS);
        IM_CHECK(afterStop >= 2);
        IM_CHECK(afterStop < POSITIONS);
        IM_CHECK(!data.isComplete());

        // The same books, engine and limit again: the run goes on with the positions left open.
        data.evaluate();
        ctx->Yield(2);
        IM_CHECK(QaplaTest::Common::waitForCondition(ctx, [&data]() {
            return !data.isBusy() && data.isComplete();
        }, 60.0F));
        IM_CHECK_EQ(scoredPositions(data.config().outFile), POSITIONS);
    };

    tst = IM_REGISTER_TEST(engine, "BookEval", "ChatbotShowsARunningEvaluation");
    tst->TestFunc = [](ImGuiTestContext* ctx) {
        prepareTestEnvironment(ctx);
        auto& data = QaplaWindows::BookEvalData::instance();
        data.config().bookFile = writeTestBook("qapla-bookeval-gui-running-in.bok");
        data.config().outFile = freshOutFile("qapla-bookeval-gui-running-out.bok");
        data.config().limit = QaplaWindows::BookEvalData::LIMIT_MOVETIME;
        data.config().limitValue = 5000;
        data.setExternalConcurrency(1);
        selectFirstEngine(data);
        data.evaluate();
        ctx->Yield(2);
        IM_CHECK(data.isBusy());

        QaplaWindows::ChatBot::ChatbotWindow::instance()->reset();
        ctx->Yield(2);
        TutorialTestCommon::navigateToChatbot(ctx);
        ctx->ItemClick("**/###Book Evaluation");
        ctx->Yield(3);

        // A run that is going leaves nothing to set up: the thread opens on it.
        IM_CHECK(ctx->ItemExists("**/###Stop"));
        IM_CHECK(!ctx->ItemExists("**/###Start Evaluation"));
        ctx->ItemClick("**/###Stop");
        IM_CHECK(QaplaTest::Common::waitForCondition(ctx, [&data]() { return !data.isBusy(); }, 30.0F));
        IM_CHECK(std::filesystem::exists(data.config().outFile));
    };
}

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
