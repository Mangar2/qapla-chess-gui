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
#include "board-data.h"

#include <memory>

struct MoveRecord;

namespace QaplaWindows {


    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class MoveListWindow : public EmbeddedWindow {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param BoardData full set of information about the current chess Game
         */
        MoveListWindow(std::shared_ptr<BoardData> BoardData);

        void draw() override;

    private:
        void renderMoveLine(const std::string& label, const MoveRecord& move, uint32_t index);
        void checkKeyboard();
        /**
         * Returns true if the table row with the given index is currently clicked.
         * Must be called during draw(), at the point where the row with this index is being rendered.
         */
        bool isRowClicked(size_t index);
        std::shared_ptr<BoardData> BoardData_;
		uint32_t currentPly_ = 0;
        int lastInputFrame_ = -1;
    };

} // namespace QaplaWindows
