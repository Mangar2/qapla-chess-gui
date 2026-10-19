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
        QaplaUi::TableFlags tableFlags,
        std::vector<ColumnDef> columns)
        : tableId_(std::move(tableId)),
        tableFlags_(tableFlags),
        columns_(std::move(columns)) {
    }

    ImGuiTable::ImGuiTable() = default;
    ImGuiTable::ImGuiTable(ImGuiTable&&) noexcept = default;
    ImGuiTable& ImGuiTable::operator=(ImGuiTable&&) noexcept = default;
    ImGuiTable::~ImGuiTable() = default;


    float ImGuiTable::calculateTextWidth(const std::string& text, float padding) const {
        return QaplaUi::calcTextSize(font(), QaplaUi::getFontSize(), text).x + padding;
    }

    void ImGuiTable::updated(std::optional<size_t> addedRow) {
        ++version_;
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

    std::string ImGuiTable::cellText(size_t row, size_t column, bool& alignRight) const {
        alignRight = column < columns_.size() && columns_[column].alignRight;
        if (row >= rows_.size() || column >= columns_.size() || column >= rows_[row].size()) {
            return {};
        }
        std::string content = rows_[row][column];
        if (columns_[column].customRender) {
            columns_[column].customRender(content, alignRight);
        }
        return content;
    }

    float ImGuiTable::layoutWidth(size_t column) const {
        if (column >= columns_.size()) {
            return 0.0F;
        }
        float colWidth = columns_[column].width;
        if (columns_[column].compute) {
            colWidth = std::max(colWidth, computeColumnWidth(column));
        }
        return colWidth;
    }

    std::optional<size_t> ImGuiTable::draw(const QaplaUi::Vec2& size, bool shrink) {
        constexpr float HORIZONTAL_SCROLLBAR_HEIGHT = 20.0F;
        QaplaUi::Vec2 tableSize = size;

        // Only the two text fonts are ever pushed; any other index keeps the current font.
        const bool pushFont = font() == QaplaUi::Font::Inter || font() == QaplaUi::Font::Mono;
        if (pushFont) {
            QaplaUi::pushFont(font());
        }

        float rowHeight = QaplaUi::getTextLineHeightWithSpacing();

        if (filterable_) {
            float cursorYBefore = QaplaUi::getCursorPosY();
            needsFilter_ |= filter_.draw();
            float cursorYAfter = QaplaUi::getCursorPosY();
            float filterHeight = cursorYAfter - cursorYBefore;
            tableSize.y = std::max(0.0F, tableSize.y - filterHeight);
            handleFiltering();
        }

        // Enables us to shrink the table height to fit the content
        if (shrink) {
            tableSize.y = std::min(tableSize.y, 
                static_cast<float>(indexManager_.size() + 1) * rowHeight // + 1 for header row
                + HORIZONTAL_SCROLLBAR_HEIGHT
            );
        }

        auto selected = QaplaUi::table(*this, tableSize);

        if (pushFont) {
            QaplaUi::popFont();
        }

        // Maintain the hight of the table without filtering by adding a dummy element
        // This prevents the UI from jumping when filtering is applied/removed
        if (keepSize_) {
            size_t totalRows = rows_.size();
            size_t filteredRows = indexManager_.size();
            if (filteredRows < totalRows) {
                float diffHeight = static_cast<float>(totalRows - filteredRows) * rowHeight;
                QaplaUi::dummy(QaplaUi::Vec2(0.0F, diffHeight));
            }
        }

        return selected;
    }

    void ImGuiTable::handleFiltering() {
        if (!filterable_ || !needsFilter_) {
            return;
        }
        needsFilter_ = false;
        needsSort_ = true;
        ++version_;
        indexManager_.filter([&](size_t rowIndex) {
            const auto& row = rows_[rowIndex];
            return filter_.matches(row);
        });
    }
    
    void ImGuiTable::sortBy(std::optional<size_t> column, bool ascending) {
        needsSort_ = false;
        ++version_;
        if (!column) {
            return;
        }
        auto sortColumn = *column;
        indexManager_.sort([&](size_t a, size_t b) {
            if (sortColumn >= columns_.size()) {
                return false;
            }
            std::string valA = (sortColumn < rows_[a].size()) ? rows_[a][sortColumn] : "";
            std::string valB = (sortColumn < rows_[b].size()) ? rows_[b][sortColumn] : "";
            if (ascending) {
                return naturalCompare(valA, valB);
            }
            return naturalCompare(valB, valA);
        });
    }

    std::optional<size_t> ImGuiTable::navigate(Navigation navigation, size_t pageRows) {
        ++version_;
        switch (navigation) {
        case Navigation::Up: indexManager_.navigateUp(); break;
        case Navigation::Down: indexManager_.navigateDown(); break;
        case Navigation::PageUp: indexManager_.navigateUp(pageRows); break;
        case Navigation::PageDown: indexManager_.navigateDown(pageRows); break;
        case Navigation::Home: indexManager_.navigateHome(); break;
        case Navigation::End: indexManager_.navigateEnd(); break;
        }
        return indexManager_.getCurrentRow();
    }

    std::string ImGuiTable::toText(size_t maxRows) const {
        if (columns_.empty() || rows_.empty()) {
            return {};
        }

        // Padded to the widest cell so the columns line up in a monospaced view, and so a reader
        // (human or model) can tell which value belongs to which heading without counting
        // separators. Measured in characters, not pixels -- the pixel widths this class computes
        // elsewhere mean nothing outside a font.
        std::vector<size_t> widths;
        widths.reserve(columns_.size());
        for (const auto& column : columns_) {
            widths.push_back(column.name.size());
        }
        size_t shownRows = std::min(maxRows, rows_.size());
        for (size_t rowIndex = 0; rowIndex < shownRows; ++rowIndex) {
            const auto& row = rows_[rowIndex];
            for (size_t col = 0; col < columns_.size() && col < row.size(); ++col) {
                widths[col] = std::max(widths[col], row[col].size());
            }
        }

        auto appendPadded = [](std::string& target, const std::string& cell, size_t width,
                                bool last) {
            target += cell;
            if (!last) {
                target.append(width - std::min(width, cell.size()), ' ');
                target += "  ";
            }
        };

        std::string text;
        size_t ruleWidth = 0;
        for (size_t col = 0; col < columns_.size(); ++col) {
            appendPadded(text, columns_[col].name, widths[col], col + 1 == columns_.size());
            ruleWidth += widths[col] + 2;
        }
        text += "\n";
        text.append(ruleWidth > 2 ? ruleWidth - 2 : ruleWidth, '-');
        text += "\n";

        for (size_t rowIndex = 0; rowIndex < shownRows; ++rowIndex) {
            const auto& row = rows_[rowIndex];
            for (size_t col = 0; col < columns_.size(); ++col) {
                appendPadded(text, col < row.size() ? row[col] : std::string{}, widths[col],
                    col + 1 == columns_.size());
            }
            text += "\n";
        }
        if (rows_.size() > shownRows) {
            text += "(" + std::to_string(rows_.size() - shownRows)
                + " further rows not listed here; the table in the application shows them all)\n";
        }
        return text;
    }

    float ImGuiTable::computeColumnWidth(size_t colIdx, float padding) const {
        if (colIdx >= columns_.size()) {
            return 0.0F;
        }
        float maxWidth = calculateTextWidth(columns_[colIdx].name, padding);
        for (const auto& row : rows_) {
            if (colIdx < row.size()) {
                float w = calculateTextWidth(row[colIdx], padding);
                if (w > maxWidth) {
                    maxWidth = w;
                }
            }
        }
        return maxWidth;
    }


}
