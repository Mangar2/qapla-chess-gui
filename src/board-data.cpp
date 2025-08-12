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


#include "board-data.h"

#include "qapla-engine/move.h"
#include "qapla-tester/time-control.h"
#include "qapla-tester/game-state.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/compute-task.h"
#include "qapla-tester/engine-config-manager.h"
#include "qapla-tester/engine-worker-factory.h"
#include "qapla-tester/game-manager-pool.h"

using namespace QaplaWindows;

BoardData::BoardData() : 
	gameState_(std::make_unique<GameState>()),
	gameRecord_(std::make_unique<GameRecord>()),
	computeTask_(std::make_unique<ComputeTask>())
{
	EngineConfigManager configManager;
	configManager.loadFromFile("./test/engines.ini");
	EngineWorkerFactory::setConfigManager(configManager);
	auto config = EngineWorkerFactory::getConfigManager().getConfig("Qapla 0.4.0");
	auto engines = EngineWorkerFactory::createEngines(*config, 2);
	computeTask_->initEngines(std::move(engines));
	TimeControl timeControl;
	timeControl.addTimeSegment({ 0, 100000, 100 });
	//timeControl.addTimeSegment({ 0, 1000, 10 }); 
	computeTask_->setTimeControl(timeControl);
	computeTask_->setPosition(true);
	epdData_.init();
}

BoardData::~BoardData() = default;

void BoardData::checkForGameEnd() {
	auto [cause, result] = gameState_->getGameResult();
	if (result != GameResult::Unterminated) {
		gameRecord_->setGameEnd(cause, result);
	}
}

std::pair<bool, bool> BoardData::addMove(std::optional<QaplaBasics::Square> departure,
		std::optional<QaplaBasics::Square> destination, QaplaBasics::Piece promote) 
{
    const auto [move, valid, promotion] = gameState_->resolveMove(
        std::nullopt, departure, destination, std::nullopt);

    if (!valid) {
        return { false, false };
    }
    else if (!move.isEmpty()) {
		MoveRecord moveRecord(gameRecord_->nextMoveIndex(), "#gui");
		moveRecord.original = move.getLAN();
		moveRecord.lan = moveRecord.original;
		moveRecord.san = gameState_->moveToSan(move);
		gameRecord_->addMove(moveRecord);
		gameState_->doMove(move);
		checkForGameEnd();
		computeTask_->setPosition(*gameRecord_);
		return { false, false };
    }
    else if (promotion) {
		return { true, true };
    }
	return { true, false };
}

void BoardData::setPosition(bool startPosition, const std::string& fen) {
	computeTask_->setPosition(startPosition, fen);
}

void BoardData::execute(std::string command) {
	if (command == "New" && gameRecord_ != nullptr) {
		gameState_->setFen(true, "");
		gameRecord_->setStartPosition(true, "", gameState_->isWhiteToMove());
		computeTask_->setPosition(*gameRecord_);
	}
	else if (command == "Stop") {
		computeTask_->stop();
	}
	else if (command == "Now") {
		computeTask_->moveNow();
	}
	else if (command == "Newgame") {
		computeTask_->newGame();
	}
	else if (command == "Play") {
		computeTask_->computeMove();
	}
	else if (command == "Analyze") {
		computeTask_->analyze();
	}
	else if (command == "Auto") {
		computeTask_->autoPlay();
	}
	else if (command == "Manual") {
		computeTask_->stop();
	}
	else {
		std::cerr << "Unknown command: " << command << '\n';
	}
}

void BoardData::stopPool() {
	GameManagerPool::getInstance().stopAll();
}

void BoardData::clearPool() {
	GameManagerPool::getInstance().clearAll();
}

void BoardData::setPoolConcurrency(uint32_t count, bool nice, bool start) {
	GameManagerPool::getInstance().setConcurrency(count, nice, start);
}

void BoardData::pollData() {
	try {
		auto engineRecords = computeTask_->getEngineRecords();
		setEngineRecords(engineRecords);

		computeTask_->getGameContext().withGameRecord([&](const GameRecord& g) {
			setGameIfDifferent(g);
			});
		epdData_.pollData();
	}
	catch (const std::exception& ex) {
		assert(false && "Error while polling data");
	}
}

void BoardData::setGameIfDifferent(const GameRecord& record) {
	if (gameRecord_ == nullptr || record.isUpdate(*gameRecord_)) {
		*gameRecord_ = record;
		gameState_->setFromGameRecord(*gameRecord_, gameRecord_->nextMoveIndex());
	}
}

uint32_t BoardData::nextMoveIndex() const {
	return gameRecord_->nextMoveIndex();
}

void BoardData::setNextMoveIndex(uint32_t moveIndex) {
	if (moveIndex <= gameRecord_->history().size()) {
		gameRecord_->setNextMoveIndex(moveIndex);
		gameState_->setFromGameRecord(*gameRecord_, moveIndex);
	}
}

bool BoardData::isGameOver() const {
	auto [cause, result] = gameRecord_->getGameResult();
	return result != GameResult::Unterminated && 
		gameRecord_->nextMoveIndex() >= gameRecord_->history().size();
}

void BoardData::stopEngine(size_t index) {
	if (index < engineRecords_.size()) {
		auto id = engineRecords_[index].identifier;
		computeTask_->stopEngine(id);
	}
}

void BoardData::restartEngine(size_t index) {
	if (index < engineRecords_.size()) {
		auto id = engineRecords_[index].identifier;
		computeTask_->restartEngine(id);
	}
}

void BoardData::setEngines(const std::vector<EngineConfig>& engines) {
	EngineList allEngined;
	for (auto& config: engines) {
		auto created = EngineWorkerFactory::createEngines(config, 1);
		if (!created.empty()) {
			allEngined.emplace_back(std::move(created[0]));
		}
	}
	computeTask_->initEngines(std::move(allEngined));
	TimeControl tc;
	tc.addTimeSegment({ 0, 10000, 100 });
	computeTask_->setTimeControl(tc);
}