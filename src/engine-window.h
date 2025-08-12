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
struct SearchInfo;

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class EngineWindow : public EmbeddedWindow {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        EngineWindow(std::shared_ptr<BoardData> boardData);
        ~EngineWindow();

        void draw() override;

    private:
        void addTables(size_t size);
        std::vector<std::unique_ptr<ImGuiTable>> tables_;
        std::vector<uint32_t> displayedMoveNo_;
		void setTable(size_t index);
        void setTable(size_t index, const MoveRecord& moveRecord);

        void drawButtons(uint32_t index);

        std::shared_ptr<BoardData> boardData_;
    };

} // namespace QaplaWindows
