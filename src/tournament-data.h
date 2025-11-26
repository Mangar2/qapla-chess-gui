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


#include "viewer-board-window.h"
#include "viewer-board-window-list.h"
#include "imgui-table.h"
#include "imgui-causes-table.h"
#include "imgui-engine-select.h"
#include "imgui-engine-global-settings.h"
#include "imgui-tournament-opening.h"
#include "imgui-tournament-pgn.h"
#include "imgui-tournament-adjudication.h"
#include "imgui-tournament-configuration.h"
#include "game-manager-pool-access.h"
#include "callback-manager.h"

#include "ini-file.h"
#include "engine-option.h"
#include "engine-config.h"
#include "pgn-io.h"
#include "adjudication-manager.h"
#include "time-control.h"
#include "logger.h"

#include <memory>
#include <optional>
#include <ostream>

namespace QaplaTester
{
    class Tournament;
    struct TournamentConfig;
}

class ImGuiConcurrency;

namespace QaplaWindows {

    class TournamentResultIncremental;

	class TournamentData {
    public: 
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
         * @param verbose If true, enables snackbar info on success.
         */
        void startTournament(bool verbose = false);

        /**
		 * @brief Polls the Tournament data for new entries.
		 */
        void pollData();

        /**
		 * @brief Clears the current analysis results.
         * @param verbose If true, enables snackbar infos.
		 */
        void clear(bool verbose = true);

        /**
         * @brief Stops all ongoing tasks in the pool.
         * @param graceful If true, stops tasks gracefully, allowing games to finish.
         */
        void stopPool(bool graceful = false);

        /**
         * @brief Gets the target pool concurrency level.
         * @return The target concurrency level.
         */
        uint32_t getExternalConcurrency() const;

        /**
         * @brief Sets the external concurrency value.
         * @param count The new external concurrency value.
         */
        void setExternalConcurrency(uint32_t count);

        /**
         * @brief Sets the pool concurrency level.
         * @param count The number of concurrent tasks to allow.
         * @param nice If true, reduces the number of active managers gradually.
         * @param direct If true, applies the change immediately without debouncing.
         */
        void setPoolConcurrency(uint32_t count, bool nice = true, bool direct = false);

        /**
         * @brief Sets the concurrency level for the tournament set/displayed by the ui.
         * 
         * It is different to the current concurrency of the pool due to
         * - debouncing
         * - stop state (in which case pool concurrency is 0)
         * 
         * @param count The desired concurrency level.
         */
        void setUiConcurrency(uint32_t count);

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
         * @brief Draws the table displaying adjudication test results.
         * @param size Size of the table to draw.
         */
        void drawAdjudicationTable(const ImVec2& size);

        /**
         * @brief Returns a reference to the tournament configuration.
         * @return A reference to the tournament configuration.
         */
        QaplaTester::TournamentConfig& config();

        /**
         * @brief Sets the engine configurations for the tournament
         * @param configurations Vector with all engine configurations
         */
        void setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration> &configurations);
 
        /**
         * @brief Sets the GameManagerPool instance to use.
         * @param pool Shared pointer to a GameManagerPool instance.
         */
        void setGameManagerPool(const std::shared_ptr<QaplaTester::GameManagerPool>& pool);
        
        /**
         * @brief Returns a reference to the engine selection.
         * @return Reference to the engine selection.
         */
        ImGuiEngineSelect& engineSelect() {
            return *engineSelect_;
        }

        /**
         * @brief Returns a const reference to the engine selection.
         * @return Const reference to the engine selection.
         */
        const ImGuiEngineSelect& engineSelect() const {
            return *engineSelect_;
        }

        /**
         * @brief Returns a reference to the global engine settings.
         * @return Reference to the global engine settings.
         */
        ImGuiEngineGlobalSettings& globalSettings() {
            return *globalSettings_;
        }

        /**
         * @brief Returns a const reference to the global engine settings.
         * @return Const reference to the global engine settings.
         */
        const ImGuiEngineGlobalSettings& globalSettings() const {
            return *globalSettings_;
        }

        ImGuiTournamentOpening& tournamentOpening() {
            return *tournamentOpening_;
        }

        const ImGuiTournamentOpening& tournamentOpening() const {
            return *tournamentOpening_;
        }

        /**
         * @brief Returns a reference to the tournament configuration UI component.
         * @return Reference to the tournament configuration UI component.
         */
        ImGuiTournamentConfiguration& tournamentConfiguration() {
            return *tournamentConfiguration_;
        }

        /**
         * @brief Returns a const reference to the tournament configuration UI component.
         * @return Const reference to the tournament configuration UI component.
         */
        const ImGuiTournamentConfiguration& tournamentConfiguration() const {
            return *tournamentConfiguration_;
        }

        /**
         * @brief Returns a reference to the PGN output configuration.
         * @return Reference to the PGN output configuration.
         */
        ImGuiTournamentPgn& tournamentPgn() {
            return *tournamentPgn_;
        }

        /**
         * @brief Returns a const reference to the PGN output configuration.
         * @return Const reference to the PGN output configuration.
         */
        const ImGuiTournamentPgn& tournamentPgn() const {
            return *tournamentPgn_;
        }

        const QaplaTester::PgnIO::Options& pgnConfig() const {
            return tournamentPgn_->pgnOptions();
        }

        QaplaTester::PgnIO::Options& pgnConfig() {
            return tournamentPgn_->pgnOptions();
        }

        /**
         * @brief Returns a reference to the adjudication configuration.
         * @return Reference to the adjudication configuration.
         */
        ImGuiTournamentAdjudication& tournamentAdjudication() {
            return *tournamentAdjudication_;
        }

        /**
         * @brief Returns a const reference to the adjudication configuration.
         * @return Const reference to the adjudication configuration.
         */
        const ImGuiTournamentAdjudication& tournamentAdjudication() const {
            return *tournamentAdjudication_;
        }

        const QaplaTester::AdjudicationManager::DrawAdjudicationConfig& drawConfig() const {
            return tournamentAdjudication_->drawConfig();
        }
        QaplaTester::AdjudicationManager::DrawAdjudicationConfig& drawConfig() {
            return tournamentAdjudication_->drawConfig();
        }
        const QaplaTester::AdjudicationManager::ResignAdjudicationConfig& resignConfig() const {
            return tournamentAdjudication_->resignConfig();
        }
        QaplaTester::AdjudicationManager::ResignAdjudicationConfig& resignConfig() {
            return tournamentAdjudication_->resignConfig();
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
         * @brief Returns true if the tournament is in the starting state.
         * @return True if the tournament is starting, false otherwise.
         */
        bool isStarting() const {
            return state_ == State::Starting;
        }

        /**
         * @brief Returns the current state of the tournament.
         * @return The current state of the tournament.
         */
        State getState() const {
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
         * @brief Returns the total number of games scheduled in the tournament.
         * @return Total number of games that will be played.
         */
        uint32_t getTotalScheduledGames() const;

        /**
         * @brief Calculates the total number of games based on current configuration.
         * This works even before the tournament is scheduled.
         * @return Total number of games that would be played with current settings.
         */
        uint32_t getTotalGames() const;

        /**
         * @brief Checks if the tournament is finished (all games played).
         * @return True if the tournament is finished, false otherwise.
         */
        bool isFinished() const;

        /**
         * @brief Returns the number of games that have been completed.
         * @return Number of finished games.
         */
        uint32_t getPlayedGames() const;

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
  
        /**
         * @brief Creates the tournament.
         * @param verbose If true, verbose error messages will be shown.
         * @return True if the tournament was created successfully, false otherwise.
         */
        bool createTournament(bool verbose);

        /**
         * @brief Builds a list of selected engines with global settings applied.
         * @return Vector of EngineConfig for all selected engines.
         */
        std::vector<QaplaTester::EngineConfig> getSelectedEngines() const;

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
         * @brief Loads the PGN configuration from configuration data.
         */
        void loadPgnConfig();

        /**
         * @brief Loads the engine selection configuration from configuration data.
         */
        void loadEngineSelectionConfig();

        /**
         * @brief Loads the global engine settings configuration from configuration data.
         */
        void loadGlobalSettingsConfig();

        /**
         * @brief Sets up callbacks for engine selection and global settings.
         */
        void setupCallbacks();

        /**
         * @brief Activates the board view for a specific game index.
         * @param gameIndex The index of the game to activate the board view for.
         */
        void activateBoardView(size_t gameIndex);

        void populateEloTable();
		void populateRunningTable();
        void populateCauseTable();

        void populateAdjudicationTable();
        void populateResignTest(QaplaTester::AdjudicationManager::TestResults &results);
        void populateDrawTest(QaplaTester::AdjudicationManager::TestResults &results);

        ViewerBoardWindowList boardWindowList_{"Tournament"};

        std::unique_ptr<QaplaTester::Tournament> tournament_{std::make_unique<QaplaTester::Tournament>()};
        std::unique_ptr<QaplaTester::TournamentConfig> config_{std::make_unique<QaplaTester::TournamentConfig>()};
        std::unique_ptr<TournamentResultIncremental> result_{std::make_unique<TournamentResultIncremental>()};
        std::unique_ptr<ImGuiConcurrency> imguiConcurrency_{std::make_unique<ImGuiConcurrency>()};
        std::unique_ptr<ImGuiEngineSelect> engineSelect_{std::make_unique<ImGuiEngineSelect>()};
        std::unique_ptr<ImGuiEngineGlobalSettings> globalSettings_{std::make_unique<ImGuiEngineGlobalSettings>()};
        std::unique_ptr<ImGuiTournamentOpening> tournamentOpening_{std::make_unique<ImGuiTournamentOpening>()};
        std::unique_ptr<ImGuiTournamentPgn> tournamentPgn_{std::make_unique<ImGuiTournamentPgn>()};
        std::unique_ptr<ImGuiTournamentAdjudication> tournamentAdjudication_{std::make_unique<ImGuiTournamentAdjudication>()};
        std::unique_ptr<ImGuiTournamentConfiguration> tournamentConfiguration_{std::make_unique<ImGuiTournamentConfiguration>()};

        GameManagerPoolAccess poolAccess_;
        ImGuiEngineGlobalSettings::GlobalConfiguration eachEngineConfig_;
        ImGuiEngineGlobalSettings::TimeControlSettings timeControlSettings_;
        std::vector<ImGuiEngineSelect::EngineConfiguration> engineConfigurations_; 
        std::unique_ptr<Callback::UnregisterHandle> pollCallbackHandle_;
        std::unique_ptr<Callback::UnregisterHandle> messageCallbackHandle_;

		uint32_t runningCount_ = 0; ///< Number of currently running games

        ImGuiTable eloTable_;
        ImGuiTable runningTable_;
        ImGuiCausesTable causesTable_;
        ImGuiTable adjudicationTable_;

        State state_ = State::Stopped;
        bool loadedTournamentData_ = false;

        // List of all section names used
        static constexpr std::array<const char*, 9> sectionNames = {
            "eachengine",
            "engineselection",
            "tournament",
            "opening",
            "pgnoutput",
            "drawadjudication",
            "resignadjudication",
            "timecontroloptions",
            "round"
        };

    };

}
