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

#include "snackbar.h"

#include "game-manager-pool-access.h"

#include <thread>
#include <atomic>
#include <utility>

/**
 * @class ImGuiConcurrency
 * @brief Handles concurrency updates via ImGui slider and ensures the GameManagerPool is updated accordingly.
 */
class ImGuiConcurrency {
public:
    /**
     * @brief Constructor for ImGuiConcurrency.
     * @param poolAccess Access to the GameManagerPool (default: uses singleton).
     */
    explicit ImGuiConcurrency(GameManagerPoolAccess poolAccess = GameManagerPoolAccess())
        : poolAccess_(std::move(poolAccess)) {}

    /**
     * @brief Initializes the ImGuiConcurrency object.
     */
    void init() {
        currentConcurrency_ = 0;
        targetConcurrency_ = 0;
        debounceCounter_ = 0;
    }

    /**
     * @brief Sets the GameManagerPoolAccess instance.
     * @param poolAccess Access to the GameManagerPool instance.
     */
    void setPoolAccess(GameManagerPoolAccess poolAccess) {
        poolAccess_ = std::move(poolAccess);
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

        if (newConcurrency == 0) {
            adjustConcurrency();
            return;
        }

        if (debounceCounter_ > 0) {
            --debounceCounter_;
            if (debounceCounter_ == 0) {
                adjustConcurrency();
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
    GameManagerPoolAccess poolAccess_; ///< Access to the GameManagerPool instance.
    std::atomic<bool> active_ = false;  ///< Whether the concurrency control is active.
    bool niceStop_ = true;  ///< Whether to finish games or abort them.
    uint32_t currentConcurrency_ = 0; ///< Tracks the current concurrency value.
    std::atomic<uint32_t> targetConcurrency_ = 0;   ///< Tracks the target concurrency value.
    int debounceCounter_;         ///< Counter for debouncing slider changes.

    /**
     * @brief Starts a thread to handle increasing concurrency if needed.
     */
    void adjustConcurrency() {

        if (currentConcurrency_ == targetConcurrency_) {
            return; 
        }
        if (currentConcurrency_ > targetConcurrency_ && active_) {
            currentConcurrency_ = targetConcurrency_;
            poolAccess_->setConcurrency(currentConcurrency_, niceStop_, true);
            return;
        }
        std::thread([this]() {
            try {
                while (currentConcurrency_ < targetConcurrency_ && active_) {
                    ++currentConcurrency_;
                    poolAccess_->setConcurrency(currentConcurrency_, niceStop_, true);
                }
            }
            catch (const std::exception& e) {
                // Log the exception if needed
                SnackbarManager::instance().showError(std::string(e.what()));
            }

        }).detach();
    }
};
