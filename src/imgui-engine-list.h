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

#include "qapla-tester/engine-record.h"

#include <imgui.h>
#include <memory>
#include <vector>
#include <string>

struct MoveRecord;
struct SearchInfo;

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class ImGuiEngineList  {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        ImGuiEngineList();
        ImGuiEngineList(ImGuiEngineList&&) noexcept;
        ImGuiEngineList& operator=(ImGuiEngineList&&) noexcept;
        ~ImGuiEngineList();

        void draw();

        /**
         * @brief Sets whether user input is allowed in the engine list.
         * @param allow True to allow input, false to disallow.
         */
        void setAllowInput(bool allow) { allowInput_ = allow; }


    private:
        void addTables(size_t size);

        /**
         * @brief Draws the engine space for a given index.
         * @param index Index of the engine to draw.
		 * @param size Size of the engine space.
		 */ 
        void drawEngineSpace(size_t index, const ImVec2 size);

        
        void setTable(size_t index);
        void setTable(size_t index, const MoveRecord& moveRecord);
        std::string drawButtons(size_t index);
        
        std::vector<std::unique_ptr<ImGuiTable>> tables_;
        std::vector<uint32_t> displayedMoveNo_;
        std::vector<uint32_t> infoCnt_;
   
        EngineRecords engineRecords_;
        MoveInfos moveInfos_;

        bool allowInput_ = false;
    };

} // namespace QaplaWindows
