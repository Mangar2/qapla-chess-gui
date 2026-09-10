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

#include "callback-manager.h"
#include "game-manager-pool-access.h"
#include "imgui-engine-select.h"
#include "imgui-tournament-pgn.h"
#include "recent-files.h"
#include "viewer-board-window-list.h"

#include <chess-game/game-record.h>
#include <engine-handling/engine-config.h>
#include <opening/pgn-save.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace QaplaTester {
    class AnalysisManager;
}

class ImGuiConcurrency;

namespace QaplaWindows {

    /**
     * @brief The backward analysis of a set of games: what it runs on, and how far it has come.
     *
     * The games are handed over when the run starts and kept here, not read from the Pgn view
     * again: from that moment the view is free for anything else, and loading the file the
     * analysis is writing into it is a normal thing to want to do while it runs.
     *
     * One AnalysisManager per engine, all scheduled at once. The manager hands out one game per
     * task and a run walks it backwards, so two engines analysing the same games are two
     * independent runs that happen to share a pool and an output file - each writes its own copy
     * of every game, told apart by the Annotator tag the manager sets.
     */
    class AnalysisData {
    public:
        /**
         * @brief What the run does, as far as anything outside needs to know.
         */
        enum class State : std::uint8_t {
            Stopped,      ///< Nothing running
            Starting,     ///< Scheduled, no game manager has picked it up yet
            Running,      ///< At least one game is being analysed
            Stopping,     ///< Asked to stop at once
            Gracefully    ///< Asked to stop once the running games are finished
        };

        /**
         * @brief The settings of a run that are not the engines and not the output file.
         */
        struct Config {
            std::string inputFile;        ///< The PGN the games were read from, for the record
            uint64_t moveTimeMs = 1000;   ///< Fixed time every position is given
        };

        AnalysisData();
        ~AnalysisData();

        AnalysisData(const AnalysisData&) = delete;
        AnalysisData& operator=(const AnalysisData&) = delete;

        static AnalysisData& instance() {
            static AnalysisData instance;
            return instance;
        }

        /** @brief Keeps the pool state, the counters and the boards current; once per frame. */
        void pollData();

        [[nodiscard]] Config& config() { return config_; }
        [[nodiscard]] const Config& config() const { return config_; }

        /** @brief The engines that analyse; the games are analysed once by each of them. */
        [[nodiscard]] ImGuiEngineSelect& getEngineSelect() { return *engineSelect_; }

        /** @brief The output file, its append mode and what is written into the comments. */
        [[nodiscard]] ImGuiTournamentPgn& outputPgn() { return *outputPgn_; }

        [[nodiscard]] RecentFiles& recentInputFiles() { return recentInputFiles_; }
        [[nodiscard]] RecentFiles& recentOutputFiles() { return recentOutputFiles_; }

        /**
         * @brief Hands the games over to the analysis.
         *
         * Called with what the Pgn view holds after filtering, at the moment the run starts.
         *
         * @param games The games to analyse.
         */
        void setGames(std::vector<QaplaTester::GameRecord> games);

        /** @brief The number of games handed over. */
        [[nodiscard]] size_t getGameCount() const { return games_.size(); }

        /**
         * @brief Reads the games to analyse from the configured input file.
         *
         * The other way in is setGames(), which is what the chatbot uses: there the games come
         * from the Pgn view, where the user has just seen and filtered them. A caller without a
         * screen has no such view, and the file is all it has -- so it reads the file, whole.
         *
         * @param error Filled with what went wrong when nothing was read.
         * @return The number of games read; zero when the file could not be used.
         */
        size_t loadGamesFromInputFile(std::string& error);

        /**
         * @brief Checks what a run needs, reporting the first thing that is missing.
         * @param sendMessage If true, says through the snackbar what is missing.
         * @return True if the analysis may be started.
         */
        [[nodiscard]] bool mayAnalyze(bool sendMessage = false) const;

        /**
         * @brief Starts the analysis with the configured engines, games and output file.
         */
        void analyse();

        /**
         * @brief Ends a running analysis.
         * @param graceful If true, the games being analysed are finished first.
         */
        void stop(bool graceful);

        [[nodiscard]] State getState() const { return state_; }
        [[nodiscard]] bool isRunning() const { return state_ == State::Running; }
        [[nodiscard]] bool isStarting() const { return state_ == State::Starting; }
        [[nodiscard]] bool isBusy() const { return state_ != State::Stopped; }

        /** @brief Games finished so far, counting every engine's copy of a game. */
        [[nodiscard]] size_t getFinishedCount() const;

        /** @brief Games to be analysed in total, counting every engine's copy of a game. */
        [[nodiscard]] size_t getTotalCount() const { return totalCount_; }

        /** @brief Whether a board is showing a game that is being recomputed right now. */
        [[nodiscard]] bool hasRunningBoards() const { return viewerBoardWindows_.isAnyRunning(); }

        /**
         * @brief What the boards of the running games say they are showing.
         *
         * The same text their tabs carry as a tooltip -- the game's number, its two players and
         * how it ended. For a caller that cannot see the screen: a test, or an answer to someone
         * asking what the run is working on.
         */
        [[nodiscard]] std::vector<std::string> runningBoardLabels() const;

        [[nodiscard]] uint32_t getExternalConcurrency() const;
        void setExternalConcurrency(uint32_t count);

        /**
         * @brief Changes how many games are analysed at the same time while a run is going on.
         * @param count The number of concurrent games to allow.
         * @param nice If true, idle game managers are reduced gradually.
         * @param direct If true, applies the change immediately without debouncing.
         */
        void setPoolConcurrency(uint32_t count, bool nice = true, bool direct = false);

        /** @brief Writes the settings to the configuration. */
        void updateConfiguration() const;

    private:
        /** @brief Reads the settings from the configuration. */
        void init();

        /** @brief The engines to analyse with, each carrying the configured per-move limit. */
        [[nodiscard]] std::vector<QaplaTester::EngineConfig> analysisEngines() const;

        Config config_;
        std::vector<QaplaTester::GameRecord> games_;
        std::unique_ptr<ImGuiEngineSelect> engineSelect_;
        std::unique_ptr<ImGuiTournamentPgn> outputPgn_;
        std::unique_ptr<ImGuiConcurrency> imguiConcurrency_;
        RecentFiles recentInputFiles_{ "analysis-input" };
        RecentFiles recentOutputFiles_{ "analysis-output" };

        /** @brief Where the analysed games are written; the analysis has its own, not the tournament's. */
        QaplaTester::PgnSave pgnSave_;
        /** @brief One per engine, all scheduled together. */
        std::vector<std::shared_ptr<QaplaTester::AnalysisManager>> managers_;
        GameManagerPoolAccess poolAccess_;
        /**
         * @brief One board per game being recomputed, shown as tabs beside the other boards.
         *
         * Registers itself and is drawn by ViewerBoardWindowList::drawAllTabs(), the same way the
         * tournament, the SPRT test and the EPD run show what they are working on.
         */
        ViewerBoardWindowList viewerBoardWindows_{ "Analysis" };
        State state_ = State::Stopped;
        size_t totalCount_ = 0;
        /**
         * @brief Whether a game was ever seen being analysed in this run.
         *
         * A run whose engines never start still ends -- the pool marks every game as finished
         * without one -- and then looks exactly like a run that went through. This is how the
         * two are told apart, so that a mistyped engine says so instead of finishing silently.
         */
        bool sawRunningGame_ = false;

        std::unique_ptr<Callback::UnregisterHandle> pollCallbackHandle_;
    };

}
