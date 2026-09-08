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

#include <base-elements/change-tracker.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace QaplaTester
{
    class GameRecord;
    struct MoveRecord;
}

namespace QaplaWindows {

    /**
     * @brief A stopwatch that never reads a clock on its own.
     *
     * Every operation receives the current time in milliseconds from the caller, so the
     * owning class controls the single point where real time enters the system.
     */
    class ClockStopwatch {
    public:
        void start(uint64_t nowMs) {
            startMs_ = nowMs;
            running_ = true;
        }

        void stop(uint64_t nowMs) {
            if (!running_) {
                return;
            }
            running_ = false;
            endMs_ = nowMs;
        }

        void reset() {
            running_ = false;
            startMs_ = 0;
            endMs_ = 0;
        }

        [[nodiscard]] bool isRunning() const { return running_; }

        [[nodiscard]] uint64_t elapsedMs(uint64_t nowMs) const {
            return running_ ? nowMs - startMs_ : endMs_ - startMs_;
        }

    private:
        bool running_ = false;
        uint64_t startMs_ = 0;
        uint64_t endMs_ = 0;
    };

    /**
     * @brief Everything the clock displays at one point in time.
     *
     * The formatted strings are what the view paints; the raw values are provided so that
     * tests and callers can reason about the numbers without parsing text.
     */
    struct ClockView {
        std::string whiteTotal;      ///< Formatted remaining time for white ("MM:SS" / "H:MM:SS").
        std::string whiteMove;       ///< Formatted time spent on white's current move.
        std::string blackTotal;      ///< Formatted remaining time for black.
        std::string blackMove;       ///< Formatted time spent on black's current move.
        std::string whiteEngineName; ///< Name shown below white's clock.
        std::string blackEngineName; ///< Name shown below black's clock.
        bool whiteToMove = true;     ///< Side whose king icon is shown and whose time runs.
        bool analyze = false;        ///< Analyze mode: the total shows the elapsed search time.

        uint64_t whiteLeftMs = 0;    ///< Remaining time for white before the running move is deducted.
        uint64_t whiteCurMoveMs = 0; ///< Time spent on white's current move, including the running stopwatch.
        uint64_t blackLeftMs = 0;    ///< Remaining time for black before the running move is deducted.
        uint64_t blackCurMoveMs = 0; ///< Time spent on black's current move, including the running stopwatch.
    };

    /**
     * @brief Decides what the chess clock shows, without any drawing.
     *
     * The class is purely reactive: it holds the complete display state and updates it from
     * the events the GUI feeds in (a new game record, a search info for one player, the
     * stopped and analyze flags). Real time enters only through the clock callback handed to
     * the constructor, so a test can substitute a fake clock and drive time deterministically.
     */
    class ClockModel {
    public:
        /** @brief Returns the current time in milliseconds. */
        using NowFn = std::function<uint64_t()>;

        /** @brief Creates a model driven by the steady system clock. */
        ClockModel();

        /**
         * @brief Creates a model driven by the given clock.
         * @param now Callback returning the current time in milliseconds. Must not be empty.
         */
        explicit ClockModel(NowFn now);

        /**
         * @brief Takes over the clock state of a game record.
         *
         * Does nothing unless the record changed since the last call, and nothing if the
         * record carries no valid time control for both sides.
         *
         * @param gameRecord The game record to extract time control and history from.
         */
        void setFromGameRecord(const QaplaTester::GameRecord& gameRecord);

        /**
         * @brief Takes over the time spent so far from a search info of one player.
         *
         * Ignored while stopped, for ponder moves, for a move record that does not belong to
         * the half-move the model waits for, and for a repeated info of an already shown move.
         *
         * @param moveRecord The move record currently being computed.
         * @param playerIndex The index of the player (0 for white, 1 for black).
         */
        void setFromMoveRecord(const QaplaTester::MoveRecord& moveRecord, uint32_t playerIndex);

        /**
         * @brief Stops or resumes the running time.
         * @param stopped True freezes both stopwatches at their current value.
         */
        void setStopped(bool stopped);

        /**
         * @brief Switches between game display and analyze display.
         * @param analyze True shows the elapsed search time instead of a remaining time.
         */
        void setAnalyze(bool analyze);

        [[nodiscard]] bool isStopped() const { return stopped_; }
        [[nodiscard]] bool isAnalyze() const { return analyze_; }

        /**
         * @brief Returns everything to display, evaluated at the current time.
         */
        [[nodiscard]] ClockView view() const;

        /**
         * @brief Returns the running time of the side to move in milliseconds.
         *
         * Used to stamp the time onto a move entered by a human player.
         */
        [[nodiscard]] uint64_t currentTimerMs() const;

    private:
        /**
         * @brief Applies the last played move of the history to the display state.
         * @param moveRecord The move preceding the position the model now shows.
         */
        void setFromHistoryMove(const QaplaTester::MoveRecord& moveRecord);

        NowFn now_;

        std::string whiteEngineName_;
        std::string blackEngineName_;
        uint64_t whiteTimeLeftMs_ = 0;
        uint64_t blackTimeLeftMs_ = 0;
        uint64_t whiteTimeCurMoveMs_ = 0;
        uint64_t blackTimeCurMoveMs_ = 0;
        ClockStopwatch whiteStopwatch_;
        ClockStopwatch blackStopwatch_;
        bool whiteToMove_ = true;

        uint32_t nextHalfmoveNo_ = 0;
        QaplaTester::ChangeTracker gameRecordTracker_;

        std::vector<uint32_t> infoCnt_{};
        std::vector<uint32_t> displayedMoveNo_{};

        bool stopped_ = false;
        bool analyze_ = false;
    };

} // namespace QaplaWindows
