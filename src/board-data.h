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

#include "epd-data.h"

#include "qapla-engine/types.h"
#include "qapla-tester/engine-record.h"

#include <memory>
#include <optional>
#include <functional>

namespace QaplaBasics{
	class Move;
}

class GameState;
class GameRecord;
class ComputeTask;

namespace QaplaWindows {

	class BoardData {
	public:
		/**
		 * @brief Constructs a new BoardData object.
		 */
		BoardData();

		~BoardData();

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
		 * @brief Sets the position of the game.
		 * @param startPosition If true, sets the position to the starting position.
		 * @param fen Optional: FEN string to set the position. Must be provided if startPosition is false.
		 */
		void setPosition(bool startPosition, const std::string& fen = "");

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

		const EngineRecords& engineRecords() const {
			return engineRecords_;
		}

		const EpdData& epdData() const {
			return epdData_;
		}
		EpdData& epdData() {
			return epdData_;
		}

		void setEngineRecords(const EngineRecords& records) {
			engineRecords_ = records;
		}

		/**
		 * @brief Sets the game state from a GameRecord, if the new state extends the existing state.
		 * @param record The GameRecord to set the game state from.
		 */
		void setGameIfDifferent(const GameRecord& record);

		/**
		 * @brief Checks if the game is over. 
		 * 1. The game must have a result (not GameResult::Unterminated).
		 * 2. The move index must be at the end of the game record.
		 * @return true if the game is over, false otherwise.
		 */
		bool isGameOver() const;

		void execute(std::string command); 

		/**
		 * @brief Polls data from several data provider (computeTask, edpData, ...) to provide them for imgui windows.
		 * This method must be called in the main gui loop to keep the data up-to-date.
		 */
		void pollData();

		/**
		 * @brief Stops all ongoing tasks in the pool.
		 */ 
		void stopPool();

		/**
		 * @brief Stops all ongoing tasks and clears all task providers in the pool.
		 */
		void clearPool();

		/**
		 * @brief Sets the pool concurrency level.
		 * @param count The number of concurrent tasks to allow.
		 * @param nice If true, reduces the number of active managers gradually.
		 * @param start If true, starts new tasks immediately.
		 */
		void setPoolConcurrency(uint32_t count, bool nice = true, bool start = false);


	private:

		EpdData epdData_;

		void checkForGameEnd();
		std::unique_ptr<GameState> gameState_;
		std::unique_ptr<GameRecord> gameRecord_;
		std::unique_ptr<ComputeTask> computeTask_;
		EngineRecords engineRecords_;
	};

}