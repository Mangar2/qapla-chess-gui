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

#include "imgui-table.h"

#include "qapla-tester/change-tracker.h"

#include <imgui.h>

#include <memory>
#include <optional>
#include <vector>
#include <string>

struct MoveRecord;
class GameRecord;

namespace QaplaWindows {


    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class ImGuiMoveList {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param BoardData full set of information about the current chess Game
         */
        ImGuiMoveList();

        /**
         * @brief Draws the move list window.
         * @return The index of the move that was clicked, if any.
         */
        std::optional<size_t> draw();

        /**
         * @brief sets the table content from a GameRecord.
         * @param gameRecord the GameRecord to extract the move list from.
         */
        void setFromGameRecord(const GameRecord& gameRecord);

        /**
         * @brief Sets whether the table rows are clickable.
         * @param clickable If true, rows can be clicked to select moves.
         */
        void setClickable(bool clickable) {
            clickable_ = clickable;
            table_.setClickable(clickable);
            table_.setAllowNavigateToZero(true);  // Chess-specific: allow navigate to start position
        }

    private:
        std::vector<std::string> mkRow(const std::string& label, const MoveRecord& move, size_t index);
		size_t currentPly_ = 0;
        bool clickable_ = false;
        ChangeTracker referenceTracker_;
        ImGuiTable table_;
    };

} // namespace QaplaWindows
