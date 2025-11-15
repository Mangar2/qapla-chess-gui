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
#include "string-helper.h"

namespace QaplaWindows {

void GameFilterData::init(const std::string& id) {
    auto sections = QaplaConfiguration::Configuration::instance()
        .getConfigData()
        .getSectionList("gamefilter", id)
        .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!sections.empty()) {
        const auto& section = sections[0];
        
        // Load active state
        active_ = section.getValue("active").value_or("0") == "1";
        
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
        
        // Load selected results
        auto resultsStr = section.getValue("results").value_or("");
        if (!resultsStr.empty()) {
            auto resultsList = QaplaHelpers::split(resultsStr, '|');
            for (const auto& res : resultsList) {
                if (res == "1-0") selectedResults_.insert(QaplaTester::GameResult::WhiteWins);
                else if (res == "0-1") selectedResults_.insert(QaplaTester::GameResult::BlackWins);
                else if (res == "1/2-1/2") selectedResults_.insert(QaplaTester::GameResult::Draw);
                else if (res == "*") selectedResults_.insert(QaplaTester::GameResult::Unterminated);
            }
        }
        
        // Load selected terminations
        auto terminationsStr = section.getValue("terminations").value_or("");
        if (!terminationsStr.empty()) {
            auto terminationsList = QaplaHelpers::splitWithUnescape(terminationsStr, '|');
            for (const auto& term : terminationsList) {
                auto cause = QaplaTester::tryParseGameEndCause(term);
                if (cause) {
                    selectedTerminations_.insert(*cause);
                }
            }
        }
    }
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
    
    // Serialize results (no escaping needed for predefined values)
    std::string resultsStr;
    for (const auto& result : selectedResults_) {
        if (!resultsStr.empty()) resultsStr += "|";
        resultsStr += QaplaTester::gameResultToPgnResult(result);
    }
    
    // Serialize terminations (with escaping for safety)
    std::string terminationsStr;
    for (const auto& termination : selectedTerminations_) {
        if (!terminationsStr.empty()) terminationsStr += "|";
        terminationsStr += QaplaHelpers::escapeDelimiter(
            QaplaTester::gameEndCauseToPgnTermination(termination), '|');
    }
    
    QaplaHelpers::IniFile::Section section{
        .name = "gamefilter",
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", id},
            {"active", active_ ? "1" : "0"},
            {"players", playersStr},
            {"opponents", opponentsStr},
            {"results", resultsStr},
            {"terminations", terminationsStr}
        }
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

void GameFilterData::toggleResult(QaplaTester::GameResult result) {
    if (selectedResults_.count(result)) {
        selectedResults_.erase(result);
    } else {
        selectedResults_.insert(result);
    }
}

void GameFilterData::toggleTermination(QaplaTester::GameEndCause cause) {
    if (selectedTerminations_.count(cause)) {
        selectedTerminations_.erase(cause);
    } else {
        selectedTerminations_.insert(cause);
    }
}

bool GameFilterData::isPlayerSelected(const std::string& player) const {
    return selectedPlayers_.count(player) > 0;
}

bool GameFilterData::isOpponentSelected(const std::string& opponent) const {
    return selectedOpponents_.count(opponent) > 0;
}

bool GameFilterData::isResultSelected(QaplaTester::GameResult result) const {
    return selectedResults_.count(result) > 0;
}

bool GameFilterData::isTerminationSelected(QaplaTester::GameEndCause cause) const {
    return selectedTerminations_.count(cause) > 0;
}

void GameFilterData::clear() {
    selectedPlayers_.clear();
    selectedOpponents_.clear();
    selectedResults_.clear();
    selectedTerminations_.clear();
}

bool GameFilterData::hasFilters() const {
    return !selectedPlayers_.empty() || 
           !selectedOpponents_.empty() || 
           !selectedResults_.empty() || 
           !selectedTerminations_.empty();
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
    if (!passesResultFilter(result)) {
        return false;
    }

    return passesTerminationFilter(cause);
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

bool GameFilterData::passesResultFilter(QaplaTester::GameResult result) const {
    return selectedResults_.empty() || selectedResults_.contains(result);
}

bool GameFilterData::passesTerminationFilter(QaplaTester::GameEndCause cause) const {
    return selectedTerminations_.empty() || selectedTerminations_.contains(cause);
}

} // namespace QaplaWindows
