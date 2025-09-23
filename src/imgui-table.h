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

#include <imgui.h>
#include <mutex>
#include <string>
#include <vector>
#include <optional>
#include <functional>

#include "table-index-manager.h"

namespace QaplaWindows {

    /**
     * @brief Encapsulates an ImGui table with static configuration and dynamic row content.
     */
    class ImGuiTable {
    public:
        ImGuiTable();
        ImGuiTable(ImGuiTable&&) noexcept;
        ImGuiTable& operator=(ImGuiTable&&) noexcept;
        ~ImGuiTable();

        struct ColumnDef {
			std::string name; 
			ImGuiTableColumnFlags flags = ImGuiTableColumnFlags_None;
			float width = 0.0f; 
			bool alignRight = false; 
            std::function<void(std::string&, bool&)> customRender = nullptr;
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
         * @brief Sets whether the table rows are clickable.
         * @param clickable If true, rows can be clicked to select a row.
         */
        void setClickable(bool clickable) {
			clickable_ = clickable;
		}

        /**
         * @brief Sets whether the table should auto-scroll to the current row.
         * @param autoScroll If true, table will scroll to current row automatically.
         */
        void setAutoScroll(bool autoScroll) {
            autoScroll_ = autoScroll;
        }

        /**
         * @brief Sets whether keyboard navigation can navigate to "row -1" (representing index 0).
         * @param allow If true, Up arrow from first row will return index 0 instead of staying at row 0.
         */
        void setAllowNavigateToZero(bool allow) {
            allowNavigateToZero_ = allow;
        }

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
         * @brief Returns the number of rows in the table.
         * @return Number of rows.
         */
        size_t size() const {
            return rows_.size();
        }

        /**
         * @brief Clears all dynamic row content.
         */
        void clear();

        /**
         * @brief Removes the last row from the table.
         */
        void pop_back();

        /**
         * @brief Removes the first row from the table.
         */
        void pop_front() {
            if (!rows_.empty()) {
                rows_.erase(rows_.begin());
                needsSort_ = true;
            }
        }

        /**
         * @brief Renders the table with dynamic content.
         * @param size Size of the table in ImGui units.
         * @param shrink If true, the table shrinks dynamically in height.
         * @return Number of row clicked, if any
         */
        std::optional<size_t> draw(const ImVec2& size, bool shrink = false);

        /**
         * @brief Returns the content of a specific cell.
         * @param row Row index.
         * @param column Column index.
         * @return Cell content string, or empty string if out of bounds.
         */
        std::string getField(size_t row, size_t column) const {
            if (row < rows_.size() && column < columns_.size()) {
                return rows_[row][column];
            }
            return "";
		}

        /**
         * @brief Sets the content of a specific cell.
         * @param row Row index.
         * @param column Column index.
         * @param value New cell content string.
         */
        void setField(size_t row, size_t column, const std::string& value) {
            if (row < rows_.size() && column < columns_.size()) {
                rows_[row][column] = value;
                needsSort_ = true;
            }
        }        /**
		 * @brief Adds a new column to a specific row.
		 * @param row Index of the row to extend.
		 * @param col New column content to add.
		 */
        void extend(size_t row, const std::string& col) {
            if (row < rows_.size()) {
                rows_[row].push_back(col);
                needsSort_ = true;
            }
        }

        /**
         * @brief Sets the column header definition for a specific column.
         * @param col Index of the column to set.
         * @param column Column definition including name, flags, and width.
		 */
        void setColumnHead(size_t col, const ColumnDef& column) {
            if (col >= columns_.size()) {
                columns_.resize(col + 1);
			}
            if (col < columns_.size()) {
                columns_[col] = column;
			}
		}

        /**
		 * @brief Returns the currently selected row index.
         */
        size_t getSelectedRow() {
            return indexManager_.getCurrentRow().value_or(0);
        }

        /**
         * @brief Sets the current row index. The current row is highlighted and kept in view.
         * @param row The index of the row to set as current, or std::nullopt if the table will not show any current row.
         */
        void setCurrentRow(std::optional<size_t> row) {
            if (autoScroll_ && row.has_value()) {
                scrollToRow_ = row; 
            }
            indexManager_.setCurrentRow(row.value_or(0));
        }

    private:
        void accentuateCurrentRow(size_t rowIndex) const;
        void drawRow(size_t rowIndex) const;

        /**
         * @brief Checks for keyboard input and returns the index of the row to focus.
         * 
         * Note it is important that this function returns std::nullopt if no key was pressed.
         * Returning an index causes the selection of a table row, and thus maybe loading a
         * new Game.
         * 
         * @param visibleRows 
         * @return std::optional<size_t> index to focus IF a key was pressed
         */
        std::optional<size_t> checkKeyboard(size_t visibleRows);
        void setupTable() const;
        void handleSorting();
        void tableHeadersRow() const;
        size_t getCurrentSortedIndex() const;

        bool clickable_ = false;
        bool autoScroll_ = false;
        bool allowNavigateToZero_ = false;
        std::optional<size_t> scrollToRow_;
        int lastInputFrame_ = -1;
        bool isRowClicked(size_t index) const;
        std::string tableId_;
        ImGuiTableFlags tableFlags_;
        std::vector<ColumnDef> columns_;
        std::vector<std::vector<std::string>> rows_;
        bool needsSort_ = true;
        ImGuiTableSortSpecs* sortSpecs_ = nullptr;
        TableIndexManager indexManager_;
    };
}
