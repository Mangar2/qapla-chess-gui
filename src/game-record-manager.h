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

#include "qapla-tester/game-record.h"
#include "qapla-tester/pgn-io.h"

#include <string>
#include <vector>
#include <functional>


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
    void load(const std::string& fileName, std::function<bool(const GameRecord&)> gameCallback = nullptr);

    /**
     * @brief Gets the loaded games.
     * @return Const reference to the vector of GameRecords.
     */
    const std::vector<GameRecord>& getGames() const { return games_; }

    /**
     * @brief Gets the game positions from the last loaded file.
     * @return Const reference to the vector of stream positions.
     */
    const std::vector<std::streampos>& getGamePositions() const { return pgnIO_.getGamePositions(); }

    /**
     * @brief Gets the most common PGN header tag names present in all loaded games.
     * @param topN Number of top tags to return (default: 10).
     * @return Vector of pairs containing tag name and occurrence count, sorted by count descending.
     */
    std::vector<std::pair<std::string, size_t>> getMostCommonTags(size_t topN = 10) const;

    /**
     * @brief Gets a specific game by index.
     * @param index Index of the game to retrieve.
     * @return Const reference to the GameRecord at the specified index.
     * @throws std::out_of_range if index is invalid.
     */
    const GameRecord& getGame(size_t index) const { return games_.at(index); }

private:
    std::vector<GameRecord> games_;  // Loaded game records
    PgnIO pgnIO_;  // PGN I/O handler
};