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

#include "embedded-window.h"
#include "game-filter-data.h"
#include "game-result.h"
#include <game-record.h>
#include <string>
#include <vector>
#include <set>
#include <functional>

namespace QaplaWindows {

/**
 * @brief Window for configuring game list filters.
 * 
 * Owns and manages GameFilterData, provides UI controls for multi-selecting
 * players, opponents, game results, and termination causes.
 */
class GameFilterWindow : public EmbeddedWindow {
public:
    GameFilterWindow();

    /**
     * @brief Initializes filter data from configuration.
     * @param configId Configuration identifier for loading/saving filter settings.
     */
    void init(const std::string& configId);

    /**
     * @brief Draws the filter configuration window.
     */
    void draw() override;

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
     * @brief Sets a callback to be called when filter settings change.
     */
    void setOnFilterChangedCallback(std::function<void()> callback) {
        onFilterChanged_ = callback;
    }

private:
    /**
     * @brief Draws the active/inactive toggle.
     */
    bool drawActiveToggle();

    /**
     * @brief Draws the player selection section.
     */
    bool drawPlayerSelection();

    /**
     * @brief Draws the opponent selection section.
     */
    bool drawOpponentSelection();

    /**
     * @brief Draws the game result selection section.
     */
    bool drawResultSelection();

    /**
     * @brief Draws the termination cause selection section.
     */
    bool drawTerminationSelection();

    /**
     * @brief Draws a name selection section (helper for player/opponent).
     */
    bool drawNameSelection(const std::string& tooltip,
                          const std::vector<std::string>& availableNames,
                          std::function<bool(const std::string&)> isSelected,
                          std::function<void(const std::string&)> onToggle,
                          std::function<void()> onClear);

    /**
     * @brief Draws a multi-select list for strings.
     */
    bool drawMultiSelectList(const std::string& label, 
                            const std::vector<std::string>& items,
                            const std::set<std::string>& selected,
                            std::function<void(const std::string&)> onToggle);

    GameFilterData filterData_;  // Owner of filter data
    std::vector<std::string> availablePlayers_;
    std::vector<std::string> availableOpponents_;
    std::set<QaplaTester::GameResult> availableResults_;
    std::vector<std::string> availableTerminations_;
    std::function<void()> onFilterChanged_;
};

} // namespace QaplaWindows
