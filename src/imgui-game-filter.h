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

#include "game-filter-data.h"
#include "game-filter-window.h"
#include "imgui-popup.h"
#include <game-record.h>
#include <functional>
#include <vector>
#include <set>

namespace QaplaWindows {

/**
 * @brief Manages game filtering UI and data.
 * 
 * This class encapsulates the filter popup window, filter data,
 * and provides methods for filtering game records.
 */
class ImGuiGameFilter {
public:
    /**
     * @brief Constructs a new ImGuiGameFilter.
     */
    ImGuiGameFilter();

    /**
     * @brief Initializes filter data from configuration.
     * @param configId Configuration identifier for loading/saving filter settings.
     */
    void init(const std::string& configId);

    /**
     * @brief Draws the filter popup if open.
     */
    void draw();

    /**
     * @brief Opens the filter popup.
     */
    void open();

    /**
     * @brief Checks if the filter popup was confirmed (OK clicked).
     * @return std::optional<bool> true if OK clicked, false if cancelled, nullopt if no action.
     */
    std::optional<bool> confirmed();

    /**
     * @brief Resets the confirmation state.
     */
    void resetConfirmation();

    /**
     * @brief Updates available filter options from loaded games.
     * @param games Vector of game records to extract filter options from.
     */
    void updateFilterOptions(const std::vector<QaplaTester::GameRecord>& games);

    /**
     * @brief Saves current filter configuration.
     * @param configId Configuration identifier for saving filter settings.
     */
    void updateConfiguration(const std::string& configId) const;

    /**
     * @brief Checks if a game passes the current filter.
     * @param game The game record to check.
     * @return true if the game passes all active filters, false otherwise.
     */
    bool passesFilter(const QaplaTester::GameRecord& game) const;

    /**
     * @brief Gets the filter data.
     * @return Reference to the filter data.
     */
    GameFilterData& getFilterData() { return filterData_; }

    /**
     * @brief Gets the filter data (const).
     * @return Const reference to the filter data.
     */
    const GameFilterData& getFilterData() const { return filterData_; }

    /**
     * @brief Sets a callback to be called when filter changes.
     * @param callback Function to call when filter is modified.
     */
    void setOnFilterChangedCallback(std::function<void()> callback);

private:
    /**
     * @brief Filter data.
     */
    GameFilterData filterData_;

    /**
     * @brief Popup window for filter configuration.
     */
    ImGuiPopup<GameFilterWindow> filterPopup_;

    /**
     * @brief Configuration ID for persistent storage.
     */
    std::string configId_;
};

} // namespace QaplaWindows
