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

#include <game-record.h>

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
     * @brief Gets the selected termination strings (from PGN Termination tag).
     */
    const std::set<std::string>& getSelectedTerminations() const { return selectedTerminations_; }

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
     * @brief Sets the selected termination strings.
     */
    void setSelectedTerminations(const std::set<std::string>& terminations) { 
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
     * @brief Toggles a termination string in the selection.
     */
    void toggleTermination(const std::string& termination);

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
     * @brief Checks if a termination string is selected.
     */
    bool isTerminationSelected(const std::string& termination) const;

    /**
     * @brief Clears all filter selections.
     */
    void clear();

    /**
     * @brief Checks if filter is active AND any filter criteria are set.
     * @return true if filter is enabled and has filter criteria configured.
     */
    bool hasActiveFilters() const;

    /**
     * @brief Checks if a game passes the current filter settings.
     * @param game The game record to check
     * @return true if the game passes all active filters, false otherwise
     */
    bool passesFilter(const QaplaTester::GameRecord& game) const;

private:
    /**
     * @brief Checks if a game passes the player names filter.
     * @param white White player name
     * @param black Black player name
     * @return true if the game passes the player filter
     */
    bool passesPlayerNamesFilter(const std::string& white, const std::string& black) const;

    /**
     * @brief Checks if a game passes the result filter.
     * @param result The game result
     * @return true if the game passes the result filter
     */
    bool passesResultFilter(QaplaTester::GameResult result) const;

    /**
     * @brief Checks if a game passes the termination filter.
     * @param termination The PGN Termination tag value
     * @return true if the game passes the termination filter
     */
    bool passesTerminationFilter(const std::string& termination) const;

private:
    bool active_ = false;
    std::set<std::string> selectedPlayers_;
    std::set<std::string> selectedOpponents_;
    std::set<QaplaTester::GameResult> selectedResults_;
    std::set<std::string> selectedTerminations_;
};

} // namespace QaplaWindows
