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

#include "imgui-table.h"
#include <imgui.h>
#include <iostream>

namespace QaplaWindows {

    ImGuiTable::ImGuiTable(const std::string& tableId,
        ImGuiTableFlags tableFlags,
        const std::vector<ColumnDef>& columns)
        : tableId_(tableId),
        tableFlags_(tableFlags),
        columns_(columns) {
    }

    ImGuiTable::ImGuiTable() = default;
    ImGuiTable::ImGuiTable(ImGuiTable&&) noexcept = default;
    ImGuiTable& ImGuiTable::operator=(ImGuiTable&&) noexcept = default;
    ImGuiTable::~ImGuiTable() = default;


    static void alignRight(const std::string& content) {
        float colWidth = ImGui::GetColumnWidth();
        float textWidth = ImGui::CalcTextSize(content.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + colWidth - textWidth - 10);
	}

    static void textAligned(const std::string& content, bool right) {
        if (right) {
			alignRight(content);
        }
        ImGui::TextUnformatted(content.c_str());
    }

    static void headerAligned(const std::string& content, bool right) {
        if (right) {
            alignRight(content);
        }
        ImGui::TableHeader(content.c_str());
    }

    void ImGuiTable::push(const std::vector<std::string>& row) {
        rows_.push_back(row);
    }

    void ImGuiTable::push_front(const std::vector<std::string>& row) {
        rows_.insert(rows_.begin(), row);
    }

    void ImGuiTable::clear() {
        rows_.clear();
    }

    void ImGuiTable::pop_back() {
        if (!rows_.empty()) {
            rows_.pop_back();
        }
    }

    void ImGuiTable::tableHeadersRow() const {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

        for (int columnN = 0; columnN < columns_.size(); columnN++) {
            if (!ImGui::TableSetColumnIndex(columnN))
                continue;
			std::string name = columns_[columnN].name;
            ImGui::PushID(columnN);
            headerAligned(columns_[columnN].name, columns_[columnN].alignRight);
            ImGui::PopID();
        }
    }

    bool ImGuiTable::isRowClicked(size_t index) const {
        if (!clickable_) return false;
        std::string id = "row" + std::to_string(index);
        ImGui::PushID(id.c_str());
        bool clicked = ImGui::Selectable("##row", false,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);
        ImGui::PopID();
        ImGui::SameLine(0.0f, 0.0f);
        return clicked;
    }

    void ImGuiTable::accentuateCurrentRow(size_t rowIndex) const {
        if (!currentRow_.has_value() || currentRow_.value() != rowIndex) {
            return;
        }
        auto baseColor = ImGui::GetStyleColorVec4(ImGuiCol_TabDimmedSelected);
        auto baseColor32 = ImGui::ColorConvertFloat4ToU32(baseColor);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, baseColor32);
    }

    void ImGuiTable::drawRow(size_t rowIndex) const {
        const auto& row = rows_[rowIndex];
        for (size_t col = 0; col < columns_.size() && col < row.size(); ++col) {
            ImGui::TableSetColumnIndex(static_cast<int>(col));
            if (columns_[col].customRender) {
                bool alignRight = columns_[col].alignRight;
                std::string content = row[col];
                columns_[col].customRender(content, alignRight);
                textAligned(content, alignRight);
            } else {
                textAligned(row[col], columns_[col].alignRight);
            }
        }
    }

    static std::optional<float> calculateOptimalScrollPosition(size_t rowIndex) {
        float scrollY = ImGui::GetScrollY();
        float windowHeight = ImGui::GetWindowHeight();
        float rowHeight = ImGui::GetTextLineHeightWithSpacing();
        
        float rowTop = (rowIndex + 1) * rowHeight; // +1 for header row
        float rowBottom = rowTop + rowHeight;
        float visibleTop = scrollY + rowHeight; // +rowHeight to account for header
        float visibleBottom = scrollY + windowHeight;
        
        // Row visible, no scroll needed
        if (rowTop >= visibleTop && rowBottom <= visibleBottom) {
            return std::nullopt;  
        }

        // Row far away → no scroll
        if (rowBottom + 1.0f < visibleTop || rowTop - 1.0f > visibleBottom) {
            return 0.5f;  
        }
        
        // Row, too far up (but close) → scroll to top
        return (rowBottom < visibleBottom) ? 0.0f : 1.0f;
    }

    std::optional<size_t> ImGuiTable::draw(const ImVec2& size, bool shrink) const {
        std::optional<size_t> clickedRow;
        ImVec2 tableSize = size;
        if (shrink) {
            float rowHeight = ImGui::GetTextLineHeightWithSpacing();
            tableSize.y = std::min(tableSize.y, (rows_.size() + 2) * rowHeight);
        }
        if (ImGui::BeginTable(tableId_.c_str(), static_cast<int>(columns_.size()), tableFlags_, tableSize)) {
            for (const auto& column : columns_) {
                ImGui::TableSetupColumn(column.name.c_str(), column.flags, column.width);
            }
            tableHeadersRow();
            for (size_t rowIndex = 0; rowIndex < rows_.size(); ++rowIndex) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (isRowClicked(rowIndex)) {
                    clickedRow = rowIndex;
                }
                accentuateCurrentRow(rowIndex);
                // Auto-scroll to row if requested
                if (scrollToRow_ && *scrollToRow_ == rowIndex) {
                    auto scrollPos = calculateOptimalScrollPosition(rowIndex);
                    if (scrollPos) {
                        std::cout << "Scrolling to row " << rowIndex << " pos=" << *scrollPos << std::endl;
                        ImGui::SetScrollHereY(*scrollPos);
                    }
                    scrollToRow_.reset(); 
                }
                drawRow(rowIndex);
            }

            ImGui::EndTable();
        }
        
        // Handle keyboard navigation 
        auto keyboardRow = checkKeyboard();

        return (keyboardRow) ? *keyboardRow : clickedRow;
    }

    std::optional<size_t> ImGuiTable::checkKeyboard() const {
        if (!clickable_ || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            return std::nullopt;
        }
        
        int currentFrame = ImGui::GetFrameCount();
        if (currentFrame == lastInputFrame_) {
            return std::nullopt;
        }
        lastInputFrame_ = currentFrame;
       
        if (currentRow_ && ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
            if (*currentRow_ > 0) {
                return *currentRow_ - 1;  // Navigate to previous row
            } else if (allowNavigateToZero_) {
                return SIZE_MAX;  // Special value indicating "navigate to zero"
            }
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
            if (!currentRow_) {
                return 0;  // Navigate to first row from "zero" position
            } else if (*currentRow_ + 1 < rows_.size()) {
                return *currentRow_ + 1;  // Navigate to next row
            }
        }
        
        return std::nullopt;
    }

}
