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

#include "game-filter-data.h"
#include "configuration.h"
#include <base-elements/string-helper.h>
#include <algorithm>

namespace QaplaWindows {

// Define static member
const std::vector<std::string> GameFilterData::emptyVector_;

void GameFilterData::init(const std::string& id) {
    auto sections = QaplaConfiguration::Configuration::instance()
        .getConfigData()
        .getSectionList("gamefilter", id)
        .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!sections.empty()) {
        const auto& section = sections[0];
        
        // We deactivate the filter by default as there is no pgn loaded after starting the application
        active_ = false;
        
        // Load selected players (with unescaping)
        auto playersStr = section.getValue("players").value_or("");
        if (!playersStr.empty()) {
            auto playersList = QaplaHelpers::splitWithUnescape(playersStr, '|');
            selectedPlayers_ = std::set<std::string>(playersList.begin(), playersList.end());
        }
        
        // Load selected opponents (with unescaping)
        auto opponentsStr = section.getValue("opponents").value_or("");
        if (!opponentsStr.empty()) {
            auto opponentsList = QaplaHelpers::splitWithUnescape(opponentsStr, '|');
            selectedOpponents_ = std::set<std::string>(opponentsList.begin(), opponentsList.end());
        }

        std::vector<std::string> loadTopics = { "results", "causes", "terminations" };

        for (const auto& topic : loadTopics) {
            auto topicStr = section.getValue(topic).value_or("");
            if (!topicStr.empty()) {
                auto topicList = QaplaHelpers::splitWithUnescape(topicStr, '|');
                selectedOptions_[topic] = std::set<std::string>(topicList.begin(), topicList.end());
            }
        }

    }
}

void GameFilterData::setActive(bool active) {
    if (active && !active_) {
        // When activating, cleanup selections that are no longer valid
        cleanupSelections();
    }
    active_ = active;
}

void GameFilterData::updateConfiguration(const std::string& id) const {
    // Serialize players (with escaping)
    std::string playersStr;
    for (const auto& player : selectedPlayers_) {
        if (!playersStr.empty()) playersStr += "|";
        playersStr += QaplaHelpers::escapeDelimiter(player, '|');
    }
    
    // Serialize opponents (with escaping)
    std::string opponentsStr;
    for (const auto& opponent : selectedOpponents_) {
        if (!opponentsStr.empty()) opponentsStr += "|";
        opponentsStr += QaplaHelpers::escapeDelimiter(opponent, '|');
    }
    
    QaplaHelpers::IniFile::KeyValueMap entries{
        {"id", id},
        {"players", playersStr},
        {"opponents", opponentsStr}
    };
    
    // Serialize all generic topics
    for (const auto& [topic, options] : selectedOptions_) {
        std::string optionsStr;
        for (const auto& option : options) {
            if (!optionsStr.empty()) optionsStr += "|";
            optionsStr += QaplaHelpers::escapeDelimiter(option, '|');
        }
        entries.push_back({topic, optionsStr});
    }
    
    QaplaHelpers::IniFile::Section section{
        .name = "gamefilter",
        .entries = entries
    };
    
    QaplaConfiguration::Configuration::instance()
        .getConfigData()
        .setSectionList("gamefilter", id, {section});
}

void GameFilterData::togglePlayer(const std::string& player) {
    if (selectedPlayers_.count(player)) {
        selectedPlayers_.erase(player);
    } else {
        selectedPlayers_.insert(player);
    }
}

void GameFilterData::toggleOpponent(const std::string& opponent) {
    if (selectedOpponents_.count(opponent)) {
        selectedOpponents_.erase(opponent);
    } else {
        selectedOpponents_.insert(opponent);
    }
}

void GameFilterData::toggleOption(const std::string& topic, const std::string& option) {
    if (selectedOptions_[topic].count(option)) {
        selectedOptions_[topic].erase(option);
    } else {
        selectedOptions_[topic].insert(option);
    }
}

bool GameFilterData::isPlayerSelected(const std::string& player) const {
    return selectedPlayers_.count(player) > 0;
}

bool GameFilterData::isOpponentSelected(const std::string& opponent) const {
    return selectedOpponents_.count(opponent) > 0;
}

bool GameFilterData::isOptionSelected(const std::string& topic, const std::string& option) const {
    auto it = selectedOptions_.find(topic);
    if (it == selectedOptions_.end()) {
        return false;
    }
    return it->second.count(option) > 0;
}

std::set<std::string> GameFilterData::getSelectedOptions(const std::string& topic) const {
    auto it = selectedOptions_.find(topic);
    if (it == selectedOptions_.end()) {
        return std::set<std::string>();
    }
    return it->second;
}

void GameFilterData::setSelectedOptions(const std::string& topic, const std::set<std::string>& options) {
    selectedOptions_[topic] = options;
}

const std::vector<std::string>& GameFilterData::getAvailableOptions(const std::string& topic) const {
    auto it = availableOptions_.find(topic);
    if (it == availableOptions_.end()) {
        return emptyVector_;
    }
    return it->second;
}

void GameFilterData::clear() {
    selectedPlayers_.clear();
    selectedOpponents_.clear();
    selectedOptions_.clear();
}

bool GameFilterData::hasActiveFilters() const {
    if (!active_) {
        return false;
    }
    
    if (!selectedPlayers_.empty() || !selectedOpponents_.empty()) {
        return true;
    }
    
    for (const auto& [topic, options] : selectedOptions_) {
        if (!options.empty()) {
            return true;
        }
    }
    
    return false;
}

bool GameFilterData::passesFilter(const QaplaTester::GameRecord& game) const {
    if (!active_) {
        return true;
    }

    const auto& tags = game.getTags();
    auto whiteIt = tags.find("White");
    std::string white = (whiteIt != tags.end()) ? whiteIt->second : "";
    auto blackIt = tags.find("Black");
    std::string black = (blackIt != tags.end()) ? blackIt->second : "";

    if (!passesPlayerNamesFilter(white, black)) {
        return false;
    }

    auto [cause, result] = game.getGameResult();
    std::string resultStr = to_string(result);
    std::string causeStr = to_string(cause);
    
    if (!passesTopicFilter("results", resultStr)) {
        return false;
    }
    if (!passesTopicFilter("causes", causeStr)) {
        return false;
    }

    // Get Termination tag from PGN
    auto terminationIt = tags.find("Termination");
    std::string termination = (terminationIt != tags.end()) ? terminationIt->second : "";
    
    return passesTopicFilter("terminations", termination);
}

bool GameFilterData::passesPlayerNamesFilter(const std::string& white, const std::string& black) const {
    if (selectedPlayers_.empty() && selectedOpponents_.empty()) {
        return true;
    }

    bool playerMatch = selectedPlayers_.empty() ||
                      selectedPlayers_.contains(white) ||
                      selectedPlayers_.contains(black);

    bool opponentMatch = selectedOpponents_.empty() ||
                        selectedOpponents_.contains(white) ||
                        selectedOpponents_.contains(black);

    if (!selectedPlayers_.empty() && !selectedOpponents_.empty()) {
        bool whitePlayerBlackOpponent = selectedPlayers_.contains(white) && selectedOpponents_.contains(black);
        bool blackPlayerWhiteOpponent = selectedPlayers_.contains(black) && selectedOpponents_.contains(white);
        return whitePlayerBlackOpponent || blackPlayerWhiteOpponent;
    }

    return playerMatch && opponentMatch;
}

bool GameFilterData::passesTopicFilter(const std::string& topic, const std::string& value) const {
    auto it = selectedOptions_.find(topic);
    if (it == selectedOptions_.end() || it->second.empty()) {
        return true;
    }
    return it->second.contains(value);
}

void GameFilterData::updateAvailableOptions(const std::vector<QaplaTester::GameRecord>& games) {
    if (games.empty()) {
        return;
    }

    // Extract unique player names (both White and Black)
    std::set<std::string> uniqueNames;
    
    // Map for all topic options
    std::map<std::string, std::set<std::string>> uniqueOptions;

    for (const auto& game : games) {
        const auto& tags = game.getTags();
        
        // Extract both White and Black player names
        auto whiteIt = tags.find("White");
        if (whiteIt != tags.end() && !whiteIt->second.empty()) {
            uniqueNames.insert(whiteIt->second);
        }
        
        auto blackIt = tags.find("Black");
        if (blackIt != tags.end() && !blackIt->second.empty()) {
            uniqueNames.insert(blackIt->second);
        }
        
        // Extract game result as string
        auto [cause, result] = game.getGameResult();
        uniqueOptions["results"].insert(to_string(result));
        uniqueOptions["causes"].insert(to_string(cause));
        
        // Extract termination from PGN tag
        auto terminationIt = tags.find("Termination");
        if (terminationIt != tags.end() && !terminationIt->second.empty()) {
            uniqueOptions["terminations"].insert(terminationIt->second);
        }
    }

    // Convert sets to vectors and sort
    availableNames_ = std::vector<std::string>(uniqueNames.begin(), uniqueNames.end());
    std::ranges::sort(availableNames_);
    
    // Convert all topics to sorted vectors
    for (const auto& [topic, optionsSet] : uniqueOptions) {
        availableOptions_[topic] = std::vector<std::string>(optionsSet.begin(), optionsSet.end());
        std::ranges::sort(availableOptions_[topic]);
    }
}

void GameFilterData::cleanupSelections() {
    // Convert availableNames_ to set for faster lookup
    std::set<std::string> availableNamesSet(availableNames_.begin(), availableNames_.end());
    
    // Cleanup players - keep only those in availableNames
    std::set<std::string> cleanedPlayers;
    for (const auto& player : selectedPlayers_) {
        if (availableNamesSet.contains(player)) {
            cleanedPlayers.insert(player);
        }
    }
    selectedPlayers_ = std::move(cleanedPlayers);
    
    // Cleanup opponents - keep only those in availableNames
    std::set<std::string> cleanedOpponents;
    for (const auto& opponent : selectedOpponents_) {
        if (availableNamesSet.contains(opponent)) {
            cleanedOpponents.insert(opponent);
        }
    }
    selectedOpponents_ = std::move(cleanedOpponents);
    
    // Cleanup all generic topics
    for (auto& [topic, selectedSet] : selectedOptions_) {
        auto availIt = availableOptions_.find(topic);
        if (availIt == availableOptions_.end()) {
            selectedSet.clear();
            continue;
        }
        
        std::set<std::string> availableSet(availIt->second.begin(), availIt->second.end());
        std::set<std::string> cleanedSet;
        for (const auto& option : selectedSet) {
            if (availableSet.contains(option)) {
                cleanedSet.insert(option);
            }
        }
        selectedSet = std::move(cleanedSet);
    }
}

} // namespace QaplaWindows
