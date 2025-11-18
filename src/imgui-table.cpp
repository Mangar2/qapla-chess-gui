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
#include "font.h"
#include <imgui.h>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace QaplaWindows {

    /**
     * @brief Natural sort comparison for strings containing numbers.
     * Compares strings in a human-friendly way: "1" < "2" < "10" instead of "1" < "10" < "2".
     * @param a First string to compare
     * @param b Second string to compare
     * @return true if a should come before b in natural sort order
     */
    static bool naturalCompare(const std::string& a, const std::string& b) {
        size_t i = 0, j = 0;
        
        while (i < a.size() && j < b.size()) {
            if (std::isdigit(static_cast<unsigned char>(a[i])) && 
                std::isdigit(static_cast<unsigned char>(b[j]))) {
                
                // Extract the full number from both strings
                size_t numStartA = i;
                size_t numStartB = j;
                
                while (i < a.size() && a[i] == '0') ++i;
                while (j < b.size() && b[j] == '0') ++j;
                
                // Find the end of the number
                size_t numEndA = i;
                size_t numEndB = j;
                while (numEndA < a.size() && std::isdigit(static_cast<unsigned char>(a[numEndA]))) ++numEndA;
                while (numEndB < b.size() && std::isdigit(static_cast<unsigned char>(b[numEndB]))) ++numEndB;
                
                // Compare by length first (longer number = larger)
                size_t lenA = numEndA - i;
                size_t lenB = numEndB - j;
                
                if (lenA != lenB) {
                    return lenA < lenB;
                }
                
                // Same length: compare digit by digit
                while (i < numEndA && j < numEndB) {
                    if (a[i] != b[j]) {
                        return a[i] < b[j];
                    }
                    ++i;
                    ++j;
                }
                
                // If numbers are equal, compare leading zeros count
                if (i == numEndA && j == numEndB) {
                    size_t zerosA = numStartA;
                    size_t zerosB = numStartB;
                    while (zerosA < a.size() && a[zerosA] == '0') ++zerosA;
                    while (zerosB < b.size() && b[zerosB] == '0') ++zerosB;
                    size_t zeroCountA = zerosA - numStartA;
                    size_t zeroCountB = zerosB - numStartB;
                    if (zeroCountA != zeroCountB) {
                        return zeroCountA < zeroCountB;
                    }
                }
            } else {
                // Non-digit characters: compare directly
                if (a[i] != b[j]) {
                    return a[i] < b[j];
                }
                ++i;
                ++j;
            }
        }
        
        // One string is a prefix of the other
        return a.size() < b.size();
    }

    ImGuiTable::ImGuiTable(const std::string& tableId,
        ImGuiTableFlags tableFlags,
        const std::vector<ColumnDef>& columns)
        : tableId_(tableId),
        tableFlags_(tableFlags),
        columns_(columns),
        indexManager_(TableIndex::Unsorted) {
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
            // ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
			alignRight(content);
            ImGui::TextUnformatted(content.c_str());
            // ImGui::PopFont();
        } else {
            ImGui::TextUnformatted(content.c_str());
        }
    }

    static void headerAligned(const std::string& content, bool right) {
        if (right) {
            alignRight(content);
        }
        ImGui::TableHeader(content.c_str());
    }

    void ImGuiTable::updated() {
        needsSort_ = true;
        indexManager_.updateSize(rows_.size());
    }

    void ImGuiTable::push(const std::vector<std::string>& row) {
        rows_.push_back(row);
        updated();
    }

    void ImGuiTable::push_front(const std::vector<std::string>& row) {
        rows_.insert(rows_.begin(), row);
        updated();
    }

    void ImGuiTable::clear() {
        rows_.clear();
        updated();
    }

    void ImGuiTable::pop_back() {
        if (!rows_.empty()) {
            rows_.pop_back();
            updated();
        }
    }

    void ImGuiTable::setupTable() const {
        ImGui::TableSetupScrollFreeze(0, 1);
        for (size_t i = 0; i < columns_.size(); ++i) {
            ImGui::TableSetupColumn(columns_[i].name.c_str(), columns_[i].flags, columns_[i].width, i);
        }
    }

    void ImGuiTable::tableHeadersRow() const {
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

        for (size_t columnN = 0; columnN < columns_.size(); columnN++) {
            if (!ImGui::TableSetColumnIndex(columnN))
                continue;
			std::string name = columns_[columnN].name;
            ImGui::PushID(static_cast<int>(columnN));
            headerAligned(columns_[columnN].name, columns_[columnN].alignRight);
            ImGui::PopID();
        }
    }

    static std::optional<float> calculateOptimalScrollPosition(size_t rowIndex) {
        float scrollY = ImGui::GetScrollY();
        float windowHeight = ImGui::GetWindowHeight();
        if (ImGui::GetScrollMaxX() > 0) {
            windowHeight -= ImGui::GetStyle().ScrollbarSize;
        }
        float rowHeight = ImGui::GetTextLineHeightWithSpacing();
        
        float rowTop = (rowIndex + 1) * rowHeight; // +1 for header row
        float rowBottom = rowTop + rowHeight;
        float visibleTop = scrollY + rowHeight; 
        float visibleBottom = scrollY + windowHeight;
        
        // Row visible, no scroll needed
        if (rowTop >= visibleTop && rowBottom <= visibleBottom) {
            return std::nullopt;  
        }

        // Row far away → scroll to center
        if (rowBottom + windowHeight < visibleTop || rowTop - windowHeight > visibleBottom) {
            return rowTop - (windowHeight * 0.5f) + (rowHeight * 0.5f);
        }
        
        // Row too far up (but close) → scroll to top
        if (rowBottom < visibleBottom) {
            return rowTop - rowHeight;
        }

        // Row too far down → scroll to bottom
        return rowBottom - windowHeight;
    }

    bool ImGuiTable::isRowClicked(size_t index) const {
        if (!clickable_) return false;
        std::string id = "row" + std::to_string(index);
        ImGui::PushID(id.c_str());
        bool clicked = ImGui::Selectable("##row", false,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);
        ImGui::PopID();
        ImGui::SameLine(0.0F, 0.0F);
        return clicked;
    }

    void ImGuiTable::accentuateCurrentRow(size_t rowIndex) const {
        auto current = indexManager_.getCurrentRow();
        if (!current || *current != rowIndex) {
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

    std::optional<size_t> ImGuiTable::draw(const ImVec2& size, bool shrink) {
        std::optional<size_t> clickedRow;
        ImVec2 tableSize = size;
        float rowHeight = ImGui::GetTextLineHeightWithSpacing();

        std::optional<size_t> keyboardRow;
        indexManager_.updateSize(rows_.size());
        if (filterable_) {
            float cursorYBefore = ImGui::GetCursorPosY();
            auto changed = filter_.draw();
            float cursorYAfter = ImGui::GetCursorPosY();
            float filterHeight = cursorYAfter - cursorYBefore;
            tableSize.y = std::max(0.0F, tableSize.y - filterHeight);
            handleFiltering(changed);
        }

        if (shrink) {
            tableSize.y = std::min(tableSize.y, (indexManager_.size() + 2) * rowHeight);
        }

        // Calculate visible rows
        size_t visibleRows = static_cast<size_t>(tableSize.y / rowHeight);
        if (visibleRows == 0) visibleRows = 1; // Fallback

        // Push the selected font if not using default system font
        ImFont* selectedFont = nullptr;
        if (fontIndex_ == FontManager::interVariableIndex && FontManager::interVariable != nullptr) {
            selectedFont = FontManager::interVariable;
        } else if (fontIndex_ == FontManager::ibmPlexMonoIndex && FontManager::ibmPlexMono != nullptr) {
            selectedFont = FontManager::ibmPlexMono;
        }
        
        if (selectedFont != nullptr) {
            ImGui::PushFont(selectedFont);
        }

        if (ImGui::BeginTable(tableId_.c_str(), static_cast<int>(columns_.size()), tableFlags_, tableSize)) {
            setupTable();
            handleSorting();
            tableHeadersRow();
            handleScrolling();
            handleClipping(clickedRow);
            keyboardRow = checkKeyboard(visibleRows);
            ImGui::EndTable();
        }

        // Pop the font if we pushed one
        if (selectedFont != nullptr) {
            ImGui::PopFont();
        }
        
        return (keyboardRow) ? *keyboardRow : clickedRow;
    }

    void ImGuiTable::handleClipping(std::optional<size_t> &clickedRow)
    {
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(indexManager_.size()));
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                size_t actualRow = indexManager_.getRowNumber(i);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (isRowClicked(actualRow))
                {
                    clickedRow = actualRow;
                }
                accentuateCurrentRow(actualRow);
                drawRow(actualRow);
            }
        }
    }

    void ImGuiTable::handleScrolling()
    {
        if (scrollToRow_)
        {
            // Find the sorted index for the row to scroll to
            auto sortedIndex = indexManager_.getRowIndex(*scrollToRow_);
            if (sortedIndex)
            {
                auto scrollPos = calculateOptimalScrollPosition(*sortedIndex);
                if (scrollPos)
                {
                    ImGui::SetScrollY(*scrollPos);
                }
            }
            scrollToRow_.reset();
        }
    }

    void ImGuiTable::handleFiltering(bool changed) {
        if (!filterable_ || !changed) return;
        needsSort_ = true;
        indexManager_.filter([&](size_t rowIndex) {
            const auto& row = rows_[rowIndex];
            if (filter_.matches(row)) {
                return true;
            }
            return false;
        });
    }
    
    void ImGuiTable::handleSorting() {
        ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs();
        if (!specs) return;
        if (needsSort_ || (specs && specs->SpecsDirty)) {
            needsSort_ = false;
            if (specs->SpecsCount > 0) {
                auto spec = specs->Specs[0];
                int column = spec.ColumnUserID;
                bool ascending = spec.SortDirection == ImGuiSortDirection_Ascending;
                indexManager_.sort([&](size_t a, size_t b) {
                    if (column >= static_cast<int>(columns_.size())) return false;
                    std::string valA = (column < static_cast<int>(rows_[a].size())) ? rows_[a][column] : "";
                    std::string valB = (column < static_cast<int>(rows_[b].size())) ? rows_[b][column] : "";
                    if (ascending) return naturalCompare(valA, valB);
                    else return naturalCompare(valB, valA);
                });
            }
            specs->SpecsDirty = false;
        }
    }

    std::optional<size_t> ImGuiTable::checkKeyboard(size_t visibleRows) {
        if (!clickable_ || !ImGui::IsWindowFocused(ImGuiFocusedFlags_None)) {
            return std::nullopt;
        }
        
        int currentFrame = ImGui::GetFrameCount();
        if (currentFrame == lastInputFrame_) {
            return std::nullopt;
        }
        lastInputFrame_ = currentFrame;

        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
            indexManager_.navigateUp();
        } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
            indexManager_.navigateDown();
        } else if (ImGui::IsKeyPressed(ImGuiKey_PageUp, true)) {
            indexManager_.navigateUp(visibleRows);
        } else if (ImGui::IsKeyPressed(ImGuiKey_PageDown, true)) {
            indexManager_.navigateDown(visibleRows);
        } else if (ImGui::IsKeyPressed(ImGuiKey_Home, true)) {
            indexManager_.navigateHome();
        } else if (ImGui::IsKeyPressed(ImGuiKey_End, true)) {
            indexManager_.navigateEnd();
        } else {
            return std::nullopt;
        }
        
        return indexManager_.getCurrentRow();
    }

}
