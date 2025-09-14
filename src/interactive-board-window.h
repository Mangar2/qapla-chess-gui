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
#include "embedded-window.h"

#include "qapla-engine/types.h"
#include "qapla-tester/time-control.h"
#include "qapla-tester/ini-file.h"
#include "qapla-tester/move-record.h"

#include <memory>
#include <optional>
#include <functional>
#include <string>

namespace QaplaBasics
{
	class Move;
}

class EngineConfig;
class GameState;
class MoveRecord;
class GameRecord;
class ComputeTask;

namespace QaplaWindows
{

	class ImGuiBoard;
	class ImGuiEngineList;
	class ImGuiClock;
	class ImGuiMoveList;
	class ImGuiBarChart;

	class InteractiveBoardWindow
	{
	public:
		/**
		 * @brief Constructs a new BoardData object.
		 */
		InteractiveBoardWindow();
		~InteractiveBoardWindow();

		/**
		 * @brief Returns a reference to the singleton instance of BoardData.
		 * @return Reference to BoardData instance.
		 */
		static InteractiveBoardWindow &instance()
		{
			static InteractiveBoardWindow instance;
			return instance;
		}

		/**
		 * @brief Returns a reference to the ImGuiBoard.
		 * @return Reference to ImGuiBoard.
		 */
		ImGuiBoard &imGuiBoard()
		{
			return *imGuiBoard_;
		}

		/**
		 * @brief Returns a reference to the ImGuiEngineList.
		 * @return Reference to ImGuiEngineList.
		 */
		ImGuiEngineList &imGuiEngineList()
		{
			return *imGuiEngineList_;
		}

		/**
		 * @brief Returns a reference to the ImGuiClock.
		 * @return Reference to ImGuiClock.
		 */
		ImGuiClock &imGuiClock()
		{
			return *imGuiClock_;
		}

		/**
		 * @brief Returns a reference to the ImGuiMoveList.
		 * @return Reference to ImGuiMoveList.
		 */
		ImGuiMoveList &imGuiMoveList()
		{
			return *imGuiMoveList_;
		}

		/**
		 * @brief Returns a const reference to the current GameRecord.
		 * @return Const reference to GameRecord.
		 */
		const GameRecord &gameRecord() const
		{
			return *gameRecord_;
		}

		/**
		 * @brief Returns a reference to the current GameRecord.
		 * @return Reference to GameRecord.
		 */
		GameRecord &gameRecord()
		{
			return *gameRecord_;
		}



		/**
		 * @brief Executes a move in the game.
		 * @param move The move to execute.
		 */
		void doMove(const MoveRecord& move);

		/**
		 * @brief Sets the position of the game.
		 * @param startPosition If true, sets the position to the starting position.
		 * @param fen Optional: FEN string to set the position. Must be provided if startPosition is false.
		 */
		void setPosition(bool startPosition, const std::string &fen = "");

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

		const EpdData &epdData() const
		{
			return epdData_;
		}
		EpdData &epdData()
		{
			return epdData_;
		}

		/**
		 * @brief Sets the game state from a GameRecord, if the new state extends the existing state.
		 * @param record The GameRecord to set the game state from.
		 */
		void setGameIfDifferent(const GameRecord &record);

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

		/**
		 * @brief stops the engine located at an index
		 * @param id Identifier of the engine to stop.
		 */
		void stopEngine(const std::string& id);

		/**
		 * @brief Restarts the engine located at an index.
		 * @param id Identifier of the engine to restart.
		 */
		void restartEngine(const std::string& id);

		/**
		 * @brief Starts the engines configured in the board data.
		 */
		void setEngines() {
			setEngines(engineConfigs_);
		}

		/**
		 * @brief Sets the engines to use
		 * @param engines Vector of EngineConfig objects representing the engines to set.
		 */
		void setEngines(const std::vector<EngineConfig> &engines);

		/**
		 * Returns true, if the given mode is active.
		 * @param mode The mode to check (e.g., "autoplay", "analyze", "play", "manual").
		 */
		bool isModeActive(const std::string &mode) const;


		/**
		 * @brief Saves the board configuration to the specified output stream in ini file format.
		 * @param out The output stream to write the configuration to.
		 */
		void saveConfig(std::ostream &out) const;

		/**
		 * @brief Loads a board engine configuration from a key-value map.
		 * @param keyValueMap A map containing key-value pairs representing the engine configuration.
		 */
		void loadBoardEngine(const QaplaHelpers::IniFile::Section &section);

		/**
		 * @brief Renders the interactive board window and its components.
		 * This method should be called within the main GUI rendering loop.
		 */
		void draw();

	private:
		/**
		 * @brief Initializes the InteractiveBoardWindow instance.
		 */
		void initSplitterWindows();

		EpdData epdData_;

		void checkForGameEnd();

		std::unique_ptr<GameRecord> gameRecord_;
		std::unique_ptr<ComputeTask> computeTask_;
		TimeControl timeControl_;

		std::unique_ptr<EmbeddedWindow> mainWindow_ = nullptr;
		std::unique_ptr<ImGuiBoard> imGuiBoard_;
		std::unique_ptr<ImGuiEngineList> imGuiEngineList_;
		std::unique_ptr<ImGuiClock> imGuiClock_;
		std::unique_ptr<ImGuiMoveList> imGuiMoveList_;
		std::unique_ptr<ImGuiBarChart> imGuiBarChart_;

		std::vector<EngineConfig> engineConfigs_;


	};

}