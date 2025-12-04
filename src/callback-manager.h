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
#include <functional>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <iostream>
#include <type_traits>

#include "game-record.h"

namespace QaplaWindows {

namespace Callback {

class Unregisterable {
public:
    using CallbackId = size_t;

    virtual ~Unregisterable() = default;
    virtual bool unregister(CallbackId id) = 0;
};

/**
 * @brief RAII handle for automatically unregistering callbacks.
 * 
 * This class provides automatic unregistration of callbacks when the
 * handle goes out of scope or is explicitly destroyed.
 * 
 * @param unregisterable Pointer to the Unregistration manager.
 * @param callbackId The ID of the callback to unregister.
 */
class UnregisterHandle {
public:
    UnregisterHandle(Unregisterable* unregisterable, Unregisterable::CallbackId callbackId);
    ~UnregisterHandle();
    
    // Move constructor and assignment
    UnregisterHandle(UnregisterHandle&& other) noexcept;
    UnregisterHandle& operator=(UnregisterHandle&& other) noexcept;
    
    // Delete copy constructor and assignment
    UnregisterHandle(const UnregisterHandle&) = delete;
    UnregisterHandle& operator=(const UnregisterHandle&) = delete;
    
private:
    /**
     * @brief Manually unregister the callback.
     * After calling this, the handle becomes invalid.
     */
    void unregister();

    Unregisterable* unregisterable_;
    Unregisterable::CallbackId callbackId_;
};

/**
 * @brief Manager class for handling callbacks with optional return type.
 * 
 * This class manages a list of callbacks that can take parameters and
 * optionally return a value. Callbacks can be registered and unregistered,
 * and all registered callbacks can be invoked with the provided arguments.
 * 
 * @tparam R The return type of the callbacks (default: void)
 * @tparam Args The types of the arguments that the callbacks will accept.
 */

template <typename R, typename... Args>
class ManagerBase : public Unregisterable {
public:
    using Callback = std::function<R(Args...)>;

private:
    std::unordered_map<CallbackId, Callback> callbacks_;
    std::atomic<CallbackId> nextId_;
    mutable std::mutex callbacks_mutex_;

public:

    /**
     * @brief Construct a new Callback Manager object   
     */
    ManagerBase() : nextId_(1) {
    };

    ~ManagerBase() override = default;

    /**
     * @brief Register a callback to the list.
     * @param callback The function/lambda to be registered
     * @return A unique_ptr to an UnregisterHandle for automatic cleanup
     */
    std::unique_ptr<UnregisterHandle> registerCallback(Callback callback) {
        if (!callback) {
            return nullptr;
        }

        CallbackId id = nextId_.fetch_add(1);

        {
            std::scoped_lock lock(callbacks_mutex_);
            callbacks_[id] = std::move(callback);
        }

        return std::make_unique<UnregisterHandle>(this, id);
    }

    /**
     * @brief Manually unregister a callback by its ID.
     * @param id The ID of the callback to unregister
     * @return true if the callback was found and removed, false otherwise
     */
    bool unregister(CallbackId id) override {
        std::scoped_lock lock(callbacks_mutex_);
        auto it = callbacks_.find(id);
        if (it != callbacks_.end()) {
            callbacks_.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Invoke all registered callbacks (for void return type).
     * 
     * Calls all callbacks in the order they were added.
     * If a callback throws an exception, it will be caught and
     * the remaining callbacks will still be executed.
     * 
     * @note This method is only available when R is void.
     */
    void invokeAll(Args... args) requires std::is_void_v<R> {
        // Create a copy of the callbacks to avoid issues with callbacks that
        // might modify the callbacks map during execution
        std::vector<Callback> callbacksCopy;

        {
            std::scoped_lock lock(callbacks_mutex_);
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
                callback(args...);
            }
            catch (...) {
                // Catch any exceptions to ensure all callbacks are called
                // In a production environment, you might want to log this
                // or handle it differently based on your needs
            }
        }
    }

    /**
     * @brief Invoke all callbacks and return true if ANY callback returns true.
     * 
     * Calls all callbacks and returns true if at least one callback returns true.
     * All callbacks are always called, regardless of individual return values.
     * 
     * @note This method is only available when R is bool.
     * @return true if at least one callback returned true, false otherwise
     */
    bool invokeAny(Args... args) requires std::is_same_v<R, bool> {
        std::vector<Callback> callbacksCopy;

        {
            std::scoped_lock lock(callbacks_mutex_);
            callbacksCopy.reserve(callbacks_.size());

            for (const auto& pair : callbacks_) {
                if (pair.second) {
                    callbacksCopy.push_back(pair.second);
                }
            }
        }

        bool result = false;
        for (const auto& callback : callbacksCopy) {
            try {
                if (callback(args...)) {
                    result = true;
                }
            }
            catch (...) {
                // Catch any exceptions to ensure all callbacks are called
            }
        }
        return result;
    }

    /**
     * @brief Invoke all callbacks and return true if ALL callbacks return true.
     * 
     * Calls all callbacks and returns true only if all callbacks return true.
     * All callbacks are always called, regardless of individual return values.
     * Returns true if there are no callbacks registered.
     * 
     * @note This method is only available when R is bool.
     * @return true if all callbacks returned true (or no callbacks registered), false otherwise
     */
    bool invokeEvery(Args... args) requires std::is_same_v<R, bool> {
        std::vector<Callback> callbacksCopy;

        {
            std::scoped_lock lock(callbacks_mutex_);
            callbacksCopy.reserve(callbacks_.size());

            for (const auto& pair : callbacks_) {
                if (pair.second) {
                    callbacksCopy.push_back(pair.second);
                }
            }
        }

        bool result = true;
        for (const auto& callback : callbacksCopy) {
            try {
                if (!callback(args...)) {
                    result = false;
                }
            }
            catch (...) {
                // Catch any exceptions to ensure all callbacks are called
                result = false;
            }
        }
        return result;
    }

    /**
     * @brief Get the number of registered callbacks.
     * @return Number of callbacks in the list
     */
    size_t size() const {
        std::scoped_lock lock(callbacks_mutex_);
        return callbacks_.size();
    }

    /**
     * @brief Check if there are any registered callbacks.
     * @return true if no callbacks are registered, false otherwise
     */
    bool empty() const {
        std::scoped_lock lock(callbacks_mutex_);
        return callbacks_.empty();
    }

    /**
     * @brief Clear all registered callbacks (optional utility method).
     * 
     * Note: This method is provided as a utility, though the requirement
     * specified that deletion is not needed. This can be useful for
     * cleanup or testing scenarios.
     */
    void clear() {
        std::scoped_lock lock(callbacks_mutex_);
        callbacks_.clear();
    }
};

/**
 * @brief Alias for void-returning callback manager (backwards compatible).
 * 
 * This alias maintains backwards compatibility with existing code that uses
 * Manager<Args...> for void-returning callbacks.
 * 
 * @tparam Args The types of the arguments that the callbacks will accept.
 */
template <typename... Args>
using Manager = ManagerBase<void, Args...>;

} // namespace Callback

class StaticCallbacks {
public:
    static Callback::Manager<>& poll() {
        static Callback::Manager<> instance;
        return instance;
    }    

    static Callback::Manager<>& save() {
        static Callback::Manager<> instance;
        return instance;
    }

    static Callback::Manager<std::string>& message() {
        static Callback::Manager<std::string> instance;
        return instance;
    }

    static Callback::Manager<const QaplaTester::GameRecord&>& gameUpdated() {
        static Callback::Manager<const QaplaTester::GameRecord&> instance;
        return instance;
    }
  
};

} // namespace QaplaWindows
