/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include "ui-update-queue.h"

#include <utility>

namespace QaplaWindows {

UiUpdateQueue& UiUpdateQueue::instance() {
    static UiUpdateQueue instance;
    return instance;
}

void UiUpdateQueue::post(std::function<void()> change) {
    if (!change) {
        return;
    }
    std::scoped_lock lock(mutex_);
    pending_.push_back(std::move(change));
}

void UiUpdateQueue::applyAll() {
    std::vector<std::function<void()>> changes;
    {
        std::scoped_lock lock(mutex_);
        changes.swap(pending_);
    }
    // Taken out from under the lock first: a change is free to post another one, and it is free
    // to take as long as a change may take -- which is not long, but the lock has no business
    // being held while it does.
    for (auto& change : changes) {
        change();
    }
}

bool UiUpdateQueue::empty() const {
    std::scoped_lock lock(mutex_);
    return pending_.empty();
}

} // namespace QaplaWindows
