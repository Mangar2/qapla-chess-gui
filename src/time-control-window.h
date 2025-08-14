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
        TimeSegment editTimeSegment(const TimeSegment& segment, bool blitz);

        /**
         * @brief Allows the user to select predefined time values for a TimeSegment.
         *
         * @param segment The current TimeSegment to be updated.
         * @param predefinedLabels A vector of strings representing the labels for predefined values.
         * @param predefinedMinutes A vector of integers representing the corresponding predefined minute values.
         * @return TimeSegment The updated TimeSegment with the selected predefined value.
         */
        TimeSegment selectPredefinedValues(
            const TimeSegment& segment,
            const std::vector<std::string>& predefinedLabels,
            const std::vector<int>& predefinedMinutes);

        /**
         * @brief Draws the UI for configuring Blitz time settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        TimeControl drawBlitzTime(const TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Tournament time settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        TimeControl drawTournamentTime(const TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Time per Move settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        TimeControl drawTimePerMove(const TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Fixed Depth settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        TimeControl drawFixedDepth(const TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Nodes per Move settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        TimeControl drawNodesPerMove(const TimeControl& currentTimeControl);

		static constexpr float inputIndent = 32.0f;  // Indentation for input fields
    };

} // namespace QaplaWindows
