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

#include "table-index.h"

#include <algorithm>
#include <functional>
#include <utility>

namespace QaplaWindows {

TableIndex::TableIndex(Mode mode) : mode_(mode) {}

void TableIndex::updateSize(size_t size) {
    if (size == unfilteredSize_) return; // No change
    unfilteredSize_ = size;
    clearFilter();
    if (mode_ == Unsorted) return;
    sortedIndices_.resize(size);
    for (size_t i = 0; i < size; ++i) sortedIndices_[i] = i;
}

size_t TableIndex::size() const {
    return (mode_ == Unsorted) ? unfilteredSize_ : filteredSize_;
}

void TableIndex::setCurrentIndex(size_t index) {
    if (size() == 0) {
        currentIndex_.reset();
    } else {
        currentIndex_ = std::min(index, size() - 1);
    }
}

void TableIndex::setCurrentRow(size_t row) {
    if (mode_ == Unsorted) {
        setCurrentIndex(row);
    } else {
        // Find the index in sortedIndices_
        for (size_t i = 0; i < size(); ++i) {
            if (sortedIndices_[i] == row) {
                currentIndex_ = i;
                return;
            }
        }
        currentIndex_.reset(); // Not found
    }
}

std::optional<size_t> TableIndex::getCurrentRow() const {
    if (!currentIndex_) {
        return std::nullopt;
    }
    if (mode_ == Unsorted) {
        return *currentIndex_;
    } else {
        return sortedIndices_[*currentIndex_];
    }
}

void TableIndex::navigateUp(size_t rows) {
    setCurrentIndex((currentIndex_ && *currentIndex_ > rows) ? *currentIndex_ - rows : 0);
}

void TableIndex::navigateDown(size_t rows) {
    setCurrentIndex(currentIndex_ ? *currentIndex_ + rows : 0);
}

void TableIndex::navigateHome() {
    setCurrentIndex(0);
}

void TableIndex::navigateEnd() {
    setCurrentIndex(size() == 0 ? 0 : size() - 1);
}

std::optional<size_t> TableIndex::getRowIndex(size_t row) const {
    if (mode_ == Unsorted) {
        if (row < size()) {
            return row;
        }
        return std::nullopt;
    } else {
        for (size_t i = 0; i < size(); ++i) {
            if (sortedIndices_[i] == row) {
                return i;
            }
        }
        return std::nullopt;
    }
}

void TableIndex::sort(const std::function<bool(size_t, size_t)>& compare) {
    if (mode_ == Sorted) {
        std::sort(sortedIndices_.begin(), sortedIndices_.begin() + filteredSize_, compare);
    }
}

void TableIndex::filter(const std::function<bool(size_t)>& predicate) {
    // We need a sortedIndex_ to filter rows
    if (mode_ == Unsorted) return;

    size_t writeIndex = 0;
    const auto selectedRow = getCurrentRow();
    for (size_t i = 0; i < sortedIndices_.size(); ++i) {
        size_t row = sortedIndices_[i];
        if (predicate(row)) {
            if (writeIndex != i) {
                std::swap(sortedIndices_[writeIndex], sortedIndices_[i]);
            }
            ++writeIndex;
        }
    }
    filteredSize_ = writeIndex;
    currentIndex_.reset();
    if (selectedRow) {
        // Try to keep the current row if still visible
        for (size_t i = 0; i < filteredSize_; ++i) {
            if (sortedIndices_[i] == *selectedRow) {
                currentIndex_ = i;
                return;
            }
        }
    } 
}

} // namespace QaplaWindows