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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#pragma once

#include "callback-manager.h"
#include "game-manager-pool-access.h"
#include "imgui-engine-select.h"
#include "recent-files.h"

#include <engine-handling/engine-config.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace QaplaTester {
    class BookEvalManager;
}

class ImGuiConcurrency;

namespace QaplaWindows {

    /**
     * @brief The evaluation of the positions of a 9 bit book: what it runs on, and how far it has come.
     *
     * The run is the one of qapla-engine-tester --bookeval, of version 2: every position of the book -
     * the start position and the one after every move - is searched once, cold, every move gets the
     * score of the position it leads to, and the scores go into a new book, which is also the state
     * the next run continues from. A run that is stopped keeps what it has; starting it again with
     * the same books and engine goes on from there.
     */
    class BookEvalData {
    public:
        enum class State : std::uint8_t {
            Stopped,      ///< Nothing running
            Starting,     ///< Scheduled, no position is being searched yet
            Running,      ///< At least one position is being searched
            Stopping,     ///< Asked to stop at once
            Gracefully    ///< Asked to stop once the running searches are back
        };

        /** @brief The kinds of limit every search is given. */
        static constexpr const char* LIMIT_DEPTH = "depth";
        static constexpr const char* LIMIT_NODES = "nodes";
        static constexpr const char* LIMIT_MOVETIME = "movetime";

        struct Config {
            std::string bookFile;               ///< The book whose positions are evaluated
            std::string outFile;                ///< The evaluated book, continued if it exists
            std::string limit = LIMIT_DEPTH;    ///< depth, nodes or movetime
            uint64_t limitValue = 9;            ///< Plies, nodes or milliseconds
            uint32_t saveIntervalS = 60;        ///< Seconds between two saves of the evaluated book
        };

        BookEvalData();
        ~BookEvalData();
        BookEvalData(const BookEvalData&) = delete;
        BookEvalData& operator=(const BookEvalData&) = delete;

        static BookEvalData& instance() {
            static BookEvalData instance;
            return instance;
        }

        /** @brief Keeps the state current and finishes a run that is over; once per frame. */
        void pollData();

        [[nodiscard]] Config& config() { return config_; }
        [[nodiscard]] const Config& config() const { return config_; }

        /** @brief The engine that searches; one engine for the whole book. */
        [[nodiscard]] ImGuiEngineSelect& getEngineSelect() { return *engineSelect_; }

        [[nodiscard]] RecentFiles& recentBookFiles() { return recentBookFiles_; }
        [[nodiscard]] RecentFiles& recentOutFiles() { return recentOutFiles_; }

        /** @brief The evaluated book offered for a book: its name with -eval before the extension. */
        [[nodiscard]] static std::string suggestedOutFile(const std::string& bookFile);

        /** @brief The time control every search is given, as the tester reads it: "depth:9". */
        [[nodiscard]] std::string timeControl() const;

        /**
         * @brief Checks what a run needs, reporting the first thing that is missing.
         * @param sendMessage If true, says through the snackbar what is missing.
         */
        [[nodiscard]] bool mayEvaluate(bool sendMessage = false) const;

        /** @brief Starts the evaluation, or continues the one in the output book. */
        void evaluate();

        /**
         * @brief Ends a running evaluation; what is evaluated is saved.
         * @param graceful If true, the running searches are waited for.
         */
        void stop(bool graceful);

        [[nodiscard]] State getState() const { return state_; }
        [[nodiscard]] bool isRunning() const { return state_ == State::Running; }
        [[nodiscard]] bool isBusy() const { return state_ != State::Stopped; }

        /** @brief Positions of the book that get a score; 0 before the first run. */
        [[nodiscard]] size_t getPositionCount() const;
        /** @brief Positions that have their score. */
        [[nodiscard]] size_t getEvaluatedCount() const;
        /** @brief Positions searched per second in this run. */
        [[nodiscard]] double getPositionsPerSecond() const;
        /** @brief True if the last run left every position with a score. */
        [[nodiscard]] bool isComplete() const;

        [[nodiscard]] uint32_t getExternalConcurrency() const;
        void setExternalConcurrency(uint32_t count);
        /** @brief Changes the number of engines while a run is going on. */
        void setPoolConcurrency(uint32_t count, bool nice = true, bool direct = false);

        /** @brief Writes the settings to the configuration. */
        void updateConfiguration() const;

    private:
        void init();
        /** @brief Saves the evaluated book and says how the run ended. */
        void finishRun();

        Config config_;
        std::unique_ptr<ImGuiEngineSelect> engineSelect_;
        std::unique_ptr<ImGuiConcurrency> imguiConcurrency_;
        RecentFiles recentBookFiles_{ "bookeval-book" };
        RecentFiles recentOutFiles_{ "bookeval-out" };
        std::shared_ptr<QaplaTester::BookEvalManager> manager_;
        GameManagerPoolAccess poolAccess_;
        State state_ = State::Stopped;
        bool wasStopped_ = false;
        size_t evaluatedAtStart_ = 0;
        std::unique_ptr<Callback::UnregisterHandle> pollCallbackHandle_;
    };

}
