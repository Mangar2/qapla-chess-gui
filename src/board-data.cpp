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
 * @author Volker B�hm
 * @copyright Copyright (c) 2025 Volker B�hm
 */


#include "board-data.h"
#include "configuration.h"

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
	timeControl_ = QaplaConfiguration::Configuration::instance()
		.getTimeControlSettings().getSelectedTimeControl();
	computeTask_->setTimeControl(timeControl_);
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
        std::nullopt, departure, destination, promote);

    if (!valid) {
        return { false, false };
    }
    else if (!move.isEmpty()) {
		MoveRecord moveRecord(gameRecord_->nextMoveIndex(), "#gui");
		moveRecord.original = move.getLAN();
		moveRecord.lan = moveRecord.original;
		moveRecord.san = gameState_->moveToSan(move);
		computeTask_->setMove(moveRecord);
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
		computeTask_->playSide();
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
		setEngineRecords(computeTask_->getEngineRecords());
		moveInfos_ = computeTask_->getMoveInfos();

		computeTask_->getGameContext().withGameRecord([&](const GameRecord& g) {
			setGameIfDifferent(g);
			timeControl_ = g.getWhiteTimeControl();
			});
		epdData_.pollData();
		auto timeControl = QaplaConfiguration::Configuration::instance()
			.getTimeControlSettings().getSelectedTimeControl();
		if (timeControl != timeControl_) {
			computeTask_->setTimeControl(timeControl);
		}
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
	if (!gameRecord_) {
		return; 
	}
	if (moveIndex <= gameRecord_->history().size()) {
		gameRecord_->setNextMoveIndex(moveIndex);
		gameState_->setFromGameRecord(*gameRecord_, moveIndex);
		computeTask_->setPosition(*gameRecord_);
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
	if (engines.size() == 0) {
		computeTask_->initEngines(EngineList{});
		return;
	}
	auto created = EngineWorkerFactory::createEngines(engines, true);
	computeTask_->initEngines(std::move(created));
}

bool BoardData::isModeActive(const std::string& mode) const {
	auto status = computeTask_->getStatus();
	switch (status) {
		case ComputeTask::Status::Stopped:
			return mode == "Manual";
		case ComputeTask::Status::Play:
			return mode == "Play";
		case ComputeTask::Status::Autoplay:
			return mode == "Auto";
		case ComputeTask::Status::Analyze:
			return mode == "Analyze";
		default:
			return false;
	}
}