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

#include "table-index-manager.h"

#include <algorithm>
#include <functional>

namespace QaplaWindows {

TableIndexManager::TableIndexManager(Mode mode) : mode_(mode) {}

void TableIndexManager::updateSize(size_t size) {
    if (size == size_) return; // No change
    size_ = size;
    sortedIndices_.resize(size);
    for (size_t i = 0; i < size; ++i) sortedIndices_[i] = i;
}

void TableIndexManager::setSortedIndices(const std::vector<size_t>& sortedIndices) {
    if (mode_ == Sorted) {
        sortedIndices_ = sortedIndices;
        // Reset currentIndex if out of bounds
        if (currentIndex_ && *currentIndex_ >= sortedIndices_.size()) {
            currentIndex_.reset();
        }
    }
}

size_t TableIndexManager::size() const {
    return mode_ == Sorted ? sortedIndices_.size() : size_;
}

void TableIndexManager::setCurrentIndex(size_t index) {
    if (size() == 0) {
        currentIndex_.reset();
    } else {
        currentIndex_ = std::min(index, size() - 1);
    }
}

void TableIndexManager::setCurrentRow(size_t row) {
    if (mode_ == Unsorted) {
        setCurrentIndex(row);
    } else {
        // Find the index in sortedIndices_
        for (size_t i = 0; i < sortedIndices_.size(); ++i) {
            if (sortedIndices_[i] == row) {
                currentIndex_ = i;
                return;
            }
        }
        currentIndex_.reset(); // Not found
    }
}

std::optional<size_t> TableIndexManager::getCurrentRow() const {
    if (!currentIndex_) {
        return std::nullopt;
    }
    if (mode_ == Unsorted) {
        return *currentIndex_;
    } else {
        return sortedIndices_[*currentIndex_];
    }
}

void TableIndexManager::navigateUp(size_t rows) {
    setCurrentIndex((currentIndex_ && *currentIndex_ > rows) ? *currentIndex_ - rows : 0);
}

void TableIndexManager::navigateDown(size_t rows) {
    setCurrentIndex(currentIndex_ ? *currentIndex_ + rows : 0);
}

void TableIndexManager::navigateHome() {
    setCurrentIndex(0);
}

void TableIndexManager::navigateEnd() {
    setCurrentIndex(size() == 0 ? 0 : size() - 1);
}

std::optional<size_t> TableIndexManager::getRowIndex(size_t row) const {
    if (mode_ == Unsorted) {
        if (row < size_) {
            return row;
        }
        return std::nullopt;
    } else {
        for (size_t i = 0; i < sortedIndices_.size(); ++i) {
            if (sortedIndices_[i] == row) {
                return i;
            }
        }
        return std::nullopt;
    }
}

void TableIndexManager::sort(const std::function<bool(size_t, size_t)>& compare, size_t size) {
    updateSize(size);
    if (mode_ == Sorted) {
        std::sort(sortedIndices_.begin(), sortedIndices_.end(), compare);
    }
}

} // namespace QaplaWindows