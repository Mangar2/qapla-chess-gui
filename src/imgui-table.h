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

#include "imgui.h"
#include  <mutex>
#include <string>
#include <vector>

namespace QaplaWindows {

    /**
     * @brief Encapsulates an ImGui table with static configuration and dynamic row content.
     */
    class ImGuiTable {
    public:
        struct ColumnDef {
			std::string name; 
			ImGuiTableColumnFlags flags = ImGuiTableColumnFlags_None;
			float width = 0.0f; 
			bool alignRight = false; 
        };

        /**
         * @brief Constructs an ImGuiTable with static table configuration.
         * @param tableId Unique table identifier used in ImGui::BeginTable.
         * @param tableFlags ImGui table flags.
		 * @param columns Column definitions including names, flags, and widths.
         */
        ImGuiTable(const std::string& tableId,
            ImGuiTableFlags tableFlags,
            const std::vector<ColumnDef>& columns);

        /**
         * @brief Pushes a new row to the end of the table.
         * @param row List of strings representing cell content.
         */
        void push(const std::vector<std::string>& row);

        /**
         * @brief Inserts a new row at the front of the table.
         * @param row List of strings representing cell content.
         */
        void push_front(const std::vector<std::string>& row);

        /**
         * @brief Clears all dynamic row content.
         */
        void clear();

        /**
         * @brief Returns the number of rows in the table.
         * @return Number of rows.
		 */
        size_t size() const {
            return rows_.size();
		}

        /**
         * @brief Renders the table with dynamic content.
         * @param size Size of the table in ImGui units.
         */
        void draw(const ImVec2& size) const;

        std::string getField(size_t row, size_t column) const {
            if (row < rows_.size() && column < columns_.size()) {
                return rows_[row][column];
            }
            return "";
		}

        void setField(size_t row, size_t column, const std::string& value) {
            if (row < rows_.size() && column < columns_.size()) {
                rows_[row][column] = value;
            }
		}

    private:
        void tableHeadersRow() const;
        std::string tableId_;
        ImGuiTableFlags tableFlags_;
        std::vector<ColumnDef> columns_;
        std::vector<std::vector<std::string>> rows_;
    };
}
