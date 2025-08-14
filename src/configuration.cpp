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

#include "configuration.h"
#include "qapla-tester/logger.h"

#include <fstream>
#include <filesystem>
#include <iostream>


using namespace QaplaConfiguration;

Configuration::Configuration() 
{
}

void Configuration::saveFile() {
    namespace fs = std::filesystem;

    try {
        // Rename existing file to backup
        if (fs::exists(CONFIG_FILE)) {
            fs::rename(CONFIG_FILE, BACKUP_FILE);
        }

        // Write new configuration
        std::ofstream outFile(CONFIG_FILE, std::ios::trunc);
        if (!outFile) {
            throw std::ios_base::failure("Failed to open configuration file for writing.");
        }

        saveData(); // Call the actual save logic
        outFile.close();

        // Remove backup if new file was written successfully
        if (fs::exists(CONFIG_FILE)) {
            fs::remove(BACKUP_FILE);
        }
    }
    catch (const std::exception& e) {
        Logger::testLogger().log(std::string("Error saving configuration: ") + e.what(), TraceLevel::error);
        // Restore backup if saving failed
        if (fs::exists(BACKUP_FILE)) {
            fs::rename(BACKUP_FILE, CONFIG_FILE);
        }
    }
}

void Configuration::loadFile() {
    namespace fs = std::filesystem;

    try {
        if (fs::exists(CONFIG_FILE)) {
            // Load from the main configuration file
            loadData();
        }
        else if (fs::exists(BACKUP_FILE)) {
            // Load from the backup file if the main file is missing
            fs::rename(BACKUP_FILE, CONFIG_FILE);
            loadData();
        }
        else {
            throw std::ios_base::failure("No configuration file found.");
        }
    }
    catch (const std::exception& e) {
        Logger::testLogger().log(std::string("Error loading configuration: ") + e.what(), TraceLevel::error);
    }
}

void Configuration::saveData() {
	saveTimeControls();
}

void Configuration::loadData() {
    // Placeholder for actual load logic
}

void Configuration::saveTimeControls() {
    try {
        std::ofstream outFile(CONFIG_FILE, std::ios::app); // Append mode to add to the file
        if (!outFile) {
            throw std::ios_base::failure("Failed to open configuration file for writing.");
        }

        // Save each TimeControl setting
        outFile << "\n[timecontrol]\n";
        outFile << "name=BlitzTime";
		timeControlSettings_.blitzTime.save(outFile);
        outFile << "\n[timecontrol]\n";
        outFile << "name=TournamentTime";
		timeControlSettings_.tournamentTime.save(outFile);
        outFile << "\n[timecontrol]\n";
        outFile << "name=TimePerMove";
		timeControlSettings_.timePerMove.save(outFile);
        outFile << "\n[timecontrol]\n";
        outFile << "name=FixedDepth";
		timeControlSettings_.fixedDepth.save(outFile);
        outFile << "\n[timecontrol]\n";
        outFile << "name=NodesPerMoves\n";
		timeControlSettings_.nodesPerMove.save(outFile);

        outFile.close();
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error in saveTimeControls: ") + e.what());
    }
}

