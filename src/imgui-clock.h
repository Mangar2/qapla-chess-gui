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

#include "qapla-tester/timer.h"
#include "qapla-tester/change-tracker.h"

#include <imgui.h>

#include <memory>
#include <string>

class GameRecord;
class MoveRecord;

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

        void draw();

        /**
         * @brief Sets the clock data from the game record.
         * @param gameRecord The game record to extract data from.
         */
        void setFromGameRecord(const GameRecord& gameRecord);

        /**
         * @brief Sets the remaining clock data from the move record.
         * @param moveRecord The move record to extract data from.
         * @param playerIndex The index of the player (0 for white, 1 for black).
         */
        void setFromMoveRecord(const MoveRecord& moveRecord, uint32_t playerIndex);

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
        struct ClockData {
            std::string wEngineName;
			std::string bEngineName;
			std::uint64_t wTimeLeftMs = 0;
			std::uint64_t bTimeLeftMs = 0;
            std::uint64_t wTimeCurMove = 0;
			std::uint64_t bTimeCurMove = 0;
            Timer wTimer;
			Timer bTimer;
			bool wtm = true; 
		};
        ClockData clockData_;
        uint32_t curHalfmoveNo_ = 0;
        ChangeTracker gameRecordTracker_;

        std::vector<uint32_t> infoCnt_{};
        std::vector<uint32_t> displayedMoveNo_{};

        bool stopped_ = false;
    };

} // namespace QaplaWindows
