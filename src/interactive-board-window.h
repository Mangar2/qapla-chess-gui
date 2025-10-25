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

namespace QaplaTester
{
	class EngineConfig;
	class GameState;
	class MoveRecord;
	class GameRecord;
	class ComputeTask;
}

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
		explicit InteractiveBoardWindow(uint32_t id);
		~InteractiveBoardWindow() override;

		/**
		 * @brief Returns a reference to the singleton instance of BoardData.
		 * @return Reference to BoardData instance.
		 */
		static InteractiveBoardWindow &instance()
		{
			static InteractiveBoardWindow instance(0);
			return instance;
		}

		/**
		 * @brief Creates and returns a unique pointer to a new InteractiveBoardWindow instance.
		 * Registeres a callback to poll data in a static instance of the CallbackManager.
		 * @return Unique pointer to a new InteractiveBoardWindow instance.
		 */
		static std::unique_ptr<InteractiveBoardWindow> createInstance();

		/**
		 * @brief Loads previously saved InteractiveBoardWindow instances from the configuration file.
		 * The instances are created based on the saved configuration data.
		 * @return A vector of unique pointers to the loaded InteractiveBoardWindow instances.
		 */
        static std::vector<std::unique_ptr<InteractiveBoardWindow>> loadInstances();

        /**
		 * @brief Returns the title of the board window.
		 * @return The title string, e.g., "Board 1".
		 */
		std::string getTitle() const {
			return "Board " + std::to_string(id_);
		}

		/**
		 * @brief Sets the position of the game.
		 * @param startPosition If true, sets the position to the starting position.
		 * @param fen Optional: FEN string to set the position. Must be provided if startPosition is false.
		 */
		void setPosition(bool startPosition, const std::string &fen = "");

		/**
		 * @brief Sets the position of the game from a GameRecord.
		 * @param gameRecord The GameRecord containing the position to set.
		 */
		void setPosition(const QaplaTester::GameRecord &gameRecord);

		/**
		 * @brief Polls data from several data provider (computeTask, edpData, ...) to provide them for imgui windows.
		 * This method must be called in the main gui loop to keep the data up-to-date.
		 */
		void pollData();

		/**
		 * @brief Stops all ongoing tasks in the pool.
		 */
		static void stopPool();

		/**
		 * @brief Stops all ongoing tasks and clears all task providers in the pool.
		 */
		static void clearPool();

		/**
		 * @brief Sets the pool concurrency level.
		 * @param count The number of concurrent tasks to allow.
		 * @param nice If true, reduces the number of active managers gradually.
		 * @param start If true, starts new tasks immediately.
		 */
		static void setPoolConcurrency(uint32_t count, bool nice = true, bool start = false);

		/**
		 * @brief Sets the engines to use
		 * @param engines Vector of EngineConfig objects representing the engines to set.
		 */
		void setEngines(const std::vector<QaplaTester::EngineConfig> &engines);

		/**
		 * @brief Renders the interactive board window and its components.
		 * This method should be called within the main GUI rendering loop.
		 */
		void draw() override;

	private:

		/**
		 * @brief Loads global engine settings from the provided section map into the given InteractiveBoardWindow instance.
		 * @param idStr The identifier string for the board (e.g., "board1").
		 */
		void loadGlobalEngineConfiguration(const std::string &idStr);

		/**
		 * @brief Loads board engine configurations from the provided section list.
		 * @param sectionList A list of INI file sections representing the board engine configurations.
		 */
		void loadBoardEnginesConfiguration(const QaplaHelpers::IniFile::SectionList &sectionList);

		/**
		 * @brief Draws a popup window to select engines using EngineSetupWindow.
		 */
		void drawEngineSelectionPopup();

		/**
		 * @brief Copies the given PV (principal variation) to the clipboard.
		 * @param id The ID of the engine.
		 * @param pv The PV string to copy.
		 */
		void copyPv(const std::string& id, const std::string& pv);

		/**
		 * @brief Converts the current game record to PGN string format.
		 * @param upToHalfmove Optional parameter to include moves only up to a specific halfmove.
		 *                     If 0, all moves are included.
		 * @return PGN string representation of the game.
		 */
		std::string computePgn(uint32_t upToHalfmove = 0);

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
		void doMove(const QaplaTester::MoveRecord& move);

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
		 * - "Auto": Sets the mode to autoplay (engine vs engine).
		 * - "Analyze": Sets the mode to analyze (engine analysis only).
		 * @param command The command to execute.
		 */
		void execute(const std::string& command);

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
		 * @brief Opens the time control dialog to configure game time settings.
		 * 
		 * Allows user to set time controls such as base time and increment
		 * (e.g., 5 minutes + 10 seconds per move).
		 */
		void openTimeControlDialog();
		
		void checkForGameEnd();

		std::unique_ptr<QaplaTester::GameRecord> gameRecord_;
		std::unique_ptr<QaplaTester::ComputeTask> computeTask_;
		QaplaTester::TimeControl timeControl_;

		std::unique_ptr<EmbeddedWindow> mainWindow_ = nullptr;
		std::unique_ptr<BoardWindow> boardWindow_;
		std::unique_ptr<EngineWindow> engineWindow_;
		std::unique_ptr<ImGuiPopup<EngineSetupWindow>> setupWindow_;

		std::unique_ptr<ImGuiClock> imGuiClock_;
		std::unique_ptr<ImGuiMoveList> imGuiMoveList_;
		std::unique_ptr<ImGuiBarChart> imGuiBarChart_;

		std::unique_ptr<Callback::UnregisterHandle> pollCallbackHandle_;
		std::unique_ptr<Callback::UnregisterHandle> gameUpdateHandle_;

		uint32_t id_;

	};

}