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

#include <pgn-io.h>

#include <fstream>
#include <filesystem>

PgnAutoSaver::PgnAutoSaver(std::string filename, uint64_t autosaveIntervalMs)
    : Autosavable(std::move(filename), ".bak", autosaveIntervalMs, 
                  []() { return Autosavable::getConfigDirectory(); })
{
}

void PgnAutoSaver::addGame(const QaplaTester::GameRecord& game) {
    games_.push_back(game);
    setModified();
    autosave();
}

void PgnAutoSaver::saveData(std::ofstream& out) {
    // Check if pruning is needed
    std::vector<QaplaTester::GameRecord> gamesToSave;
    
    if (games_.size() > MAX_GAMES_BEFORE_PRUNE) {
        // Keep only the most recent games (remove oldest GAMES_TO_REMOVE)
        gamesToSave.assign(games_.begin() + GAMES_TO_REMOVE, games_.end());
        
        // Update the internal collection to match what we're saving
        games_ = gamesToSave;
    } else {
        gamesToSave = games_;
    }

    // Save games using PgnIO
    QaplaTester::PgnIO pgnIO;
    for (const auto& game : gamesToSave) {
        pgnIO.saveGameToStream(out, game);
    }
}

void PgnAutoSaver::loadData(std::ifstream& in) {
    // Close the ifstream and use PgnIO to load from the file path
    // PgnIO needs the filename, not the stream
    if (!in.is_open()) {
        return;
    }
    
    // Get the filename from the autosavable path
    std::string fileName = getFilePath();
    in.close();
    
    // Check if file exists and is not empty
    if (!std::filesystem::exists(fileName) || std::filesystem::file_size(fileName) == 0) {
        games_.clear();
        return;
    }
    
    // Load games using GameRecordManager
    try {
        gameRecordManager_.load(fileName);
        games_ = gameRecordManager_.getGames();
    } catch (const std::exception&) {
        // If loading fails, start with empty collection
        games_.clear();
    }
}
