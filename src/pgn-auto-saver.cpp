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

#include "pgn-auto-saver.h"
#include "os-dialogs.h"

#include <filesystem>

namespace QaplaWindows {

std::string PgnAutoSaver::getFilePath() const {
    // Get config directory
    std::string directory = OsDialogs::getConfigDirectory();
    
    // Create directory if it doesn't exist
    if (!directory.empty() && !std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
    }
    
    // Combine directory and filename
    std::filesystem::path path(directory);
    path /= filename_;
    return path.string();
}

void PgnAutoSaver::addGame(const QaplaTester::GameRecord& game) {
    std::string filePath = getFilePath();
    
    // Append game to file
    gameRecordManager_.appendGame(filePath, game);

}

void PgnAutoSaver::checkAndPrune() {
    std::string filePath = getFilePath();
    
    // Check if file exists
    if (!std::filesystem::exists(filePath)) {
        return;
    }
    
    // Prune old games if file has too many
    gameRecordManager_.pruneOldGames(filePath, MAX_GAMES_BEFORE_PRUNE);
}

} // namespace QaplaWindows