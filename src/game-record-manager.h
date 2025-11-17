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

#include "game-record.h"
#include "pgn-io.h"
#include "game-filter-data.h"

#include <string>
#include <vector>
#include <functional>

namespace QaplaWindows {
    class GameFilterData;
}

/**
 * @brief Manages a collection of GameRecords loaded from PGN files.
 */
class GameRecordManager {
public:
    GameRecordManager() = default;

    /**
     * @brief Loads games from a PGN file using PgnIO.
     * @param fileName Name of the PGN file to load.
     * @param gameCallback Optional callback function called for each loaded game.
     */
    void load(const std::string& fileName, std::function<bool(const QaplaTester::GameRecord&, float)> gameCallback = nullptr);

    /**
     * @brief Gets the loaded games.
     * @return Const reference to the vector of GameRecords.
     */
    [[nodiscard]] const std::vector<QaplaTester::GameRecord>& getGames() const { return games_; }

    /**
     * @brief Gets the game positions from the last loaded file.
     * @return Const reference to the vector of stream positions.
     */
    [[nodiscard]] const std::vector<std::streampos>& getGamePositions() const { return pgnIO_.getGamePositions(); }

    /**
     * @brief Gets the most common PGN header tag names present in all loaded games.
     * @param topN Number of top tags to return (default: 10).
     * @return Vector of pairs containing tag name and occurrence count, sorted by count descending.
     */
    [[nodiscard]] std::vector<std::pair<std::string, size_t>> getMostCommonTags(size_t topN = 10) const;

     /**
     * @brief Loads a specific game by index from the previously loaded file.
     * @param index Index of the game to load.
     * @return Optional GameRecord if successful.
     */
    [[nodiscard]] std::optional<QaplaTester::GameRecord> loadGameByIndex(size_t index);

    /**
     * @brief Gets the raw PGN text of a specific game by index.
     * @param index Index of the game to retrieve.
     * @return Optional string containing the raw PGN text if successful.
     */
    [[nodiscard]] std::optional<std::string> getRawGameText(size_t index);

    /**
     * @brief Gets the filename of the currently loaded PGN file.
     * @return Reference to the current filename string.
     */
    [[nodiscard]] const std::string& getCurrentFileName() const { return pgnIO_.getCurrentFileName(); }

    /**
     * @brief Appends a single game to an existing PGN file.
     * @param fileName Target filename to append to.
     * @param game Game record to append.
     */
    void appendGame(const std::string& fileName, const QaplaTester::GameRecord& game);

    /**
     * @brief Prunes old games from the beginning of a PGN file, keeping only the most recent games.
     * Creates a temporary file with the pruned games, then replaces the original.
     * @param fileName Filename to prune.
     * @param maxGames Maximum number of games allowed before pruning is triggered.
     */
    void pruneOldGames(const std::string& fileName, size_t maxGames);

    /**
     * @brief Saves games to a file, handling special cases like same-file save.
     * @param fileName Target filename to save to.
     * @param filterData Filter configuration to apply.
     * @param progressCallback Callback for progress updates (gamesProcessed, progress 0-1).
     * @param cancelCheck Function to check if operation should be cancelled.
     * @return Number of games saved.
     */
    [[nodiscard]] size_t save(const std::string& fileName,
                const QaplaWindows::GameFilterData& filterData,
                std::function<void(size_t, float)> progressCallback,
                std::function<bool()> cancelCheck);

private:
    /**
     * @brief Saves games to the same file (uses temporary file).
     * @param fileName Target filename.
     * @param filterData Filter configuration to apply.
     * @param progressCallback Callback for progress updates.
     * @param cancelCheck Function to check if operation should be cancelled.
     * @return Number of games saved.
     */
    [[nodiscard]] size_t saveToSameFile(const std::string& fileName,
                          const QaplaWindows::GameFilterData& filterData,
                          std::function<void(size_t, float)> progressCallback,
                          std::function<bool()> cancelCheck);

    /**
     * @brief Copies the entire file without filtering.
     * @param fileName Target filename.
     */
    void saveWithoutFilter(const std::string& fileName);

    /**
     * @brief Saves filtered games to a file.
     * @param fileName Target filename.
     * @param filterData Filter configuration to apply.
     * @param progressCallback Callback for progress updates.
     * @param cancelCheck Function to check if operation should be cancelled.
     * @return Number of games saved.
     */
    [[nodiscard]] size_t saveWithFilter(const std::string& fileName,
                          const QaplaWindows::GameFilterData& filterData,
                          std::function<void(size_t, float)> progressCallback,
                          std::function<bool()> cancelCheck);

    std::vector<QaplaTester::GameRecord> games_;  // Loaded game records
    QaplaTester::PgnIO pgnIO_;  // PGN I/O handler
};