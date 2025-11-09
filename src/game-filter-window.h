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
#include <string>
#include <vector>
#include <set>
#include <functional>

namespace QaplaWindows {

/**
 * @brief Window for configuring game list filters.
 * 
 * Provides UI controls for multi-selecting players, opponents,
 * game results, and termination causes.
 */
class GameFilterWindow : public EmbeddedWindow {
public:
    GameFilterWindow();

    /**
     * @brief Draws the filter configuration window.
     */
    void draw() override;

    /**
     * @brief Sets the filter data to edit.
     */
    void setFilterData(GameFilterData* filterData) { filterData_ = filterData; }

    /**
     * @brief Sets available player names from loaded games.
     */
    void setAvailablePlayers(const std::vector<std::string>& players) { 
        availablePlayers_ = players; 
    }

    /**
     * @brief Sets available opponent names from loaded games.
     */
    void setAvailableOpponents(const std::vector<std::string>& opponents) { 
        availableOpponents_ = opponents; 
    }

    /**
     * @brief Sets available game results from loaded games.
     */
    void setAvailableResults(const std::set<QaplaTester::GameResult>& results) { 
        availableResults_ = results; 
    }

    /**
     * @brief Sets available termination causes from loaded games.
     */
    void setAvailableTerminations(const std::set<QaplaTester::GameEndCause>& terminations) { 
        availableTerminations_ = terminations; 
    }

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
    void drawActiveToggle();

    /**
     * @brief Draws the player selection section.
     */
    void drawPlayerSelection();

    /**
     * @brief Draws the opponent selection section.
     */
    void drawOpponentSelection();

    /**
     * @brief Draws the game result selection section.
     */
    void drawResultSelection();

    /**
     * @brief Draws the termination cause selection section.
     */
    void drawTerminationSelection();

    /**
     * @brief Draws a multi-select list for strings.
     */
    bool drawMultiSelectList(const std::string& label, 
                            const std::vector<std::string>& items,
                            const std::set<std::string>& selected,
                            std::function<void(const std::string&)> onToggle);

    GameFilterData* filterData_ = nullptr;
    std::vector<std::string> availablePlayers_;
    std::vector<std::string> availableOpponents_;
    std::set<QaplaTester::GameResult> availableResults_;
    std::set<QaplaTester::GameEndCause> availableTerminations_;
    std::function<void()> onFilterChanged_;
};

} // namespace QaplaWindows
