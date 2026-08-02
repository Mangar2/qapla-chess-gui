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

constexpr int DEBOUNCE_FRAMES = 10;

/**
 * @class ImGuiConcurrency
 * @brief Handles concurrency updates via ImGui slider and ensures the GameManagerPool is updated accordingly.
 *
 * Deliberately separates two different notions of "concurrency" that earlier versions of this
 * class conflated into a single field:
 * - externalConcurrency_ ("configured"): the user's target setting, e.g. what a slider or the
 *   AI's configure_tournament/configure_sprt/configure_epd tools set. Always >= 1 and survives
 *   start/stop/init unchanged -- it's what the user asked for, not what's currently happening.
 * - currentConcurrency_ ("active"): how many games the pool is actually told to run right now.
 *   Legitimately 0 while stopped/idle.
 * Call stop() (not update(0)) to make the pool stop running games -- it zeroes only the active
 * side, leaving the configured value untouched so it's still there the next time a run starts.
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
     * @brief Initializes the ImGuiConcurrency object for a new run.
     *
     * Deliberately leaves externalConcurrency_ (the configured value) untouched -- called from
     * every mode's start(), it must not reset what the user already configured.
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
     * @param direct If true, applies the change immediately without debouncing.
     */
    void update(uint32_t newConcurrency, bool direct = false) {
        if (!active_) return;
        setExternalConcurrency(newConcurrency);

        if (newConcurrency != targetConcurrency_) {
            targetConcurrency_ = newConcurrency;
            debounceCounter_ = DEBOUNCE_FRAMES; // Reset debounce counter
        }

        if (newConcurrency == 0 || direct) {
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
     * @brief Tells the pool to stop running games (active concurrency 0).
     *
     * Deliberately separate from update(0): that call would also overwrite the user's
     * configured concurrency down to the clamp floor via setExternalConcurrency() (the exact
     * "current vs. configured" bug this class used to have -- stopping a tournament/SPRT/EPD
     * run silently reset its configured concurrency instead of leaving it for the next run).
     * stop() only zeroes the active side; getExternalConcurrency() keeps returning whatever the
     * user last configured.
     */
    void stop() {
        if (!active_) return;
        currentConcurrency_ = 0;
        targetConcurrency_ = 0;
        debounceCounter_ = 0;
        poolAccess_->setConcurrency(0, niceStop_, true);
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

    /**
     * @brief Gets the configured (external) concurrency value.
     * @return The configured concurrency value. Always >= 1, even before any explicit
     *         configuration (see externalConcurrency_'s default) or after a stop.
     */
    [[nodiscard]] uint32_t getExternalConcurrency() const {
        return externalConcurrency_;
    }

    /**
     * @brief Sets the external concurrency value.
     * @param count The new external concurrency value.
     */
    void setExternalConcurrency(uint32_t count) {
        externalConcurrency_ = std::clamp(count, 1U, 32U);
    }

private:
    GameManagerPoolAccess poolAccess_; ///< Access to the GameManagerPool instance.
    bool active_ = false;  ///< Whether the concurrency control is active.
    bool niceStop_ = true;  ///< Whether to finish games or abort them.
    uint32_t currentConcurrency_ = 0;  ///< Tracks the current (active) concurrency value.
    uint32_t targetConcurrency_ = 0;   ///< Tracks the target concurrency value for debouncing.
    // Defaults to 1, not 0: getExternalConcurrency()'s "always >= 1" invariant must hold even
    // before setExternalConcurrency()/update() is ever called (e.g. status queried before the
    // user has configured or started anything this session) -- previously defaulted to 0, which
    // broke that invariant for every reader until something happened to call the setter first.
    uint32_t externalConcurrency_ = 1;       ///< Tracks the configured (not current) concurrency.
    int debounceCounter_;         ///< Counter for debouncing slider changes.

    /**
     * @brief Starts a thread to handle increasing concurrency if needed.
     */
    void adjustConcurrency() {
        if (!active_) {
            return;
        }
        if (currentConcurrency_ == targetConcurrency_) {
            return; 
        }
        currentConcurrency_ = targetConcurrency_;
        poolAccess_->setConcurrency(currentConcurrency_, niceStop_, true);
    }
};
