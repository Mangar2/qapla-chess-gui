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
#include <map>

namespace QaplaWindows {

/**
 * @brief Data structure for game list filter configuration.
 * 
 * Stores filter parameters for filtering PGN games by player names,
 * game results, and termination reasons.
 * Uses a generic map-based approach for extensible filtering options.
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
     * When activating, cleans up selections that are no longer in available options.
     */
    void setActive(bool active);

    /**
     * @brief Gets the selected player names (White).
     */
    const std::set<std::string>& getSelectedPlayers() const { return selectedPlayers_; }

    /**
     * @brief Gets the selected opponent names (Black).
     */
    const std::set<std::string>& getSelectedOpponents() const { return selectedOpponents_; }

    /**
     * @brief Gets the selected options for a specific topic.
     * @param topic The filter topic (e.g., "results", "terminations")
     * @return Set of selected option strings for the topic
     */
    std::set<std::string> getSelectedOptions(const std::string& topic) const;

    /**
     * @brief Sets the selected player names.
     */
    void setSelectedPlayers(const std::set<std::string>& players) { selectedPlayers_ = players; }

    /**
     * @brief Sets the selected opponent names.
     */
    void setSelectedOpponents(const std::set<std::string>& opponents) { selectedOpponents_ = opponents; }

    /**
     * @brief Sets the selected options for a specific topic.
     * @param topic The filter topic
     * @param options Set of option strings to select
     */
    void setSelectedOptions(const std::string& topic, const std::set<std::string>& options);

    /**
     * @brief Toggles a player name in the selection.
     */
    void togglePlayer(const std::string& player);

    /**
     * @brief Toggles an opponent name in the selection.
     */
    void toggleOpponent(const std::string& opponent);

    /**
     * @brief Toggles an option for a specific topic in the selection.
     * @param topic The filter topic
     * @param option The option string to toggle
     */
    void toggleOption(const std::string& topic, const std::string& option);

    /**
     * @brief Checks if a player is selected.
     */
    bool isPlayerSelected(const std::string& player) const;

    /**
     * @brief Checks if an opponent is selected.
     */
    bool isOpponentSelected(const std::string& opponent) const;

    /**
     * @brief Checks if an option for a specific topic is selected.
     * @param topic The filter topic
     * @param option The option string to check
     * @return true if the option is selected
     */
    bool isOptionSelected(const std::string& topic, const std::string& option) const;

    /**
     * @brief Gets the available names (players/opponents).
     */
    const std::vector<std::string>& getAvailableNames() const { return availableNames_; }

    /**
     * @brief Gets the available options for a specific topic.
     * @param topic The filter topic
     * @return Vector of available option strings for the topic
     */
    const std::vector<std::string>& getAvailableOptions(const std::string& topic) const;

    /**
     * @brief Updates available filter options from loaded games.
     * @param games Vector of game records to extract filter options from.
     */
    void updateAvailableOptions(const std::vector<QaplaTester::GameRecord>& games);

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
     * @brief Checks if a game passes a generic topic filter.
     * @param topic The filter topic
     * @param value The value to check against the filter
     * @return true if the game passes the topic filter
     */
    bool passesTopicFilter(const std::string& topic, const std::string& value) const;

    /**
     * @brief Cleans up selections that are no longer in available options.
     */
    void cleanupSelections();

private:
    bool active_ = false;
    
    // Special handling for players and opponents (bidirectional matching)
    std::set<std::string> selectedPlayers_;
    std::set<std::string> selectedOpponents_;
    std::vector<std::string> availableNames_;
    
    // Generic topic-based filtering
    std::map<std::string, std::set<std::string>> selectedOptions_;
    std::map<std::string, std::vector<std::string>> availableOptions_;
    
    // Empty vector for returning when topic not found
    static const std::vector<std::string> emptyVector_;
};

} // namespace QaplaWindows
