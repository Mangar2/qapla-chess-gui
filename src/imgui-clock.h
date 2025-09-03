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

    private:
        struct ClockData {
            std::string wEngineName;
			std::string bEngineName;
			std::uint64_t wTimeLeftMs;
			std::uint64_t bTimeLeftMs;
            std::uint64_t wTimeCurMove;
			std::uint64_t bTimeCurMove;
			bool wtm = true; 
		};
        ClockData clockData_;
        uint32_t curHalfmoveNo_ = 0;
    };

} // namespace QaplaWindows
