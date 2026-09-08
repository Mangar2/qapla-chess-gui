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

#include "clock-model.h"

#include <imgui.h>

#include <cstdint>

namespace QaplaTester
{
    class GameRecord;
    struct MoveRecord;
}

namespace QaplaWindows {

    /**
     * @brief Draws the chess clocks of a game.
     *
     * The window owns no display logic of its own: what is shown is decided by ClockModel,
     * this class only lays the values out and paints them. See clock-model.h.
     */
    class ImGuiClock {
    public:
        ImGuiClock();
        ~ImGuiClock();

        void draw() const;

        /**
         * @brief Sets the clock data from the game record.
         * @param gameRecord The game record to extract data from.
         */
        void setFromGameRecord(const QaplaTester::GameRecord& gameRecord) {
            model_.setFromGameRecord(gameRecord);
        }

        /**
         * @brief Sets the remaining clock data from the move record.
         * @param moveRecord The move record to extract data from.
         * @param playerIndex The index of the player (0 for white, 1 for black).
         */
        void setFromMoveRecord(const QaplaTester::MoveRecord& moveRecord, uint32_t playerIndex) {
            model_.setFromMoveRecord(moveRecord, playerIndex);
        }

        /**
         * @brief Stops the clock timers.
         */
        void setStopped(bool stopped) {
            model_.setStopped(stopped);
        }

        void setAnalyze(bool analyze) {
            model_.setAnalyze(analyze);
        }

        /**
         * @brief Returns the current timer in milliseconds for the side to move.
         * @return Current timer in milliseconds.
         */
        [[nodiscard]] uint64_t getCurrentTimerMs() const {
            return model_.currentTimerMs();
        }

    private:
        ClockModel model_;
    };

} // namespace QaplaWindows
