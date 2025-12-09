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

#include <string>
#include <fstream>
#include <functional>

#include "callback-manager.h"

namespace QaplaHelpers {

    /**
     * @brief Base class for objects that support automatic saving with backup and recovery functionality.
     * 
     * This class provides a generic framework for handling objects that:
     * - Need periodic auto-saving when modified
     * - Require backup/recovery mechanisms for safety
     * - Can have customizable directory structures
     * - Support different file formats through virtual methods
     * 
     * Classes inheriting from Autosavable must implement:
     * - saveData(std::ofstream&): Write data to the output stream
     * - loadData(std::ifstream&): Read data from the input stream
     */
    class Autosavable {
    public:
        /**
         * @brief Constructor for Autosavable.
         * @param filename The base filename (without path).
         * @param backupSuffix Suffix for the backup file (default: ".bak").
         * @param autosaveIntervalMs Auto-save interval in milliseconds (default: 60000ms = 1 minute).
         * @param directoryProvider Function that returns the directory path for files.
         */
        Autosavable(std::string filename, 
                    std::string backupSuffix = ".bak",
                    uint64_t autosaveIntervalMs = 60000,
                    const std::function<std::string()>& directoryProvider = nullptr);

        /**
         * @brief Virtual destructor.
         */
        virtual ~Autosavable() = default;

        /**
         * @brief Autosaves the file if it has changed since the last save and enough 
         * time has passed since last save.
         */
        virtual void autosave();

        /**
         * @brief Saves the file with a safety mechanism (backup and restore on failure).
         */
        void saveFile();

        /**
         * @brief Loads the file with a fallback mechanism (tries backup if main file fails).
         */
        void loadFile();

        /**
         * @brief Marks the file as modified, triggering autosave when conditions are met.
         */
        void setModified() {
            modified_ = true;
        }

        /**
         * @brief Checks if the file has been modified since the last save.
         * @return True if the file has been modified, false otherwise.
         */
        bool isModified() const {
            return modified_;
        }

        /**
         * @brief Gets the full path to the main file.
         * @return The full file path.
         */
        const std::string& getFilePath() const {
            return filePath_;
        }

        /**
         * @brief Gets the full path to the backup file.
         * @return The full backup file path.
         */
        const std::string& getBackupFilePath() const {
            return backupFilePath_;
        }

        /**
         * @brief Sets a custom directory provider function.
         * @param directoryProvider Function that returns the directory path.
         */
        void setDirectoryProvider(std::function<std::string()> directoryProvider) {
            directoryProvider_ = directoryProvider;
            updateFilePaths();
        }

        /**
         * @brief Sets the auto-save interval.
         * @param intervalMs Auto-save interval in milliseconds.
         */
        void setAutosaveInterval(uint64_t intervalMs) {
            autosaveIntervalMs_ = intervalMs;
        }

    protected:
        /**
         * @brief Pure virtual method to save data to an output stream.
         * Derived classes must implement this method to write their data.
         * @param out The output stream to write data to.
         */
        virtual void saveData(std::ofstream& out) = 0;

        /**
         * @brief Pure virtual method to load data from an input stream.
         * Derived classes must implement this method to read their data.
         * @param in The input stream to read data from.
         */
        virtual void loadData(std::ifstream& in) = 0;

        /**
         * @brief Gets the directory where files should be stored.
         * Uses the directory provider if set, otherwise falls back to current directory.
         * @return The directory path.
         */
        std::string getDirectory() const;

        /**
         * @brief Updates the file paths based on current directory and filename settings.
         */
        void updateFilePaths();

        /**
         * @brief Updates the file paths based on a specific file path.
         * @param filePath The full file path to use for updating.
         */
        void updateFilePaths(const std::string& filePath);

        /**
         * @brief Configuration-specific directory provider.
         * Returns platform-specific configuration directory for qapla-chess-gui.
         * @return Configuration directory path.
         */
        static std::string getConfigDirectory();

        /**
         * @brief Helper method to attempt loading data from a specific file.
         * @param filepath The file path to load from.
         * @return True if loading was successful, false otherwise.
         */
        bool tryLoadFromFile(const std::string& filepath);

        /**
         * @brief Determines if backup file should be preferred over main file.
         * Checks if backup exists and if main file is missing, empty, or suspiciously small.
         * @return True if backup should be used, false otherwise.
         */
        bool shouldPreferBackup() const;

        /**
         * @brief Restores backup file to main file and attempts to load it.
         * @return True if backup was successfully restored and loaded, false otherwise.
         */
        bool restoreAndLoadBackup();

    private:
        std::string filename_;                      ///< Base filename without path
        std::string backupSuffix_;                  ///< Suffix for backup files
        std::string filePath_;                      ///< Full path to the main file
        std::string backupFilePath_;                ///< Full path to the backup file
        
        bool modified_ = false;                      ///< Flag indicating if data has been modified
        uint64_t lastSaveTimestamp_ = 0;            ///< Timestamp of last save operation
        uint64_t autosaveIntervalMs_;               ///< Auto-save interval in milliseconds
        
        std::function<std::string()> directoryProvider_;  ///< Function to get the directory path

        std::unique_ptr<QaplaWindows::Callback::UnregisterHandle> unregisterHandle_;  ///< Handle to unregister autosave callback

        /**
         * @brief Minimum file size ratio to consider main file valid.
         * If main file is smaller than (backup size * this ratio), use backup instead.
         * Default is 0.5 (50%) - adjust to 0.9 for stricter validation (90%).
         */
        static constexpr double MIN_VALID_FILE_SIZE_RATIO = 0.9;

        /**
         * @brief Default directory provider that returns the current directory.
         * @return Current directory path.
         */
        static std::string defaultDirectoryProvider();


    };

} // namespace QaplaHelpers