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

#include "callback-manager.h"

#include <iostream>

namespace QaplaWindows {

namespace Callback {

// UnregisterHandle implementation
UnregisterHandle::UnregisterHandle(Manager* manager, size_t callbackId)
    : manager_(manager), callbackId_(callbackId) {
}

UnregisterHandle::~UnregisterHandle() {
    unregister();
}

UnregisterHandle::UnregisterHandle(UnregisterHandle&& other) noexcept
    : manager_(other.manager_), callbackId_(other.callbackId_) {
    other.manager_ = nullptr;
}

UnregisterHandle& UnregisterHandle::operator=(UnregisterHandle&& other) noexcept {
    if (this != &other) {
        unregister(); // Unregister current callback
        
        manager_ = other.manager_;
        callbackId_ = other.callbackId_;
        
        other.manager_ = nullptr;
    }
    return *this;
}

void UnregisterHandle::unregister() {
    if (manager_) {
        manager_->unregisterCallback(callbackId_);
    }
}

// Manager implementation
Manager::Manager() : nextId_(1) {
    
}

std::unique_ptr<UnregisterHandle> Manager::registerCallback(Callback callback) {
    if (!callback) {
        return nullptr;
    }
    
    CallbackId id = nextId_.fetch_add(1);
    
    {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        callbacks_[id] = std::move(callback);
    }
    
    return std::make_unique<UnregisterHandle>(this, id);
}

bool Manager::unregisterCallback(CallbackId id) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    auto it = callbacks_.find(id);
    if (it != callbacks_.end()) {
        callbacks_.erase(it);
        return true;
    }
    return false;
}

void Manager::invokeAll() {
    // Create a copy of the callbacks to avoid issues with callbacks that
    // might modify the callbacks map during execution
    std::vector<Callback> callbacksCopy;
    
    {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        callbacksCopy.reserve(callbacks_.size());
        
        for (const auto& pair : callbacks_) {
            if (pair.second) {
                callbacksCopy.push_back(pair.second);
            }
        }
    }
    
    // Execute all callbacks without holding the lock
    for (const auto& callback : callbacksCopy) {
        try {
            callback();
        }
        catch (...) {
            // Catch any exceptions to ensure all callbacks are called
            // In a production environment, you might want to log this
            // or handle it differently based on your needs
        }
    }
}

size_t Manager::size() const {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    return callbacks_.size();
}

bool Manager::empty() const {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    return callbacks_.empty();
}

void Manager::clear() {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    callbacks_.clear();
}

} // namespace Callback

} // namespace QaplaWindows
