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

#include "game-manager-pool.h"
#include <memory>

/**
 * @class GameManagerPoolAccess
 * @brief Provides access to a GameManagerPool instance, either via a shared_ptr or the singleton.
 */
class GameManagerPoolAccess {
public:
    /**
     * @brief Default constructor - will use singleton instance.
     */
    GameManagerPoolAccess() = default;

    /**
     * @brief Constructor with explicit pool instance.
     * @param pool Shared pointer to a GameManagerPool instance.
     */
    explicit GameManagerPoolAccess(std::shared_ptr<QaplaTester::GameManagerPool> pool)
        : pool_(std::move(pool)) {}

    /**
     * @brief Get a raw pointer to the GameManagerPool.
     * @return Pointer to the pool instance (either from shared_ptr or singleton).
     */
    [[nodiscard]] QaplaTester::GameManagerPool* get() const {
        return pool_ ? pool_.get() : &QaplaTester::GameManagerPool::getInstance();
    }

    /**
     * @brief Arrow operator for convenient access to pool methods.
     * @return Pointer to the pool instance.
     */
    [[nodiscard]] QaplaTester::GameManagerPool* operator->() const {
        return get();
    }

    /**
     * @brief Dereference operator for convenient access.
     * @return Reference to the pool instance.
     */
    [[nodiscard]] QaplaTester::GameManagerPool& operator*() const {
        return *get();
    }

private:
    std::shared_ptr<QaplaTester::GameManagerPool> pool_; ///< Optional pool instance; if nullptr, uses singleton.
};