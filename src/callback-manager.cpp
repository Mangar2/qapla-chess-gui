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

namespace QaplaWindows::Callback {

// UnregisterHandle implementation
UnregisterHandle::UnregisterHandle(Unregistration* unregisterable, size_t callbackId)
    : unregisterable_(unregisterable), callbackId_(callbackId) {
}

UnregisterHandle::~UnregisterHandle() {
    unregister();
}

UnregisterHandle::UnregisterHandle(UnregisterHandle&& other) noexcept
    : unregisterable_(other.unregisterable_), callbackId_(other.callbackId_) {
    other.unregisterable_ = nullptr;
}

UnregisterHandle& UnregisterHandle::operator=(UnregisterHandle&& other) noexcept {
    if (this != &other) {
        unregister(); // Unregister current callback
        
        unregisterable_ = other.unregisterable_;
        callbackId_ = other.callbackId_;
        
        other.unregisterable_ = nullptr;
    }
    return *this;
}

void UnregisterHandle::unregister() {
    if (unregisterable_ != nullptr) {
        unregisterable_->unregister(callbackId_);
    }
}



} // namespace QaplaWindows::Callback

