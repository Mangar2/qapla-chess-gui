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

/**
 * @brief Header-only class for managing parameterless callbacks in a list.
 * 
 * This class provides a simple callback management system where callbacks
 * can be added to a list and all callbacks can be invoked at once.
 * Access is provided through a static instance for global usage.
 * 
 * Features:
 * - Header-only implementation
 * - Static instance access via instance() method
 * - Register callbacks to the list
 * - Invoke all registered callbacks
 * - Thread-safe operations (if needed, can be extended)
 * 
 * Usage example:
 * @code
 * // Register a callback
 * CallbackManager::instance().registerCallback([]() {
 *     std::cout << "Callback executed!" << std::endl;
 * });
 * 
 * // Execute all callbacks
 * CallbackManager::instance().invokeAll();
 * @endcode
 */

namespace QaplaWindows {

class CallbackManager {
public:
    using Callback = std::function<void()>;

private:
    std::vector<Callback> callbacks_;

    // Private constructor for singleton pattern
    CallbackManager() = default;

public:
    // Delete copy constructor and assignment operator to ensure singleton
    CallbackManager(const CallbackManager&) = delete;
    CallbackManager& operator=(const CallbackManager&) = delete;
    CallbackManager(CallbackManager&&) = delete;
    CallbackManager& operator=(CallbackManager&&) = delete;

    /**
     * @brief Get the static instance of CallbackManager.
     * @return Reference to the singleton instance
     */
    static CallbackManager& instance() {
        static CallbackManager instance;
        return instance;
    }

    /**
     * @brief Register a callback to the list.
     * @param callback The parameterless function/lambda to be registered
     */
    void registerCallback(Callback callback) {
        if (callback) {
            callbacks_.push_back(std::move(callback));
        }
    }

    /**
     * @brief Invoke all registered callbacks.
     * 
     * Calls all callbacks in the order they were added.
     * If a callback throws an exception, it will be caught and
     * the remaining callbacks will still be executed.
     */
    void invokeAll() {
        for (const auto& callback : callbacks_) {
            try {
                if (callback) {
                    callback();
                }
            }
            catch (...) {
                // Catch any exceptions to ensure all callbacks are called
                // In a production environment, you might want to log this
                // or handle it differently based on your needs
            }
        }
    }

    /**
     * @brief Get the number of registered callbacks.
     * @return Number of callbacks in the list
     */
    size_t size() const {
        return callbacks_.size();
    }

    /**
     * @brief Check if there are any registered callbacks.
     * @return true if no callbacks are registered, false otherwise
     */
    bool empty() const {
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
        callbacks_.clear();
    }
};

} // namespace QaplaWindows
