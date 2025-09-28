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

namespace QaplaHelpers {

    /**
     * @brief Base class for files that need automatic saving with backup and recovery functionality.
     * 
     * This class provides a generic framework for handling files that:
     * - Need periodic auto-saving when modified
     * - Require backup/recovery mechanisms for safety
     * - Can have customizable directory structures
     * - Support different file formats through virtual methods
     * 
     * Classes inheriting from AutoSaveFile must implement:
     * - saveData(std::ofstream&): Write data to the output stream
     * - loadData(std::ifstream&): Read data from the input stream
     */
    class AutoSaveFile {
    public:
        /**
         * @brief Constructor for AutoSaveFile.
         * @param filename The base filename (without path).
         * @param backupSuffix Suffix for the backup file (default: ".bak").
         * @param autosaveIntervalMs Auto-save interval in milliseconds (default: 60000ms = 1 minute).
         * @param directoryProvider Function that returns the directory path for files.
         */
        AutoSaveFile(const std::string& filename, 
                     const std::string& backupSuffix = ".bak",
                     uint64_t autosaveIntervalMs = 60000,
                     std::function<std::string()> directoryProvider = nullptr);

        /**
         * @brief Virtual destructor.
         */
        virtual ~AutoSaveFile() = default;

        /**
         * @brief Autosaves the file if it has changed since the last save and enough 
         * time has passed since last save.
         */
        void autosave();

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
            changed_ = true;
        }

        /**
         * @brief Checks if the file has been modified since the last save.
         * @return True if the file has been modified, false otherwise.
         */
        bool isModified() const {
            return changed_;
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
         * @brief Configuration-specific directory provider.
         * Returns platform-specific configuration directory for qapla-chess-gui.
         * @return Configuration directory path.
         */
        static std::string getConfigDirectory();

    private:
        std::string filename_;                      ///< Base filename without path
        std::string backupSuffix_;                  ///< Suffix for backup files
        std::string filePath_;                      ///< Full path to the main file
        std::string backupFilePath_;                ///< Full path to the backup file
        
        bool changed_ = false;                      ///< Flag indicating if data has been modified
        uint64_t lastSaveTimestamp_ = 0;            ///< Timestamp of last save operation
        uint64_t autosaveIntervalMs_;               ///< Auto-save interval in milliseconds
        
        std::function<std::string()> directoryProvider_;  ///< Function to get the directory path

        /**
         * @brief Default directory provider that returns the current directory.
         * @return Current directory path.
         */
        static std::string defaultDirectoryProvider();


    };

} // namespace QaplaHelpers