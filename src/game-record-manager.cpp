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

#include "game-record-manager.h"
#include <algorithm>

using QaplaTester::GameRecord;
using QaplaTester::PgnIO;

void GameRecordManager::load(const std::string& fileName, std::function<bool(const GameRecord&, float)> gameCallback) {
    games_ = pgnIO_.loadGames(fileName, false, gameCallback);  // Load without comments
}

std::optional<GameRecord> GameRecordManager::loadGameByIndex(size_t index) {
    return pgnIO_.loadGameAtIndex(index);
}

std::optional<std::string> GameRecordManager::getRawGameText(size_t index) {
    return pgnIO_.getRawGameText(index);
}

std::vector<std::pair<std::string, size_t>> GameRecordManager::getMostCommonTags(size_t topN) const {
    std::map<std::string, size_t> tagCounts;
    
    // Count occurrences of each tag across all games
    for (const auto& game : games_) {
        const auto& tags = game.getTags();
        for (const auto& tag : tags) {
            tagCounts[tag.first]++;
        }
    }
    
    // Convert to vector of pairs and sort by count descending
    std::vector<std::pair<std::string, size_t>> result(tagCounts.begin(), tagCounts.end());
    std::sort(result.begin(), result.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return top N
    if (result.size() > topN) {
        result.resize(topN);
    }
    
    return result;
}