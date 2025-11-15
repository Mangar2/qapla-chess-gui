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
#include <fstream>
#include <filesystem>
#include <format>

using QaplaTester::GameRecord;
using QaplaTester::PgnIO;

void GameRecordManager::load(const std::string& fileName, std::function<bool(const GameRecord&, float)> gameCallback) {
    games_ = pgnIO_.loadGames(fileName, true, gameCallback);  // Load without comments
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

size_t GameRecordManager::save(const std::string& fileName,
                                const QaplaWindows::GameFilterData& filterData,
                                std::function<void(size_t, float)> progressCallback,
                                std::function<bool()> cancelCheck) {
    const std::string& sourceFile = pgnIO_.getCurrentFileName();
    
    // Check if source and target are the same file
    if (!sourceFile.empty() && 
        std::filesystem::weakly_canonical(sourceFile) == std::filesystem::weakly_canonical(fileName)) {
        return saveToSameFile(fileName, filterData, progressCallback, cancelCheck);
    }
    
    // Check if filter is active
    bool hasFilter = filterData.hasActiveFilters();
    
    if (!hasFilter) {
        // No filtering needed - just copy
        saveWithoutFilter(fileName);
        return games_.size();
    } else {
        return saveWithFilter(fileName, filterData, progressCallback, cancelCheck);
    }
}

size_t GameRecordManager::saveToSameFile(const std::string& fileName,
                                          const QaplaWindows::GameFilterData& filterData,
                                          std::function<void(size_t, float)> progressCallback,
                                          std::function<bool()> cancelCheck) {
    // Create temporary file name
    std::filesystem::path filePath(fileName);
    std::filesystem::path tempPath = filePath;
    tempPath.replace_filename(filePath.stem().string() + ".tmp");
    
    // Save filtered games to temp file
    size_t gamesSaved = saveWithFilter(tempPath.string(), filterData, progressCallback, cancelCheck);
    
    // Check if operation was cancelled
    if (!cancelCheck || !cancelCheck()) {
        // Delete original file
        std::filesystem::remove(fileName);
        
        // Rename temp file to original name
        std::filesystem::rename(tempPath, fileName);
    } else {
        // Cancelled - remove temp file
        std::filesystem::remove(tempPath);
    }
    
    return gamesSaved;
}

void GameRecordManager::saveWithoutFilter(const std::string& fileName) {
    const std::string& sourceFile = pgnIO_.getCurrentFileName();
    if (!sourceFile.empty()) {
        std::ifstream src(sourceFile, std::ios::binary);
        std::ofstream dst(fileName, std::ios::binary);
        dst << src.rdbuf();
        src.close();
        dst.close();
    }
}

size_t GameRecordManager::saveWithFilter(const std::string& fileName,
                                          const QaplaWindows::GameFilterData& filterData,
                                          std::function<void(size_t, float)> progressCallback,
                                          std::function<bool()> cancelCheck) {
    // Open file for writing in binary mode to preserve line endings
    std::ofstream outFile(fileName, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!outFile.is_open()) {
        throw std::runtime_error(
            std::format("Failed to open file for writing: {}", fileName)
        );
    }
    
    size_t gamesSaved = 0;
    size_t totalGames = games_.size();
    
    // Save each game that passes the filter
    for (size_t i = 0; i < totalGames; ++i) {
        const auto& game = games_[i];
        
        // Check if user cancelled
        if (cancelCheck && cancelCheck()) {
            break;
        }
        
        // Apply filter
        if (!filterData.passesFilter(game)) {
            continue;
        }
        
        // Get raw game text and write it
        auto rawText = pgnIO_.getRawGameText(i);
        if (rawText) {
            outFile << *rawText;
            gamesSaved++;
        }
        
        // Update progress
        if (progressCallback) {
            progressCallback(gamesSaved, static_cast<float>(i + 1) / static_cast<float>(totalGames));
        }
    }
    
    outFile.close();
    return gamesSaved;
}
