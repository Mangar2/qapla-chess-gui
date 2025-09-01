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

#include "qapla-tester/game-manager-pool.h"

#include <thread>
#include <atomic>

/**
 * @class ImGuiConcurrency
 * @brief Handles concurrency updates via ImGui slider and ensures the GameManagerPool is updated accordingly.
 */
class ImGuiConcurrency {
public:
    /**
     * @brief Constructor for ImGuiConcurrency.
     * @param initialConcurrency The initial concurrency value.
     */
    ImGuiConcurrency() = default;

    /**
     * @brief Initializes the ImGuiConcurrency object.
     */
    void init() {
        currentConcurrency_ = 0;
        targetConcurrency_ = 0;
        debounceCounter_ = 0;
    }

    /**
     * @brief Updates the concurrency value based on user input.
     * @param newConcurrency The new concurrency value from the ImGui slider.
     */
    void update(uint32_t newConcurrency) {
        if (!active_) return;

        if (newConcurrency != targetConcurrency_) {
            targetConcurrency_ = newConcurrency;
            debounceCounter_ = 10; // Reset debounce counter
        }

        if (debounceCounter_ > 0) {
            --debounceCounter_;
            if (debounceCounter_ == 0) {
                startThreadIfNeeded();
            }
        }
    }

    /**
     * @brief Sets the nice stop flag. When nice stop is true, 
     * games will be played until its end.
     * @param niceStop The new value for the nice stop flag.
     */
    void setNiceStop(bool niceStop) {
        niceStop_ = niceStop;
    }

    /**
     * @brief Sets the active flag.
     * @param active The new value for the active flag.
     */
    void setActive(bool active) {
        active_ = active;
    }

private:
    std::atomic<bool> active_ = false;  ///< Whether the concurrency control is active.
    bool niceStop_ = true;  ///< Whether to finish games or abort them.
    uint32_t currentConcurrency_ = 0; ///< Tracks the current concurrency value.
    std::atomic<uint32_t> targetConcurrency_ = 0;   ///< Tracks the target concurrency value.
    int debounceCounter_;         ///< Counter for debouncing slider changes.

    /**
     * @brief Starts a thread to handle increasing concurrency if needed.
     */
    void startThreadIfNeeded() {
        if (currentConcurrency_ == targetConcurrency_) {
            return; 
        }
        std::thread([this]() {
            auto& pool = GameManagerPool::getInstance();

            while (currentConcurrency_ < targetConcurrency_ && active_) {
                ++currentConcurrency_;
                pool.setConcurrency(currentConcurrency_, niceStop_, true);
            }

            if (currentConcurrency_ > targetConcurrency_ && active_) {
                currentConcurrency_ = targetConcurrency_;
                pool.setConcurrency(currentConcurrency_, niceStop_, true);
            }
        }).detach();
    }
};
