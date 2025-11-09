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

#include "game-result.h"
#include <string>
#include <set>
#include <vector>

namespace QaplaWindows {

/**
 * @brief Data structure for game list filter configuration.
 * 
 * Stores filter parameters for filtering PGN games by player names,
 * game results, and termination reasons.
 */
class GameFilterData {
public:
    GameFilterData() = default;

    /**
     * @brief Initialize filter data from configuration.
     * @param id Unique identifier for this filter configuration.
     */
    void init(const std::string& id);

    /**
     * @brief Update configuration with current filter settings.
     * @param id Unique identifier for this filter configuration.
     */
    void updateConfiguration(const std::string& id) const;

    /**
     * @brief Checks if the filter is active.
     */
    bool isActive() const { return active_; }

    /**
     * @brief Sets the filter active state.
     */
    void setActive(bool active) { active_ = active; }

    /**
     * @brief Gets the selected player names (White).
     */
    const std::set<std::string>& getSelectedPlayers() const { return selectedPlayers_; }

    /**
     * @brief Gets the selected opponent names (Black).
     */
    const std::set<std::string>& getSelectedOpponents() const { return selectedOpponents_; }

    /**
     * @brief Gets the selected game results.
     */
    const std::set<QaplaTester::GameResult>& getSelectedResults() const { return selectedResults_; }

    /**
     * @brief Gets the selected termination causes.
     */
    const std::set<QaplaTester::GameEndCause>& getSelectedTerminations() const { return selectedTerminations_; }

    /**
     * @brief Sets the selected player names.
     */
    void setSelectedPlayers(const std::set<std::string>& players) { selectedPlayers_ = players; }

    /**
     * @brief Sets the selected opponent names.
     */
    void setSelectedOpponents(const std::set<std::string>& opponents) { selectedOpponents_ = opponents; }

    /**
     * @brief Sets the selected game results.
     */
    void setSelectedResults(const std::set<QaplaTester::GameResult>& results) { selectedResults_ = results; }

    /**
     * @brief Sets the selected termination causes.
     */
    void setSelectedTerminations(const std::set<QaplaTester::GameEndCause>& terminations) { 
        selectedTerminations_ = terminations; 
    }

    /**
     * @brief Toggles a player name in the selection.
     */
    void togglePlayer(const std::string& player);

    /**
     * @brief Toggles an opponent name in the selection.
     */
    void toggleOpponent(const std::string& opponent);

    /**
     * @brief Toggles a game result in the selection.
     */
    void toggleResult(QaplaTester::GameResult result);

    /**
     * @brief Toggles a termination cause in the selection.
     */
    void toggleTermination(QaplaTester::GameEndCause cause);

    /**
     * @brief Checks if a player is selected.
     */
    bool isPlayerSelected(const std::string& player) const;

    /**
     * @brief Checks if an opponent is selected.
     */
    bool isOpponentSelected(const std::string& opponent) const;

    /**
     * @brief Checks if a result is selected.
     */
    bool isResultSelected(QaplaTester::GameResult result) const;

    /**
     * @brief Checks if a termination cause is selected.
     */
    bool isTerminationSelected(QaplaTester::GameEndCause cause) const;

    /**
     * @brief Clears all filter selections.
     */
    void clear();

    /**
     * @brief Checks if any filter is applied (besides active flag).
     */
    bool hasFilters() const;

private:
    bool active_ = false;
    std::set<std::string> selectedPlayers_;
    std::set<std::string> selectedOpponents_;
    std::set<QaplaTester::GameResult> selectedResults_;
    std::set<QaplaTester::GameEndCause> selectedTerminations_;
};

} // namespace QaplaWindows
