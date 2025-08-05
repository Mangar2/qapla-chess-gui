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

#pragma once

#include "qapla-engine/types.h"
#include <memory>
#include <optional>

namespace QaplaBasics{
	class Move;
}

class GameState;
class GameRecord;

namespace QaplaWindows {

	class BoardData {
	public:
		/**
		 * @brief Constructs a new BoardData object.
		 */
		BoardData();

		/**
		 * @brief Returns a const reference to the current GameState.
		 * @return Const reference to GameState.
		 */
		const GameState& gameState() const {
			return *gameState_;
		}

		/**
		 * @brief Returns a reference to the current GameState.
		 * @return Reference to GameState.
		 */
		GameState& gameState() {
			return *gameState_;
		}

		/**
		 * @brief Returns a const reference to the current GameRecord.
		 * @return Const reference to GameRecord.
		 */
		const GameRecord& gameRecord() const {
			return *gameRecord_;
		}

		/**
		 * @brief Returns a reference to the current GameRecord.
		 * @return Reference to GameRecord.
		 */
		GameRecord& gameRecord() {
			return *gameRecord_;
		}

		/**
		 * @brief Adds a move to the game.
		 * @param departure Optional: departure square of the move.
		 * @param destination Optional: destination square of the move.
		 * @param promote Piece to promote to, if applicable.
		 * @return first true, if different moves matches, second true, if promotion piece is required
		 */
		std::pair<bool, bool> addMove(std::optional<QaplaBasics::Square> departure, 
			std::optional<QaplaBasics::Square> destination, 
			QaplaBasics::Piece promote);

		/**
		 * @brief Returns the current move index.
		 * @return Current move index (uint32_t).
		 */
		uint32_t nextMoveIndex() const;

		/**
		 * @brief Sets the current move index.
		 * @param moveIndex The move index to set.
		 */
		void setNextMoveIndex(uint32_t moveIndex);

	private:
		std::unique_ptr<GameState> gameState_;
		std::unique_ptr<GameRecord> gameRecord_;
	};

}