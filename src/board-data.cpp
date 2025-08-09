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
#include "qapla-tester/game-state.h"
#include "qapla-tester/game-record.h"

using namespace QaplaWindows;

BoardData::BoardData() : 
	gameState_(std::make_unique<GameState>()), gameRecord_(std::make_unique<GameRecord>())
{
}

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
		setPositionCallback_(*gameRecord_);
		return { false, false };
    }
    else if (promotion) {
		return { true, true };
    }
	return { true, false };
}

void BoardData::execute(std::string command) {
	if (command == "New" && gameRecord_ != nullptr) {
		gameState_->setFen(true, "");
		gameRecord_->setStartPosition(true, "", gameState_->isWhiteToMove());
		setPositionCallback_(*gameRecord_);
	}
	else if (executeCallback_) {
		executeCallback_(command);
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