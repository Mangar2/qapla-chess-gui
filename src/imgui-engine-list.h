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
        virtual ~ImGuiEngineList();

        /**
         * @brief Renders the engine window and its components.
         * This method should be called within the main GUI rendering loop.
         * @return A pair containing the engine ID and command string if an action was triggered, otherwise empty strings.
         */
        virtual std::pair<std::string, std::string> draw();

        /**
         * @brief Sets whether user input is allowed in the engine list.
         * @param allow True to allow input, false to disallow.
         */
        void setAllowInput(bool allow) { 
            allowInput_ = allow; 
        }

        /**
         * @brief Sets the engine records for the list.
         * @param engineRecords The engine records to display.
         */
        void setEngineRecords(const EngineRecords& engineRecords) {
            engineRecords_ = engineRecords;
        }   

        /**
         * @brief Gets the current engine records.
         * @return The current engine records.
         */
        const EngineRecords& getEngineRecords() const {
            return engineRecords_;
        }

        /**
         * @brief Sets the move record for the list.
         * @param moveRecord The move record to display.
         * @param playerIndex The index of the player (0 or 1).
         */
        void setFromMoveRecord(const MoveRecord& moveRecord, uint32_t playerIndex) {
            addTables(playerIndex + 1);
            setTable(playerIndex, moveRecord);
        }

    private:
        void addTables(size_t size);

        /**
         * @brief Draws the engine space for a given index.
         * @param index Index of the engine to draw.
		 * @param size Size of the engine space.
         * @return String representing any action to perform (e.g., "stop", "restart") or empty if no action.
		 */
        std::string drawEngineSpace(size_t index, const ImVec2 size);

        std::string drawEngineArea(const ImVec2 &topLeft, ImDrawList *drawList, 
            const ImVec2 &max, float cEngineInfoWidth, size_t index, bool isSmall);

        void drawEngineTable(const ImVec2 &topLeft, float cEngineInfoWidth, 
            float cSectionSpacing, size_t index, const ImVec2 &max, const ImVec2 &size);

        void setTable(size_t index, const MoveRecord& moveRecord);

        std::vector<std::string> mkTableLine(ImGuiTable* table, const SearchInfo& info);

        std::string drawButtons(size_t index);
        
        std::vector<std::unique_ptr<ImGuiTable>> tables_;
        std::vector<uint32_t> displayedMoveNo_;
        std::vector<uint32_t> infoCnt_;
   
        EngineRecords engineRecords_;

        bool allowInput_ = false;
    };

} // namespace QaplaWindows
