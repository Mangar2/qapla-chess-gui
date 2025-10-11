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


#include "tournament-board-window.h"
#include "imgui-table.h"
#include "imgui-engine-select.h"

#include "qapla-tester/ini-file.h"
#include "qapla-tester/engine-option.h"
#include "qapla-tester/engine-config.h"
#include "qapla-tester/pgn-io.h"
#include "qapla-tester/adjudication-manager.h"
#include "qapla-tester/time-control.h"
#include "qapla-tester/logger.h"

#include <memory>
#include <optional>
#include <ostream>

class Tournament;
struct TournamentConfig;

class ImGuiConcurrency;

namespace QaplaWindows {

    class TournamentResultIncremental;

	class TournamentData {
    public: 
        struct EachEngineConfig {
            std::string tc;
			std::string restart;
			std::string traceLevel;
            uint32_t hash;
            std::string ponder;

		};

        enum class State {
            Stopped,
            Starting,
            Running,
            GracefulStopping
        };

        TournamentData();
        ~TournamentData();

        /**
         * @brief Starts the tournament.
         */
        void startTournament();

        /**
		 * @brief Polls the Tournament data for new entries.
		 */
        void pollData();

        /**
		 * @brief Clears the current analysis results.
		 */
        void clear();

        /**
         * @brief Stops all ongoing tasks in the pool.
         * @param graceful If true, stops tasks gracefully, allowing games to finish.
         */
        void stopPool(bool graceful = false);

        /**
         * @brief Sets the pool concurrency level.
         * @param count The number of concurrent tasks to allow.
         * @param nice If true, reduces the number of active managers gradually.
         */
        void setPoolConcurrency(uint32_t count, bool nice = true);

        /**
         * @brief Draws the tournament elo table.
         * @param size Size of the table to draw.
         * @return The index of the selected row, or std::nullopt if no row was selected.
		 */
        std::optional<size_t> drawEloTable(const ImVec2& size);

        /**
         * @brief Draws the table displaying the running tournament pairings.
         * @param size Size of the table to draw.
         * @return The index of the selected row, or std::nullopt if no row was selected.
         */
        std::optional<size_t> drawRunningTable(const ImVec2& size);

        /**
         * @brief Draws the table displaying the causes for game termination.
         * @param size Size of the table to draw.
         */
        void drawCauseTable(const ImVec2& size);

        /**
         * @brief Draws the tournament tabs showing boards for all running games.
         */
        void drawTabs();

        /**
         * @brief Returns a reference to the tournament configuration.
         * @return A reference to the tournament configuration.
         */
        TournamentConfig& config();

        /**
         * @brief Sets the engine configurations for the tournament
         * @param configurations Vector with all engine configurations
         */
        void setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration> &configurations);
 
        uint32_t& concurrency() {
            return concurrency_;
		}

        const PgnIO::Options& pgnConfig() const {
            return pgnConfig_;
		}

        PgnIO::Options& pgnConfig() {
            return pgnConfig_;
        }

        const EachEngineConfig& eachEngineConfig() const {
            return eachEngineConfig_;
		}

        EachEngineConfig& eachEngineConfig() {
            return eachEngineConfig_;
		}

        const AdjudicationManager::DrawAdjudicationConfig& drawConfig() const {
            return drawConfig_;
        }
        AdjudicationManager::DrawAdjudicationConfig& drawConfig() {
            return drawConfig_;
        }
		const AdjudicationManager::ResignAdjudicationConfig& resignConfig() const {
            return resignConfig_;
        }
        AdjudicationManager::ResignAdjudicationConfig& resignConfig() {
            return resignConfig_;
        }

        /**
         * @brief Updates the configuration in the singleton.
         * @details This method creates Section entries for all configuration aspects and stores them
         *          in the Configuration singleton using setSectionList.
         */
        void updateConfiguration();

        /**
         * @brief Updates the tournament data in the singleton.
         * @details This method creates Section entries for tournament result data
         */
        void updateTournamentResults();
       
        /**
         * @brief Returns a reference to the tournament data singleton.
         * @return A reference to the tournament data singleton.
         */
        static TournamentData& instance() {
            static TournamentData instance;
            return instance;
		}

        /**
         * @brief Returns true if the tournament is currently running.
         * @return True if the tournament is running, false otherwise.
         */
        bool isRunning() const;

        /**
         * @brief Returns the current state of the tournament.
         * @return The current state of the tournament.
         */
        State state() const {
            return state_;
        }

        /**
         * @brief Checks if the tournament data is available and has games left to play.
         * @return True if the tournament data is available and has games left, false otherwise.
         */
        bool isAvailable() const;

        /**
         * @brief Checks if the tournament has started (tasks have been scheduled) no matter if it is running.
         * @return True if the tournament has started, false otherwise.
         */
        bool hasTasksScheduled() const;

        /**
         * @brief Saves all tournament data including configuration and results to a file.
         * @param filename The file path to save the tournament data to.
         */
        static void saveTournament(const std::string& filename);

        /**
         * @brief Loads all tournament data from a file.
         * @param filename The file path to load the tournament data from.
         */
        void loadTournament(const std::string& filename);

	private:
        bool validateOpenings();

        /**
         * @brief Creates the tournament.
         * @param verbose If true, verbose error messages will be shown.
         * @return True if the tournament was created successfully, false otherwise.
         */
        bool createTournament(bool verbose);

        /**
        * @brief Creates a tournament and loads it from the current configuration and engine settings.
        */
        void loadTournament();
        
        /**
         * @brief Loads the tournament configuration from a list of INI file sections.
         * @details This method processes each section and loads the corresponding configuration data
         *          into the tournament data structure.
         */
        void loadConfig();


         /**
         * @brief Loads the tournament openings from configuration.
         * @details This method loads opening settings from the configuration data.
		 */
        void loadOpenings();

        /**
         * @brief Loads the tournament configuration from configuration data.
         * @details This method loads tournament settings from the configuration data.
         */
        void loadTournamentConfig();

        /**
         * @brief Loads the configuration for each engine from configuration data.
         */
        void loadEachEngineConfig();

        /**
         * @brief Loads the PGN configuration from configuration data.
         */
        void loadPgnConfig();

        /**
         * @brief Loads the draw adjudication configuration from configuration data.
         */
        void loadDrawAdjudicationConfig();

        /**
         * @brief Loads the resign adjudication configuration from configuration data.
         */
        void loadResignAdjudicationConfig();

    private:
        void populateEloTable();
		void populateRunningTable();
        void populateCauseTable();

        void populateViews();

        std::vector<TournamentBoardWindow> boardWindow_;

		std::unique_ptr<Tournament> tournament_;
        std::unique_ptr<TournamentConfig> config_;
        std::unique_ptr<TournamentResultIncremental> result_;
        std::unique_ptr<ImGuiConcurrency> imguiConcurrency_;

        PgnIO::Options pgnConfig_;
		EachEngineConfig eachEngineConfig_;
		AdjudicationManager::DrawAdjudicationConfig drawConfig_;
		AdjudicationManager::ResignAdjudicationConfig resignConfig_;
        std::vector<ImGuiEngineSelect::EngineConfiguration> engineConfigurations_{};

        uint32_t concurrency_ = 1;
		uint32_t runningCount_ = 0; ///< Number of currently running games

        ImGuiTable eloTable_;
        ImGuiTable runningTable_;
        ImGuiTable causeTable_;


        int32_t selectedIndex_ = 0;
        State state_ = State::Stopped;

        bool loadedTournamentData_ = false;

        // List of all section names used
        static constexpr std::array<const char*, 8> sectionNames = {
            "eachengine",
            "engineselection",
            "tournament",
            "opening",
            "pgnoutput",
            "drawadjudication",
            "resignadjudication",
            "round"
        };

    };

}
