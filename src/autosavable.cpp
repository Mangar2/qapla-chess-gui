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
using QaplaTester::Logger;
using QaplaTester::TraceLevel;

Autosavable::Autosavable(std::string filename, 
                         std::string backupSuffix,
                         uint64_t autosaveIntervalMs,
                         const std::function<std::string()>& directoryProvider)
    : filename_(std::move(filename))
    , backupSuffix_(std::move(backupSuffix))
    , autosaveIntervalMs_(autosaveIntervalMs)
    , directoryProvider_(directoryProvider ? directoryProvider : defaultDirectoryProvider)
{
    updateFilePaths();
}

void Autosavable::autosave() {
    if (!modified_) {
         return;
    }
    
    uint64_t currentTime = Timer::getCurrentTimeMs();
    if (currentTime - lastSaveTimestamp_ < autosaveIntervalMs_) {
        return; 
    }
    
    saveFile();
    lastSaveTimestamp_ = Timer::getCurrentTimeMs();
    modified_ = false; 
}

void Autosavable::saveFile() {
    namespace fs = std::filesystem;

    try {
        // Ensure the directory exists
        std::string directory = getDirectory();

        if (!directory.empty()) {
            fs::create_directories(directory);

            // Update file paths in case directory changed
            updateFilePaths();
        }
        
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
        
        // Ensure all data is written to disk
        outFile.flush();
        
        // Check if the write operation was successful
        if (!outFile.good()) {
            throw std::ios_base::failure("Failed to write data to file: " + filePath_);
        }
        
        outFile.close();
        
        // Verify file exists and has content before removing backup
        if (!fs::exists(filePath_)) {
            throw std::ios_base::failure("File does not exist after writing: " + filePath_);
        }
        
        // Check if file has reasonable size (not empty)
        if (fs::file_size(filePath_) == 0) {
            throw std::ios_base::failure("File is empty after writing: " + filePath_);
        }
        
        // Only now remove the backup file since saving was successful
        if (fs::exists(backupFilePath_)) {
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

bool Autosavable::tryLoadFromFile(const std::string& filepath) {
    namespace fs = std::filesystem;
    
    try {
        std::ifstream inFile(filepath, std::ios::in);
        if (!inFile.is_open()) {
            return false;
        }
        
        loadData(inFile);
        inFile.close();
        
        lastSaveTimestamp_ = Timer::getCurrentTimeMs();
        modified_ = false;
        return true;
    }
    catch (const std::exception& e) {
        Logger::testLogger().log(std::string("Failed to load from ") + filepath + ": " + e.what(),
            TraceLevel::error);
        return false;
    }
}

bool Autosavable::shouldPreferBackup() const {
    namespace fs = std::filesystem;
    
    if (!fs::exists(backupFilePath_)) {
        return false;
    }
    
    Logger::testLogger().log("Warning: Backup file exists, indicating potential save failure: " + backupFilePath_, 
        TraceLevel::warning);
    
    if (!fs::exists(filePath_)) {
        Logger::testLogger().log("Main file missing, using backup", TraceLevel::warning);
        return true;
    }
    
    // Both exist: check if main file is suspiciously small/empty
    auto mainSize = fs::file_size(filePath_);
    auto backupSize = fs::file_size(backupFilePath_);
    
    if (mainSize == 0) {
        Logger::testLogger().log("Main file is empty, using backup", TraceLevel::warning);
        return true;
    }
    
    if (mainSize < static_cast<uintmax_t>(backupSize * MIN_VALID_FILE_SIZE_RATIO)) {
        Logger::testLogger().log("Main file is significantly smaller than backup (ratio: " + 
            std::to_string(MIN_VALID_FILE_SIZE_RATIO) + "), using backup", TraceLevel::warning);
        return true;
    }
    
    Logger::testLogger().log("Main file size looks valid, attempting to load it (backup available as fallback)", 
        TraceLevel::info);
    return false;
}

bool Autosavable::restoreAndLoadBackup() {
    namespace fs = std::filesystem;
    
    if (!fs::exists(backupFilePath_)) {
        return false;
    }
    
    Logger::testLogger().log("Restoring from backup: " + backupFilePath_, TraceLevel::info);
    
    // Remove corrupted main file if it exists
    if (fs::exists(filePath_)) {
        fs::remove(filePath_);
    }
    
    // Rename backup to main file
    fs::rename(backupFilePath_, filePath_);
    
    // Try to load
    return tryLoadFromFile(filePath_);
}

void Autosavable::loadFile() {
    namespace fs = std::filesystem;

    std::string directory = getDirectory();
    if (!directory.empty()) {
        updateFilePaths();
    }

    // Decide which file to try first based on file state
    if (shouldPreferBackup()) {
        // Backup looks better than main file
        if (restoreAndLoadBackup()) {
            return; 
        }
        Logger::testLogger().log("Backup load failed", TraceLevel::error);
        return;
    }

    if (fs::exists(filePath_)) {
        if (tryLoadFromFile(filePath_)) {
            return; 
        }
        
        Logger::testLogger().log("Main file failed to load, attempting backup recovery", TraceLevel::warning);
        if (restoreAndLoadBackup()) {
            Logger::testLogger().log("Successfully recovered from backup", TraceLevel::info);
            return;
        }
        
        Logger::testLogger().log("Backup recovery also failed, no ini file loaded", TraceLevel::error);
        return;
    }
    
    // Neither main file nor backup exists
    Logger::testLogger().log("No file found: " + filePath_, TraceLevel::error);
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

void Autosavable::updateFilePaths(const std::string& filePath) {
    namespace fs = std::filesystem;
    fs::path path(filePath);
    filename_ = path.filename().string();
    std::string directory = path.parent_path().string();
    if (directory.empty()) {
        directory = getDirectory();
    }
    filePath_ = (fs::path(directory) / path.filename()).string();
    backupFilePath_ = (fs::path(directory) / (path.filename().string() + backupSuffix_)).string();
}

std::string Autosavable::defaultDirectoryProvider() {
    return "";  // No directory as fallback
}

std::string Autosavable::getConfigDirectory() {
    namespace fs = std::filesystem;

#ifdef _WIN32
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, "LOCALAPPDATA") == 0 && buf != nullptr) {
        std::string path(buf);
        free(buf);
        return path + "/qapla-chess-gui";
    }
    // Fallback, if LOCALAPPDATA is not set
    return std::string(".") + "/qapla-chess-gui";
#else
    return std::string(std::getenv("HOME")) + "/.qapla-chess-gui";
#endif
}