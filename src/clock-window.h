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
 * @author Volker B�hm
 * @copyright Copyright (c) 2025 Volker B�hm
 */

#pragma once

#include "embedded-window.h"
#include "board-data.h"
#include <memory>


namespace QaplaWindows {

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class ClockWindow : public EmbeddedWindow {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        ClockWindow();
        ~ClockWindow();

        void draw() override;

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
		bool setClockData();
    };

} // namespace QaplaWindows
