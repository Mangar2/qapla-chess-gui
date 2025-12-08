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

#include "viewer-board-window-list.h"
#include "imgui-engine-select.h"
#include "imgui-tournament-opening.h"
#include "imgui-tournament-pgn.h"
#include "imgui-tournament-adjudication.h"
#include "imgui-engine-global-settings.h"
#include "imgui-sprt-configuration.h"
#include "imgui-causes-table.h"
#include "game-manager-pool-access.h"
#include "callback-manager.h"

#include <memory>
#include <vector>
#include <cstdint>

// Forward declarations
namespace QaplaTester {
    struct SprtConfig;
    class GameManagerPool;
    class SprtManager;
    struct MonteCarloResult;
}
class ImGuiConcurrency;

namespace QaplaWindows {

    class SprtTournamentData {
    public:
        enum class State {
            Stopped,
            Starting,
            Running,
            GracefulStopping
        };

        SprtTournamentData();
        ~SprtTournamentData();

        /**
         * @brief Returns the singleton instance of SprtTournamentData.
         * @return Reference to the SprtTournamentData singleton.
         */
        static SprtTournamentData& instance() {
            static SprtTournamentData instance;
            return instance;
        }

        /**
         * @brief Starts the SPRT tournament.
         */
        void startTournament();

        /**
         * @brief Polls the SPRT tournament data for new entries.
         */
        void pollData();

        /**
         * @brief Clears the current SPRT tournament results.
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
         * @param direct If true, applies the change immediately without debouncing.
         */
        void setPoolConcurrency(uint32_t count, bool nice = true, bool direct = false);

        /**
         * @brief Sets the GameManagerPool instance to use.
         * @param pool Shared pointer to a GameManagerPool instance.
         */
        void setGameManagerPool(const std::shared_ptr<QaplaTester::GameManagerPool>& pool);

        /**
         * @brief Checks if the SPRT tournament is currently running.
         * @return true if running, false otherwise
         */
        bool isRunning() const {
            return state_ != State::Stopped;
        }

        /**
         * @brief Checks if any operation (SPRT tournament or Monte Carlo test) is currently running.
         * @return true if SPRT tournament or Monte Carlo test is running, false otherwise
         */
        bool isAnyRunning() const;

        /**
         * @brief Checks if the SPRT tournament has finished.
         * @details The tournament is finished if a decision has been made or the max games limit reached.
         *          This always queries the SprtManager and does not use cached state.
         * @return true if the tournament has finished, false otherwise.
         */
        bool isFinished() const;

        /**
         * @brief Returns the current state of the SPRT tournament.
         * @return The current state.
         */
        State state() const {
            return state_;
        }

        /**
         * @brief Checks if there are tasks scheduled.
         * @return true if tasks are scheduled, false otherwise
         */
        bool hasResults() const;

        /**
         * @brief Checks if the SPRT tournament is currently starting.
         * @return true if starting, false otherwise
         */
        bool isStarting() const {
            return state_ == State::Starting;
        }

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
         * @brief Returns a reference to the tournament opening configuration.
         * @return Reference to the tournament opening configuration.
         */
        ImGuiTournamentOpening& tournamentOpening() {
            return *tournamentOpening_;
        }

        /**
         * @brief Returns a const reference to the tournament opening configuration.
         * @return Const reference to the tournament opening configuration.
         */
        const ImGuiTournamentOpening& tournamentOpening() const {
            return *tournamentOpening_;
        }

        /**
         * @brief Returns a reference to the tournament PGN configuration.
         * @return Reference to the tournament PGN configuration.
         */
        ImGuiTournamentPgn& tournamentPgn() {
            return *tournamentPgn_;
        }

        /**
         * @brief Returns a const reference to the tournament PGN configuration.
         * @return Const reference to the tournament PGN configuration.
         */
        const ImGuiTournamentPgn& tournamentPgn() const {
            return *tournamentPgn_;
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

        /**
         * @brief Returns a reference to the SPRT configuration.
         * @return Reference to the SPRT configuration.
         */
        QaplaTester::SprtConfig& sprtConfig();

        /**
         * @brief Returns a const reference to the SPRT configuration.
         * @return Const reference to the SPRT configuration.
         */
        const QaplaTester::SprtConfig& sprtConfig() const;

        /**
         * @brief Returns a const reference to the tournament results.
         * @return Const reference to the current tournament results (as TournamentResult).
         */
        QaplaTester::TournamentResult getTournamentResult() const;

        /**
         * @brief Returns a reference to the SPRT configuration UI component.
         * @return Reference to the SPRT configuration UI component.
         */
        ImGuiSprtConfiguration& sprtConfiguration() {
            return *sprtConfiguration_;
        }

        /**
         * @brief Returns a const reference to the SPRT configuration UI component.
         * @return Const reference to the SPRT configuration UI component.
         */
        const ImGuiSprtConfiguration& sprtConfiguration() const {
            return *sprtConfiguration_;
        }

        /**
         * @brief Sets the engine configurations for the SPRT tournament.
         * @param configurations Vector containing all engine configurations.
         */
        void setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations);

        /**
         * @brief Creates the SPRT tournament with the configured engines and settings.
         * @param verbose Whether to show error messages
         * @return true if tournament creation was successful, false otherwise
         */
        bool createTournament(bool verbose);

        /**
         * @brief Loads tournament results from configuration.
         * @details Creates the tournament and then loads saved results from the configuration.
         */
        void loadTournament();

        /**
         * @brief Updates the tournament results in the Configuration singleton.
         * @details This method creates Section entries for SPRT tournament result data
         *          and stores them in the Configuration singleton using setSectionList.
         */
        void updateTournamentResults();

        /**
         * @brief Draws the table displaying the current SPRT duel result.
         * @param size Size of the table to draw.
         */
        void drawResultTable(const ImVec2& size);

        /**
         * @brief Draws the table displaying the SPRT test result.
         * @param size Size of the table to draw.
         */
        void drawSprtTable(const ImVec2& size);

        /**
         * @brief Draws the table displaying the causes for game termination.
         * @param size Size of the table to draw.
         */
        void drawCauseTable(const ImVec2& size);

        /**
         * @brief Draws the table displaying the Monte Carlo test results.
         * @param size Size of the table to draw.
         */
        void drawMonteCarloTable(const ImVec2& size);

        /**
         * @brief Saves all SPRT tournament data including configuration and results to a file.
         * @param filename The file path to save the tournament data to.
         */
        static void saveTournament(const std::string& filename);

        /**
         * @brief Loads all SPRT tournament data from a file.
         * @param filename The file path to load the tournament data from.
         */
        void loadTournament(const std::string& filename);

        /**
         * @brief Runs a Monte Carlo test on the current SPRT configuration.
         * @return true if test was started, false if a test is already running.
         */
        bool runMonteCarloTest();

        /**
         * @brief Checks if a Monte Carlo test is currently running.
         * @return true if running, false otherwise.
         */
        bool isMonteCarloTestRunning() const;

        /**
         * @brief Stops any running Monte Carlo test.
         */
        void stopMonteCarloTest();

        /**
         * @brief Activates the board view for a specific game index.
         * @param gameIndex The index of the game to activate the board view for.
         */
        void activateBoardView(size_t gameIndex);

    private:
        /**
         * @brief Populates the result table with current duel result data.
         * @details Fills the table with engineA, engineB, rating from engineA's perspective, and game count.
         */
        void populateResultTable();

        /**
         * @brief Populates the SPRT table with current SPRT test result.
         * @details Fills the table with engineA, engineB, lowerBound, llr, upperBound, and decision info.
         */
        void populateSprtTable();

        /**
         * @brief Populates the causes table with game termination causes.
         * @details Fills the table with termination causes for both engines in the current duel.
         */
        void populateCausesTable();

        /**
         * @brief Populates the Monte Carlo table with test results.
         * @details Fills the table with simulation results from the Monte Carlo test using a callback.
         */
        void populateMonteCarloTable();

        /**
         * @brief Sets up callbacks for UI component changes.
         * @details Registers callbacks for engine selection changes.
         */
        void setupCallbacks();

        /**
         * @brief Loads the engine selection configuration from the configuration file.
         * @details Retrieves engine selection settings from the "engineselection" section
         *          with ID "sprt-tournament" and applies them to the engine selector.
         */
        void loadEngineSelectionConfig();

        /**
         * @brief Loads the global engine settings configuration from the configuration file.
         * @details Retrieves global engine and time control settings from the configuration
         *          sections with ID "sprt-tournament".
         */
        void loadGlobalSettingsConfig();

        ViewerBoardWindowList boardWindowList_;
        ImGuiTable resultTable_;
        ImGuiTable sprtTable_;
        ImGuiCausesTable causesTable_;
        ImGuiTable montecarloTable_;

        std::unique_ptr<ImGuiEngineSelect> engineSelect_;
        std::unique_ptr<ImGuiTournamentOpening> tournamentOpening_;
        std::unique_ptr<ImGuiTournamentPgn> tournamentPgn_;
        std::unique_ptr<ImGuiTournamentAdjudication> tournamentAdjudication_;
        std::unique_ptr<ImGuiEngineGlobalSettings> globalSettings_;
        std::unique_ptr<ImGuiSprtConfiguration> sprtConfiguration_;
        std::shared_ptr<QaplaTester::SprtManager> sprtManager_;
        std::unique_ptr<QaplaTester::SprtConfig> sprtConfig_;
        std::unique_ptr<ImGuiConcurrency> imguiConcurrency_;
        GameManagerPoolAccess poolAccess_;
        std::vector<ImGuiEngineSelect::EngineConfiguration> engineConfigurations_;

        std::unique_ptr<Callback::UnregisterHandle> pollCallbackHandle_;
        std::unique_ptr<Callback::UnregisterHandle> messageCallbackHandle_;

        ImGuiEngineGlobalSettings::GlobalConfiguration eachEngineConfig_;
        ImGuiEngineGlobalSettings::TimeControlSettings timeControlSettings_;

        State state_ = State::Stopped;

        // List of all section names used
        static constexpr std::array<const char*, 9> sectionNames = {
            "eachengine",
            "engineselection",
            "sprtconfig",
            "opening",
            "pgnoutput",
            "drawadjudication",
            "resignadjudication",
            "timecontroloptions",
            "round"
        };
    };

}
