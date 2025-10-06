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

#include "embedded-window.h"
#include "qapla-tester/time-control.h"

#include <memory>

namespace QaplaWindows {


    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class TimeControlWindow : public EmbeddedWindow {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param BoardData full set of information about the current chess Game
         */
        TimeControlWindow();

        void draw() override;

    private:

        /**
         * @brief Edits a time segment by providing input fields for its configuration.
         *
         * @param segment The current time segment to be edited.
         * @param blitz If true, limits input to minutes and seconds for the base time and seconds/milliseconds for the increment.
         * @return TimeSegment The updated time segment with the new configuration.
         */
        static TimeSegment editTimeSegment(const TimeSegment& segment, bool blitz);



        /**
         * @brief Draws the UI for configuring Blitz time settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        static TimeControl drawBlitzTime(const TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Tournament time settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        static TimeControl drawTournamentTime(const TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Time per Move settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        static TimeControl drawTimePerMove(const TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Fixed Depth settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        static TimeControl drawFixedDepth(const TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Nodes per Move settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        static TimeControl drawNodesPerMove(const TimeControl& currentTimeControl);

    };

} // namespace QaplaWindows
