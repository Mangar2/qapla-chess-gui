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

#include "autosavable.h"
#include "game-record-manager.h"

#include <game-record.h>

#include <vector>
#include <string>

/**
 * @brief Automatically saves PGN games with automatic pruning of old games.
 * 
 * This class manages automatic saving of chess games in PGN format. It:
 * - Uses automatic path and filename determination via Autosavable
 * - Saves games to a configurable directory (typically config directory)
 * - Automatically prunes old games when file size exceeds a threshold
 * - Removes the oldest games (from the beginning of the file) to maintain a reasonable size
 * 
 * The class inherits from Autosavable and follows the pattern used by Configuration and EpdData.
 * Files are saved to the platform-specific config directory:
 * - Windows: %LOCALAPPDATA%/qapla-chess-gui
 * - Linux/Mac: ~/.qapla-chess-gui
 * 
 * When the number of games exceeds MAX_GAMES_BEFORE_PRUNE (900), the oldest GAMES_TO_REMOVE (100)
 * games are automatically deleted during the next save operation.
 * 
 * Usage example:
 * @code
 *   PgnAutoSaver autoSaver;
 *   autoSaver.loadFile();  // Load existing games on startup
 *   
 *   // Later, when a game finishes:
 *   autoSaver.addGame(gameRecord);  // Add new game and trigger autosave
 *   
 *   // Periodic autosave (called in main loop):
 *   autoSaver.autosave();  // Saves if modified and interval elapsed
 * @endcode
 */
class PgnAutoSaver : public QaplaHelpers::Autosavable {
public:
    /**
     * @brief Maximum number of games before automatic pruning is triggered.
     */
    static constexpr size_t MAX_GAMES_BEFORE_PRUNE = 900;

    /**
     * @brief Number of oldest games to remove during pruning.
     */
    static constexpr size_t GAMES_TO_REMOVE = 100;

    /**
     * @brief Default filename for auto-saved games.
     */
    static constexpr const char* DEFAULT_FILENAME = "auto-saved-games.pgn";

    /**
     * @brief Constructor.
     * @param filename Base filename for auto-saved games (default: "auto-saved-games.pgn").
     * @param autosaveIntervalMs Auto-save interval in milliseconds (default: 60000ms = 1 minute).
     */
    PgnAutoSaver(std::string filename = DEFAULT_FILENAME,
                 uint64_t autosaveIntervalMs = 60000);

    /**
     * @brief Adds a game to the auto-save collection and triggers autosave.
     * @param game The game record to add.
     */
    void addGame(const QaplaTester::GameRecord& game);

    /**
     * @brief Gets the number of games currently stored.
     * @return Number of games.
     */
    size_t getGameCount() const { return games_.size(); }

    /**
     * @brief Gets the loaded games.
     * @return Const reference to the vector of GameRecords.
     */
    const std::vector<QaplaTester::GameRecord>& getGames() const { return games_; }

    /**
     * @brief Clears all stored games.
     */
    void clear() {
        games_.clear();
        setModified();
    }

protected:
    /**
     * @brief Saves the data to an output stream.
     * Implements the pure virtual method from Autosavable.
     * Automatically prunes old games if count exceeds MAX_GAMES_BEFORE_PRUNE.
     * @param out Output stream to write to.
     */
    void saveData(std::ofstream& out) override;

    /**
     * @brief Loads the data from an input stream.
     * Implements the pure virtual method from Autosavable.
     * @param in Input stream to read from.
     */
    void loadData(std::ifstream& in) override;

private:
    std::vector<QaplaTester::GameRecord> games_;  ///< Collection of game records
    GameRecordManager gameRecordManager_;  ///< Manager for PGN I/O operations
};
