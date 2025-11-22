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
#include "i18n.h"

#include <imgui.h>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace QaplaWindows {

    /**
     * @brief Skips leading zeros in a string starting at position pos.
     * @param str The string to process
     * @param pos The starting position (will be updated)
     */
    static void skipLeadingZeros(const std::string& str, size_t& pos) {
        while (pos < str.size() && str[pos] == '0') {
            ++pos;
        }
    }

    /**
     * @brief Finds the end position of a numeric sequence in a string.
     * @param str The string to process
     * @param start The starting position
     * @return The position after the last digit
     */
    static size_t findNumericEnd(const std::string& str, size_t start) {
        size_t end = start;
        while (end < str.size() && std::isdigit(static_cast<unsigned char>(str[end])) != 0) {
            ++end;
        }
        return end;
    }

    /**
     * @brief Counts leading zeros in a numeric sequence.
     * @param str The string to process
     * @param start The starting position of the numeric sequence
     * @return The number of leading zeros
     */
    static size_t countLeadingZeros(const std::string& str, size_t start) {
        size_t count = 0;
        while (start + count < str.size() && str[start + count] == '0') {
            ++count;
        }
        return count;
    }

    /**
     * @brief Compares two numeric sequences in strings.
     * @param a First string
     * @param b Second string
     * @param posA Position in first string (will be updated)
     * @param posB Position in second string (will be updated)
     * @return Comparison result: negative if a < b, positive if a > b, 0 if equal
     */
    static int compareNumericSequences(const std::string& a, const std::string& b, 
                                       size_t& posA, size_t& posB) {
        size_t numStartA = posA;
        size_t numStartB = posB;
        
        skipLeadingZeros(a, posA);
        skipLeadingZeros(b, posB);
        
        size_t numEndA = findNumericEnd(a, posA);
        size_t numEndB = findNumericEnd(b, posB);
        
        // Compare by length first (longer number = larger)
        size_t lenA = numEndA - posA;
        size_t lenB = numEndB - posB;
        
        if (lenA != lenB) {
            posA = numEndA;
            posB = numEndB;
            return (lenA < lenB) ? -1 : 1;
        }
        
        // Same length: compare digit by digit
        while (posA < numEndA && posB < numEndB) {
            if (a[posA] != b[posB]) {
                char result = (a[posA] < b[posB]) ? -1 : 1;
                posA = numEndA;
                posB = numEndB;
                return result;
            }
            ++posA;
            ++posB;
        }
        
        // Numbers are equal, compare leading zeros count
        size_t zeroCountA = countLeadingZeros(a, numStartA);
        size_t zeroCountB = countLeadingZeros(b, numStartB);
        
        if (zeroCountA != zeroCountB) {
            return (zeroCountA < zeroCountB) ? -1 : 1;
        }
        
        return 0;
    }

    /**
     * @brief Natural sort comparison for strings containing numbers.
     * Compares strings in a human-friendly way: "1" < "2" < "10" instead of "1" < "10" < "2".
     * @param a First string to compare
     * @param b Second string to compare
     * @return true if a should come before b in natural sort order
     */
    static bool naturalCompare(const std::string& a, const std::string& b) {
        size_t i = 0;
        size_t j = 0;
        
        while (i < a.size() && j < b.size()) {
            if (std::isdigit(static_cast<unsigned char>(a[i])) != 0 && 
                std::isdigit(static_cast<unsigned char>(b[j])) != 0) {
                
                int cmp = compareNumericSequences(a, b, i, j);
                if (cmp != 0) {
                    return cmp < 0;
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

    ImGuiTable::ImGuiTable(std::string tableId,
        ImGuiTableFlags tableFlags,
        std::vector<ColumnDef> columns)
        : tableId_(std::move(tableId)),
        tableFlags_(tableFlags),
        columns_(std::move(columns)) {
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

    void ImGuiTable::updated(std::optional<size_t> addedRow) {
        needsSort_ = true;
        needsFilter_ = true;
        indexManager_.updateSize(rows_.size(), addedRow);
    }

    void ImGuiTable::push(const std::vector<std::string>& row) {
        rows_.push_back(row);
        updated(rows_.size() - 1);
    }

    void ImGuiTable::push_front(const std::vector<std::string>& row) {
        rows_.insert(rows_.begin(), row);
        updated(0);
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
            if (!ImGui::TableSetColumnIndex(static_cast<int>(columnN))) {
                continue;
            }
            std::string translatedName = Translator::instance().translate("Table", columns_[columnN].name);
            ImGui::PushID(static_cast<int>(columnN));
            headerAligned(translatedName, columns_[columnN].alignRight);
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
        
        float rowTop = static_cast<float>(rowIndex + 1) * rowHeight; // +1 for header row
        float rowBottom = rowTop + rowHeight;
        float visibleTop = scrollY + rowHeight; 
        float visibleBottom = scrollY + windowHeight;
        
        // Row visible, no scroll needed
        if (rowTop >= visibleTop && rowBottom <= visibleBottom) {
            return std::nullopt;  
        }

        // Row far away → scroll to center
        if (rowBottom + windowHeight < visibleTop || rowTop - windowHeight > visibleBottom) {
            return rowTop - (windowHeight * 0.5F) + (rowHeight * 0.5F);
        }
        
        // Row too far up (but close) → scroll to top
        if (rowBottom < visibleBottom) {
            return rowTop - rowHeight;
        }

        // Row too far down → scroll to bottom
        return rowBottom - windowHeight;
    }

    bool ImGuiTable::isRowClicked(size_t index) const {
        if (!clickable_) {
            return false;
        }
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
        if (filterable_) {
            float cursorYBefore = ImGui::GetCursorPosY();
            needsFilter_ |= filter_.draw();
            float cursorYAfter = ImGui::GetCursorPosY();
            float filterHeight = cursorYAfter - cursorYBefore;
            tableSize.y = std::max(0.0F, tableSize.y - filterHeight);
            handleFiltering();
        }

        if (shrink) {
            tableSize.y = std::min(tableSize.y, static_cast<float>(indexManager_.size() + 2) * rowHeight);
        }

        // Calculate visible rows
        auto visibleRows = static_cast<size_t>(tableSize.y / rowHeight);
        if (visibleRows == 0) {
            visibleRows = 1; // Fallback
        }

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

    void ImGuiTable::handleFiltering() {
        if (!filterable_ || !needsFilter_) {
            return;
        }
        needsFilter_ = false;
        needsSort_ = true;
        indexManager_.filter([&](size_t rowIndex) {
            const auto& row = rows_[rowIndex];
            return filter_.matches(row);
        });
    }
    
    void ImGuiTable::handleSorting() {
        ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs();
        if (specs == nullptr) {
            return;
        }
        if (needsSort_ || (specs != nullptr && specs->SpecsDirty)) {
            needsSort_ = false;
            if (specs->SpecsCount > 0) {
                auto spec = specs->Specs[0];
                auto column = static_cast<int>(spec.ColumnUserID);
                bool ascending = spec.SortDirection == ImGuiSortDirection_Ascending;
                indexManager_.sort([&](size_t a, size_t b) {
                    auto columnSize = static_cast<int>(columns_.size());
                    if (column >= columnSize) {
                        return false;
                    }
                    auto rowASizeInt = static_cast<int>(rows_[a].size());
                    auto rowBSizeInt = static_cast<int>(rows_[b].size());
                    std::string valA = (column < rowASizeInt) ? rows_[a][static_cast<size_t>(column)] : "";
                    std::string valB = (column < rowBSizeInt) ? rows_[b][static_cast<size_t>(column)] : "";
                    if (ascending) {
                        return naturalCompare(valA, valB);
                    }
                    return naturalCompare(valB, valA);
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
