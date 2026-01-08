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

#include "base-elements/timer.h"
#include "base-elements/change-tracker.h"
#include "game-manager/engine-record.h"

#include <imgui.h>

#include <memory>
#include <string>

namespace QaplaTester
{
    class GameRecord;
    struct MoveRecord;
}

namespace QaplaWindows {

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class ImGuiClock {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        ImGuiClock();
        ~ImGuiClock();

        void draw() const;

        /**
         * @brief Sets the clock data from the game record.
         * @param gameRecord The game record to extract data from.
         */
        void setFromGameRecord(const QaplaTester::GameRecord &gameRecord);


        /**
         * @brief Sets the remaining clock data from the move record.
         * @param moveRecord The move record to extract data from.
         * @param playerIndex The index of the player (0 for white, 1 for black).
         */
        void setFromMoveRecord(const QaplaTester::MoveRecord& moveRecord, uint32_t playerIndex);

        /**
         * @brief Stops the clock timers.
         */
        void setStopped(bool stopped) {
            if (stopped && !stopped_) {
                clockData_.wTimer.stop();
                clockData_.bTimer.stop();
            }
            stopped_ = stopped; 
        }

        void setAnalyze(bool analyze) {
            analyze_ = analyze;
        }

        /**
         * @brief Returns the current timer in milliseconds for the side to move.
         * @return Current timer in milliseconds.
         */
        uint64_t getCurrentTimerMs() const {
            return clockData_.wtm ? 
                clockData_.wTimer.elapsedMs() :
                clockData_.bTimer.elapsedMs();
        }

    private:

        /** 
         * @brief Updates the clock display state from a historical move record.
         * 
         * This private method is called internally to synchronize the clock display
         * with a specific move from the game history. It extracts timing information
         * from the specified move record and updates the internal clock state accordingly.
         * 
         * @param gameRecord The game record containing the complete move history
         *                   and game state information.
         * @param curMoveIndex The zero-based index of the move in the game history
         *                     to extract timing information from. Must be less than
         *                     the size of the history vector.
         */
        void setFromHistoryMove(const QaplaTester::MoveRecord& moveRecord);

        struct ClockData {
            std::string wEngineName;
			std::string bEngineName;
			std::uint64_t wTimeLeftMs = 0;
			std::uint64_t bTimeLeftMs = 0;
            std::uint64_t wTimeCurMove = 0;
			std::uint64_t bTimeCurMove = 0;
            QaplaHelpers::Timer wTimer;
			QaplaHelpers::Timer bTimer;
			bool wtm = true; 
		};
        ClockData clockData_;
        uint32_t nextHalfmoveNo_ = 0;
        QaplaTester::ChangeTracker gameRecordTracker_;

        std::vector<uint32_t> infoCnt_{};
        std::vector<uint32_t> displayedMoveNo_{};

        bool stopped_ = false;
        bool analyze_ = false;
    };

} // namespace QaplaWindows
