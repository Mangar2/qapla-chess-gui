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

#include "epd-test.h"
#include "game-manager.h"
#include "game-manager-pool.h"
#include "game-state.h"
#include "string-helper.h"

/*
inline std::ostream& operator<<(std::ostream& os, const EpdTestCase& test) {

    os << std::setw(20) << std::left << test.id
        << "|" << std::setw(8) << std::right
        << (test.correct ? QaplaHelpers::formatMs(test.correctAtTimeInMs, 2) : test.tested ? "-" : "?")
        << ", D:" << std::setw(3) << std::right
        << (test.correct ? std::to_string(test.correctAtDepth) : test.tested ? "-" : "?")
        << ", M: " << std::setw(5) << std::left << test.playedMove
        << " | BM: ";

    for (const auto& bm : test.bestMoves) {
        os << bm << " ";
    }
    return os;
}
*/

void EpdTest::initialize(const EpdTestResult& tests)
{
    result_ = tests;
	updateCnt_++;
    testIndex_ = 0;
    oldestIndexInUse_ = 0;
}

void EpdTest::schedule(const std::shared_ptr<EpdTest>& self, const EngineConfig& engine) {
    GameManagerPool::getInstance().addTaskProvider(self, engine); 
    GameManagerPool::getInstance().startManagers();
}

void EpdTest::continueAnalysis() {
    testIndex_ = oldestIndexInUse_.load();
}

std::optional<GameTask> EpdTest::nextTask() {
    std::lock_guard<std::mutex> lock(testResultMutex_);


    GameTask task;
    task.taskType = GameTask::Type::ComputeMove;
    
    GameState gameState;
    while (testIndex_ < result_.result.size() && result_.result[testIndex_].tested) {
        ++testIndex_;
    }
    if (testIndex_ >= result_.result.size()) {
        return std::nullopt;
    }
	auto& test = result_.result[testIndex_];
    task.gameRecord.setPositionName(test.id);
    gameState.setFen(false, test.fen);
    auto correctedFen = gameState.getFen();
    task.gameRecord.setStartPosition(false, correctedFen, gameState.isWhiteToMove(), 
        gameState.getStartHalfmoves(), "", "");
    task.gameRecord.setTimeControl(result_.tc_, result_.tc_);
    task.taskId = std::to_string(testIndex_);
    testIndex_++;
    std::cout << "EPD Test: Starting task " << task.taskId << " with position " << test.id << " (" << correctedFen << ")" << std::endl;

    return task;
}

bool EpdTest::setPV(const std::string& taskId,
    const std::vector<std::string>& pv,
    uint64_t timeInMs,
    std::optional<uint32_t> depth,
    std::optional<uint64_t> nodes,
    [[maybe_unused]] std::optional<uint32_t> multipv) 
{
    if (pv.empty()) {
        return false;
    }

    const auto index = (std::stoi(taskId));
    std::lock_guard<std::mutex> lock(testResultMutex_);

    if (index < 0 || index >= static_cast<int>(result_.result.size())) return false;

    auto& test = result_.result[static_cast<uint32_t>(index)];
    // We may have an info line from the engine after the move was already played
    if (test.tested) return false; 
    assert(test.playedMove.empty());

    const std::string& firstMove = pv.front();
    bool found = std::any_of(test.bestMoves.begin(), test.bestMoves.end(),
        [&](const std::string& bm) {
            return isSameMove(test.fen, firstMove, bm);
        });

    if (found) {
        if (test.correctAtDepth == -1 && depth.has_value()) {
            test.correctAtDepth = static_cast<int>(depth.value());
        }
        if (test.correctAtTimeInMs == 0) {
            test.correctAtTimeInMs = timeInMs;
        }
        if (test.correctAtNodeCount == 0 && nodes.has_value()) {
            test.correctAtNodeCount = nodes.value();
        }
    }
    else {
        test.correctAtDepth = -1;
        test.correctAtTimeInMs = 0;
        test.correctAtNodeCount = 0;
    }

    bool earlyStop =
		test.seenPlies > 0 && test.correctAtDepth >= 0 && depth.has_value() &&
        timeInMs >= test.minTimeInS * 1000 &&
        static_cast<int>(*depth) - test.correctAtDepth >= test.seenPlies;

    return earlyStop;
}

void EpdTest::setGameRecord(const std::string& taskId, const GameRecord& record) {
    const std::string& fen = record.getStartFen();

    const auto& moves = record.history();
    if (moves.empty()) return;

    const auto& move = moves.back();
    const std::string& played = move.san.empty() ? move.lan : move.san;

    int taskIdIndex = -1;
    try {
        taskIdIndex = std::stoi(taskId);
    }
    catch (...) {
		throw AppError::make("Invalid taskId: " + taskId + ". Must be a valid integer.");
    }

    auto recentOldestIndex = oldestIndexInUse_.load();
    if (taskIdIndex < 0 || taskIdIndex >= static_cast<int>(result_.result.size())) return;
    {
        std::lock_guard<std::mutex> lock(testResultMutex_);
        const size_t index = static_cast<size_t>(taskIdIndex);
        auto& test = result_.result[index];
        std::cout << "EPD Test: " << index << " played " << played << std::endl;
        assert(test.playedMove.empty());

        test.tested = true;
        test.playedMove = played;
        test.correct = std::any_of(
            test.bestMoves.begin(), test.bestMoves.end(),
            [&](const std::string& bm) {
                return isSameMove(fen, played, bm);
            }
        );
        test.timeMs = move.timeMs;
        test.searchDepth = static_cast<int>(move.depth);
        test.nodeCount = move.nodes;

        // Note: SetPV might have set the correct depth, time, nodes already
        if (test.correct && test.correctAtDepth == -1) {
            test.correctAtDepth = static_cast<int>(move.depth);
            test.correctAtTimeInMs = move.timeMs;
            test.correctAtNodeCount = move.nodes;
        }

        // If SetPV saw the correct move but it was finally not played, we need to remove the former result
        if (!test.correct) {
            test.correctAtDepth = -1;
            test.correctAtTimeInMs = 0;
            test.correctAtNodeCount = 0;
        }

        // Close gap, if oldest
        if (index == oldestIndexInUse_) {
            while (oldestIndexInUse_ < result_.result.size() && !result_.result[oldestIndexInUse_].playedMove.empty()) {
                ++oldestIndexInUse_;
            }
        }
        updateCnt_++;
    }
    if (recentOldestIndex != oldestIndexInUse_ && testResultCallback_) {
        testResultCallback_(this, recentOldestIndex, oldestIndexInUse_);
    }
}

bool EpdTest::isSameMove(const std::string& fen, const std::string& move1str, const std::string& move2str) const {
    GameState gameState;
    gameState.setFen(false, fen);
    auto move1 = gameState.stringToMove(move1str, false);
    auto move2 = gameState.stringToMove(move2str, false);
    return move1 == move2;
}