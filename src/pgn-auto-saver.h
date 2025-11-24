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

#include "game-record-manager.h"

#include <game-record.h>

#include <string>

namespace QaplaWindows {

/**
 * @brief Automatically saves PGN games by appending them to a file.
 * 
 * This class manages automatic saving of chess games in PGN format. It:
 * - Uses Autosavable's path determination for consistent file location
 * - Appends new games to an existing file
 * - Automatically prunes old games when file contains too many games
 * - Removes the oldest games (from the beginning of the file) to maintain a reasonable size
 * 
 * Files are saved to the platform-specific config directory:
 * - Windows: %LOCALAPPDATA%/qapla-chess-gui
 * - Linux/Mac: ~/.qapla-chess-gui
 * 
 * When the number of games exceeds MAX_GAMES_BEFORE_PRUNE (900), the oldest GAMES_TO_REMOVE (100)
 * games are automatically deleted during pruning.
 * 
 * Usage example:
 * @code
 *   PgnAutoSaver& autoSaver = PgnAutoSaver::instance();
 *   
 *   // When a game finishes:
 *   autoSaver.addGame(gameRecord);  // Appends game to file
 * @endcode
 */
class PgnAutoSaver {
public:
    /**
     * @brief Maximum number of games before automatic pruning is triggered.
     */
    static constexpr size_t MAX_GAMES_BEFORE_PRUNE = 900;

    /**
     * @brief Default filename for auto-saved games.
     */
    static constexpr const char* DEFAULT_FILENAME = "auto-saved-games.pgn";

    /**
     * @brief Gets the singleton instance.
     */
    static PgnAutoSaver& instance() {
        static bool initialize = true;
        static PgnAutoSaver instance;
        if (initialize) {
            instance.checkAndPrune();
            initialize = false;
        }
        return instance;
    }

    /**
     * @brief Adds a game and appends it to the PGN file.
     * @param game The game record to add.
     */
    void addGame(const QaplaTester::GameRecord& game);

    /**
     * @brief Gets the full file path where games are saved.
     * @return Full path to the auto-save PGN file.
     */
    std::string getFilePath() const;

private:
    PgnAutoSaver() = default;
    
    /**
     * @brief Checks game count and prunes if necessary.
     */
    void checkAndPrune();

    GameRecordManager gameRecordManager_;  ///< Manager for PGN I/O operations
    std::string filename_{DEFAULT_FILENAME};  ///< Base filename
};

} // namespace QaplaWindows