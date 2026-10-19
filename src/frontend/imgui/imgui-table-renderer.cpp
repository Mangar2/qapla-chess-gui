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

/**
 * @file imgui-table-renderer.cpp
 * @brief QaplaUi::table() on Dear ImGui: an ImGui table with a clipper over the table's rows.
 */

#include "imgui-ui-backend.h"

#include "imgui-table.h"
#include "i18n.h"

#include <algorithm>
#include <string>

using QaplaWindows::ImGuiTable;
using QaplaWindows::Translator;

namespace {

    void alignRight(const std::string& content) {
        constexpr float RIGHT_PADDING = 10.0F;
        const float colWidth = ImGui::GetColumnWidth();
        const float textWidth = ImGui::CalcTextSize(content.c_str()).x;
        // Never to the left of the cell: text wider than its column used to be pushed out on the
        // left, where the cell's clip rectangle cut it off -- so "-280.00" in a column with room
        // for six characters showed as "280.00", a tablebase loss read as a winning score. Too
        // wide now starts at the left edge and loses its last characters instead of its first.
        const float offset = std::max(0.0F, colWidth - textWidth - RIGHT_PADDING);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
    }

    void textAligned(const std::string& content, bool right) {
        if (right) {
            alignRight(content);
        }
        ImGui::TextUnformatted(content.c_str());
    }

    void headerAligned(const std::string& content, bool right) {
        if (right) {
            alignRight(content);
        }
        ImGui::TableHeader(content.c_str());
    }

    void setupTable(const ImGuiTable& table) {
        ImGui::TableSetupScrollFreeze(0, 1);
        const auto& columns = table.columns();
        for (size_t i = 0; i < columns.size(); ++i) {
            ImGui::TableSetupColumn(columns[i].name.c_str(), QaplaImGui::toIm(columns[i].flags),
                table.layoutWidth(i), static_cast<ImGuiID>(i));
        }
    }

    void tableHeadersRow(const ImGuiTable& table) {
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        const auto& columns = table.columns();
        for (size_t columnN = 0; columnN < columns.size(); columnN++) {
            if (!ImGui::TableSetColumnIndex(static_cast<int>(columnN))) {
                continue;
            }
            std::string translatedName = Translator::instance().translate("Table", columns[columnN].name);
            ImGui::PushID(static_cast<int>(columnN));
            headerAligned(translatedName, columns[columnN].alignRight);
            ImGui::PopID();
        }
    }

    void handleSorting(ImGuiTable& table) {
        ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs();
        if (specs == nullptr) {
            return;
        }
        if (table.needsSort() || specs->SpecsDirty) {
            if (specs->SpecsCount > 0) {
                const auto& spec = specs->Specs[0];
                table.sortBy(static_cast<size_t>(spec.ColumnUserID),
                    spec.SortDirection == ImGuiSortDirection_Ascending);
            } else {
                table.sortBy(std::nullopt, true);
            }
            specs->SpecsDirty = false;
        }
    }

    std::optional<float> calculateOptimalScrollPosition(size_t rowIndex) {
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

    void handleScrolling(ImGuiTable& table) {
        auto scrollToRow = table.takeScrollToRow();
        if (!scrollToRow) {
            return;
        }
        auto position = table.positionOf(*scrollToRow);
        if (position) {
            auto scrollPos = calculateOptimalScrollPosition(*position);
            if (scrollPos) {
                ImGui::SetScrollY(*scrollPos);
            }
        }
    }

    bool isRowClicked(const ImGuiTable& table, size_t index) {
        if (!table.isClickable()) {
            return false;
        }
        std::string id = "row" + std::to_string(index);
        ImGui::PushID(id.c_str());
        bool clicked = ImGui::Selectable("##row", false,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
        ImGui::PopID();
        ImGui::SameLine(0.0F, 0.0F);
        return clicked;
    }

    void accentuateCurrentRow(const ImGuiTable& table, size_t rowIndex) {
        auto current = table.currentRow();
        if (!current || *current != rowIndex) {
            return;
        }
        auto baseColor = ImGui::GetStyleColorVec4(ImGuiCol_TabDimmedSelected);
        auto baseColor32 = ImGui::ColorConvertFloat4ToU32(baseColor);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, baseColor32);
    }

    void drawRow(const ImGuiTable& table, size_t rowIndex) {
        const auto columnCount = table.columns().size();
        const auto rowSize = table.cellCount(rowIndex);
        for (size_t col = 0; col < columnCount && col < rowSize; ++col) {
            ImGui::TableSetColumnIndex(static_cast<int>(col));
            bool right = false;
            auto content = table.cellText(rowIndex, col, right);
            textAligned(content, right);
        }
    }

    void handleClipping(const ImGuiTable& table, std::optional<size_t>& clickedRow) {
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(table.visibleRowCount()));
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                size_t actualRow = table.rowAt(static_cast<size_t>(i));
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (isRowClicked(table, actualRow)) {
                    clickedRow = actualRow;
                }
                accentuateCurrentRow(table, actualRow);
                drawRow(table, actualRow);
            }
        }
    }

    /**
     * Note it is important that this function returns std::nullopt if no key was pressed.
     * Returning an index causes the selection of a table row, and thus maybe loading a
     * new Game.
     */
    std::optional<size_t> checkKeyboard(ImGuiTable& table, size_t visibleRows) {
        if (!table.isClickable() || !ImGui::IsWindowFocused(ImGuiFocusedFlags_None)) {
            return std::nullopt;
        }
        if (!table.claimInputFrame(ImGui::GetFrameCount())) {
            return std::nullopt;
        }

        using Navigation = ImGuiTable::Navigation;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
            return table.navigate(Navigation::Up, visibleRows);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
            return table.navigate(Navigation::Down, visibleRows);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_PageUp, true)) {
            return table.navigate(Navigation::PageUp, visibleRows);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_PageDown, true)) {
            return table.navigate(Navigation::PageDown, visibleRows);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Home, true)) {
            return table.navigate(Navigation::Home, visibleRows);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_End, true)) {
            return table.navigate(Navigation::End, visibleRows);
        }
        return std::nullopt;
    }

} // namespace

namespace QaplaUi {

    std::optional<std::size_t> table(QaplaWindows::ImGuiTable& table, Vec2 size) {
        std::optional<size_t> clickedRow;
        std::optional<size_t> keyboardRow;

        float rowHeight = ImGui::GetTextLineHeightWithSpacing();
        auto visibleRows = static_cast<size_t>(size.y / rowHeight);
        if (visibleRows == 0) {
            visibleRows = 1; // Fallback
        }

        if (ImGui::BeginTable(table.tableId().c_str(), static_cast<int>(table.columns().size()),
            QaplaImGui::toIm(table.tableFlags()), QaplaImGui::toIm(size))) {
            setupTable(table);
            handleSorting(table);
            tableHeadersRow(table);
            handleScrolling(table);
            handleClipping(table, clickedRow);
            keyboardRow = checkKeyboard(table, visibleRows);
            ImGui::EndTable();
        }

        return keyboardRow ? keyboardRow : clickedRow;
    }

} // namespace QaplaUi
