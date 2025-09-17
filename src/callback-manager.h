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

/**
 * @brief Class for managing parameterless callbacks with automatic unregistration.
 * 
 * This class provides a comprehensive callback management system where callbacks
 * can be added to a list with automatic cleanup capabilities. Each callback
 * receives a unique ID and returns an UnregisterHandle for RAII-style cleanup.
 * Access is provided through a static instance for global usage.
 * 
 * Features:
 * - Unique ID assignment to each callback
 * - RAII-style automatic unregistration via UnregisterHandle
 * - Manual unregistration by callback ID
 * - Static instance access via instance() method
 * - Thread-safe operations with atomic ID generation
 * - Exception-safe callback invocation
 * 
 * Usage example:
 * @code
 * // Register a callback with automatic cleanup
 * auto handle = Manager::instance().registerCallback([]() {
 *     std::cout << "Callback executed!" << std::endl;
 * });
 * 
 * // Execute all callbacks
 * Manager::instance().invokeAll();
 * 
 * // Callback will be automatically unregistered when handle goes out of scope
 * // Or manually: handle->unregister();
 * @endcode
 */

namespace QaplaWindows {

namespace Callback {

// Forward declaration
class Manager;

/**
 * @brief RAII handle for automatically unregistering callbacks.
 * 
 * This class provides automatic unregistration of callbacks when the
 * handle goes out of scope or is explicitly destroyed.
 */
class UnregisterHandle {
public:
    UnregisterHandle(Manager* manager, size_t callbackId);
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

    Manager* manager_;
    size_t callbackId_;
};

class Manager { 
public:
    using Callback = std::function<void()>;
    using CallbackId = size_t;

private:
    std::unordered_map<CallbackId, Callback> callbacks_;
    std::atomic<CallbackId> nextId_;
    mutable std::mutex callbacks_mutex_;

public:

    /**
     * @brief Construct a new Callback Manager object   
     */
    Manager();

    /**
     * @brief Register a callback to the list.
     * @param callback The parameterless function/lambda to be registered
     * @return A unique_ptr to an UnregisterHandle for automatic cleanup
     */
    std::unique_ptr<UnregisterHandle> registerCallback(Callback callback);

    /**
     * @brief Manually unregister a callback by its ID.
     * @param id The ID of the callback to unregister
     * @return true if the callback was found and removed, false otherwise
     */
    bool unregisterCallback(CallbackId id);

    /**
     * @brief Invoke all registered callbacks.
     * 
     * Calls all callbacks in the order they were added.
     * If a callback throws an exception, it will be caught and
     * the remaining callbacks will still be executed.
     */
    void invokeAll();

    /**
     * @brief Get the number of registered callbacks.
     * @return Number of callbacks in the list
     */
    size_t size() const;

    /**
     * @brief Check if there are any registered callbacks.
     * @return true if no callbacks are registered, false otherwise
     */
    bool empty() const;

    /**
     * @brief Clear all registered callbacks (optional utility method).
     * 
     * Note: This method is provided as a utility, though the requirement
     * specified that deletion is not needed. This can be useful for
     * cleanup or testing scenarios.
     */
    void clear();
};

} // namespace Callback

class StaticCallbacks {
public:
    static Callback::Manager& poll() {
        static Callback::Manager instance;
        return instance;
    }    

    static Callback::Manager& save() {
        static Callback::Manager instance;
        return instance;
    }

    static Callback::Manager& read() {
        static Callback::Manager instance;
        return instance;
    }
};

} // namespace QaplaWindows
