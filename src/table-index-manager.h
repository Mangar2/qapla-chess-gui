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

#include "games-filter.h"

#include <vector>
#include <optional>
#include <cstddef>
#include <functional>

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
     * @brief Initializes the index manager with the given size.
     * @param size The total number of rows.
     */
    void updateSize(size_t size);

    /**
     * @brief Sets the sorted indices for Sorted mode.
     * @param sortedIndices The sorted indices vector.
     */
    void setSortedIndices(const std::vector<size_t>& sortedIndices);

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

    /**
     * @brief Gets the actual row number for a given index.
     * @param index The index to convert.
     * @return The corresponding row number.
     */
    size_t getRowNumber(size_t index) const {
        if (mode_ == Sorted && index < sortedIndices_.size()) {
            return sortedIndices_[index];
        }
        return index;
    }

    /**
     * @brief Gets the index for a given row number.
     * @param row The row number to convert.
     * @return The corresponding index, or nullopt if not found.
     */
    std::optional<size_t> getRowIndex(size_t row) const;

    /**
     * @brief Get the Sorted Indices object 
     * 
     * @return std::vector<size_t>& 
     */
    std::vector<size_t>& getSortedIndices() {
        return sortedIndices_;
    }

    /**
     * @brief Sorts the indices based on the provided comparison function.
     * @param compare A comparison function that takes two row numbers and returns true if the first should come before the second.
     * @param size The total number of rows.
     */
    void sort(const std::function<bool(size_t, size_t)>& compare, size_t size);



private:
    Mode mode_;
    std::optional<size_t> currentIndex_; // Index into the table (0 to size()-1)
    std::vector<size_t> sortedIndices_;
    size_t size_ = 0; // For Unsorted mode
};

} // namespace QaplaWindows