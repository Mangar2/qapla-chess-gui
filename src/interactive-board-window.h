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

#include "imgui-popup.h"
#include "engine-setup-window.h"
#include "embedded-window.h"
#include "callback-manager.h"

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

	class BoardWindow;
	class EngineWindow;
	class EngineSetupWindow;
	class ImGuiClock;
	class ImGuiMoveList;
	class ImGuiBarChart;

	class InteractiveBoardWindow : public EmbeddedWindow
	{
	public:
		/**
		 * @brief Constructs a new BoardData object.
		 */
		InteractiveBoardWindow();
		~InteractiveBoardWindow() override;

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
		 * @brief Creates and returns a unique pointer to a new InteractiveBoardWindow instance.
		 * Registeres a callback to poll data in a static instance of the CallbackManager.
		 * @return Unique pointer to a new InteractiveBoardWindow instance.
		 */
		static std::unique_ptr<InteractiveBoardWindow> createInstance() {
			auto instance = std::make_unique<InteractiveBoardWindow>();

			QaplaWindows::CallbackManager::instance().registerCallback([instance = instance.get()]() {
				instance->pollData();
			});

			return instance;
		}

		/**
		 * @brief Sets the position of the game.
		 * @param startPosition If true, sets the position to the starting position.
		 * @param fen Optional: FEN string to set the position. Must be provided if startPosition is false.
		 */
		void setPosition(bool startPosition, const std::string &fen = "");



		const EpdData &epdData() const
		{
			return epdData_;
		}
		EpdData &epdData()
		{
			return epdData_;
		}


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
		void draw() override;

	private:

		/**
		 * @brief Draws a popup window to select engines using EngineSetupWindow.
		 */
		void drawEngineSelectionPopup();

		/**
		 * @brief Opens the engine selection popup window.
		 * This sets the flag to open the popup, which will be drawn in the next calls.
		 */
		void openEngineSelectionPopup();

		/**
		 * @brief Swaps the engines assigned to white and black.
		 * It uses the GameContext swapPlayers method to perform the swap. It only sets a switched flag
		 * and will not restart the engines.
		 */
		void swapEngines();

		/**
		 * @brief Executes a move in the game.
		 * @param move The move to execute.
		 */
		void doMove(const MoveRecord& move);

		/**
		 * @brief Sets the current move index.
		 * @param moveIndex The move index to set.
		 */
		void setNextMoveIndex(uint32_t moveIndex);

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
		 * @brief Executes a command on the board window.
		 * Supported commands:
		 * - "New": Starts a new game with the initial position.
		 * - "Invert": Inverts the board orientation.
		 * - "Manual": Sets the mode to manual (user input only).
		 * - "Play": Sets the mode to play (user vs engine).
		 * - "Autoplay": Sets the mode to autoplay (engine vs engine).
		 * - "Analyze": Sets the mode to analyze (engine analysis only).
		 * @param command The command to execute.
		 */
		void execute(std::string command);

		/**
		 * @brief Initializes the InteractiveBoardWindow instance.
		 */
		void initSplitterWindows();

		/**
		 * @brief Stops all ongoing tasks.
		 */
		void stop();
		/**
		 * @brief Notifies task processor to play the side to move.
		 */
		void playSide();
		/**
		 * @brief Notifies task processor to analyze the current position.
		 */
		void analyze();
		/**
		 * @brief Notifies task processor to autoplay the game (both sides).
		 */
		void autoPlay();
		/**
		 * @brief Notifies task processor to set the start position.
		 * This will stop ongoing computations, reset all game state and sets the start position on the board.
		 */
		void setStartPosition();

		EpdData epdData_;

		void checkForGameEnd();

		std::unique_ptr<GameRecord> gameRecord_;
		std::unique_ptr<ComputeTask> computeTask_;
		TimeControl timeControl_;

		std::unique_ptr<EmbeddedWindow> mainWindow_ = nullptr;
		std::unique_ptr<BoardWindow> boardWindow_;
		std::unique_ptr<EngineWindow> engineWindow_;
		std::unique_ptr<ImGuiPopup<EngineSetupWindow>> setupWindow_;

		std::unique_ptr<ImGuiClock> imGuiClock_;
		std::unique_ptr<ImGuiMoveList> imGuiMoveList_;
		std::unique_ptr<ImGuiBarChart> imGuiBarChart_;

		std::vector<EngineConfig> engineConfigs_;

	};

}