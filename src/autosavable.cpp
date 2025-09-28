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

#include "autosavable.h"
#include "qapla-tester/logger.h"
#include "qapla-tester/timer.h"

#include <filesystem>
#include <iostream>
#include <cstdlib>

using namespace QaplaHelpers;

Autosavable::Autosavable(const std::string& filename, 
                         const std::string& backupSuffix,
                         uint64_t autosaveIntervalMs,
                         std::function<std::string()> directoryProvider)
    : filename_(filename)
    , backupSuffix_(backupSuffix)
    , autosaveIntervalMs_(autosaveIntervalMs)
    , directoryProvider_(directoryProvider ? directoryProvider : defaultDirectoryProvider)
{
    updateFilePaths();
}

void Autosavable::autosave() {
    if (!changed_) return;
    
    uint64_t currentTime = Timer::getCurrentTimeMs();
    if (currentTime - lastSaveTimestamp_ < autosaveIntervalMs_) {
        return; 
    }
    
    saveFile();
    lastSaveTimestamp_ = Timer::getCurrentTimeMs();
    changed_ = false; 
}

void Autosavable::saveFile() {
    namespace fs = std::filesystem;

    try {
        // Ensure the directory exists
        std::string directory = getDirectory();
        fs::create_directories(directory);
        
        // Update file paths in case directory changed
        updateFilePaths();
        
        // Rename existing file to backup
        if (fs::exists(filePath_)) {
            fs::rename(filePath_, backupFilePath_);
        }

        // Open the file for writing
        std::ofstream outFile(filePath_, std::ios::trunc);
        if (!outFile) {
            throw std::ios_base::failure("Failed to open file for writing: " + filePath_);
        }

        // Save the data using the derived class implementation
        saveData(outFile);
        outFile.close();

        // Remove the backup file if saving was successful
        if (fs::exists(filePath_)) {
            fs::remove(backupFilePath_);
        }
    }
    catch (const std::exception& e) {
        Logger::testLogger().log(std::string("Error saving file: ") + e.what(), TraceLevel::error);

        // Restore the backup if saving failed
        if (fs::exists(backupFilePath_)) {
            fs::rename(backupFilePath_, filePath_);
        }
    }
}

void Autosavable::loadFile() {
    namespace fs = std::filesystem;

    try {
        // Ensure the directory exists
        std::string directory = getDirectory();
        fs::create_directories(directory);
        
        // Update file paths in case directory changed
        updateFilePaths();

        std::ifstream inFile;

        if (fs::exists(filePath_)) {
            inFile.open(filePath_, std::ios::in);
        }
        else if (fs::exists(backupFilePath_)) {
            // Restore from backup if main file doesn't exist
            fs::rename(backupFilePath_, filePath_);
            inFile.open(filePath_, std::ios::in);
        }
        else {
            throw std::ios_base::failure("No file found: " + filePath_);
        }

        if (!inFile.is_open()) {
            throw std::ios_base::failure("Failed to open file for reading: " + filePath_);
        }

        // Load the data using the derived class implementation
        loadData(inFile);
        inFile.close();

        lastSaveTimestamp_ = Timer::getCurrentTimeMs();
        changed_ = false;
    }
    catch (const std::exception& e) {
        Logger::testLogger().log(std::string("Cannot load file: ") + e.what(),
            TraceLevel::error);
    }
}

std::string Autosavable::getDirectory() const {
    if (directoryProvider_) {
        return directoryProvider_();
    }
    return defaultDirectoryProvider();
}

void Autosavable::updateFilePaths() {
    std::string directory = getDirectory();
    filePath_ = directory + "/" + filename_;
    backupFilePath_ = directory + "/" + filename_ + backupSuffix_;
}

std::string Autosavable::defaultDirectoryProvider() {
    return ".";  // Current directory as fallback
}

std::string Autosavable::getConfigDirectory() {
    namespace fs = std::filesystem;

#ifdef _WIN32
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, "LOCALAPPDATA") == 0 && buf) {
        std::string path(buf);
        free(buf);
        return path + "/qapla-chess-gui";
    }
    // Fallback, falls LOCALAPPDATA nicht gesetzt ist
    return std::string(".") + "/qapla-chess-gui";
#else
    return std::string(std::getenv("HOME")) + "/.qapla-chess-gui";
#endif
}