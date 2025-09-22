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

#include <vector>
#include <optional>
#include <cstddef>

namespace QaplaWindows {

/**
 * @brief Manages table row indices in sorted or unsorted mode transparently.
 */
class TableIndexManager {
public:
    enum Mode { Unsorted, Sorted };

    /**
     * @brief Constructs a TableIndexManager with the specified mode.
     * @param mode The initial mode (Unsorted or Sorted).
     */
    TableIndexManager(Mode mode = Unsorted);

    /**
     * @brief Sets the mode and optionally the sorted indices.
     * @param mode The new mode.
     * @param sortedIndices The sorted indices vector (required for Sorted mode).
     */
    void setMode(Mode mode, const std::vector<size_t>& sortedIndices = {});

    /**
     * @brief Sets the sorted indices for Sorted mode.
     * @param sortedIndices The sorted indices vector.
     */
    void setSortedIndices(const std::vector<size_t>& sortedIndices);

    /**
     * @brief Sets the size for Unsorted mode.
     * @param size The total number of rows.
     */
    void setSize(size_t size);

    /**
     * @brief Gets the size (total number of indices).
     * @return The size of the table.
     * @note Valid indices are from 0 to size() - 1.
     */
    size_t size() const;

    /**
     * @brief Sets the current index directly.
     * @param index The index to set (must be < maxIndex()).
     */
    void setCurrentIndex(size_t index);

    /**
     * @brief Gets the current index.
     * @return The current index, or nullopt if none is set.
     */
    std::optional<size_t> getCurrentIndex() const {
        return currentIndex_;
    }

    /**
     * @brief Sets the current row (maps to index internally).
     * @param row The row number to set as current.
     */
    void setCurrentRow(size_t row);

    /**
     * @brief Gets the current row.
     * @return The current row number, or nullopt if none.
     */
    std::optional<size_t> getCurrentRow() const;

    /**
     * @brief Navigates up by the number of visible rows (page up).
     * @param rows The number of rows to jump.
     */
    void navigateUp(size_t rows = 1);

    /**
     * @brief Navigates down by the number of visible rows (page down).
     * @param rows The number of rows to jump.
     */
    void navigateDown(size_t rows = 1);

    /**
     * @brief Navigates to the first row.
     */
    void navigateHome();

    /**
     * @brief Navigates to the last row.
     */
    void navigateEnd();

private:
    Mode mode_;
    std::optional<size_t> currentIndex_; // Index into the table (0 to size()-1)
    std::vector<size_t> sortedIndices_;
    size_t size_ = 0; // For Unsorted mode
};

} // namespace QaplaWindows