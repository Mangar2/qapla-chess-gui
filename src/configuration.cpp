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
#include "qapla-tester/string-helper.h"
#include "qapla-tester/timer.h"
#include "qapla-tester/time-control.h"

#include <fstream>
#include <filesystem>
#include <iostream>


using namespace QaplaConfiguration;

Configuration::Configuration() 
{
}

void Configuration::autosave() {
	static const uint64_t AUTOSAVE_INTERVAL_MS = 60000; // 60 seconds
    if (!changed_) return;
	uint64_t currentTime = Timer::getCurrentTimeMs();
    if (currentTime - lastSaveTimestamp_ < AUTOSAVE_INTERVAL_MS) {
        return; 
	}
    saveFile();
	lastSaveTimestamp_ = Timer::getCurrentTimeMs();
    changed_ = false; 
}

void Configuration::saveFile() {
    namespace fs = std::filesystem;

    try {
        // Rename existing file to backup
        if (fs::exists(CONFIG_FILE)) {
            fs::rename(CONFIG_FILE, BACKUP_FILE);
        }

        std::ofstream outFile(CONFIG_FILE, std::ios::trunc);
        if (!outFile) {
            throw std::ios_base::failure("Failed to open configuration file for writing.");
        }

        saveData(outFile); 
        outFile.close();

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
        std::ifstream inFile;

        if (fs::exists(CONFIG_FILE)) {
            // Open the main configuration file
            inFile.open(CONFIG_FILE, std::ios::in);
        }
        else if (fs::exists(BACKUP_FILE)) {
            // Rename and open the backup file if the main file is missing
            fs::rename(BACKUP_FILE, CONFIG_FILE);
            inFile.open(CONFIG_FILE, std::ios::in);
        }
        else {
            throw std::ios_base::failure("No configuration file found.");
        }

        if (!inFile.is_open()) {
            throw std::ios_base::failure("Failed to open configuration file for reading.");
        }

        // Pass the opened file stream to loadData
        loadData(inFile);

        // Close the file stream after loading
        inFile.close();
		lastSaveTimestamp_ = Timer::getCurrentTimeMs();
		changed_ = false; 
    }
    catch (const std::exception& e) {
        Logger::testLogger().log(std::string("Error loading configuration: ") + e.what(), TraceLevel::error);
    }
}

void Configuration::saveData(std::ofstream& out) {
	saveTimeControls(out);
}

void Configuration::loadData(std::ifstream& in) {
    try {
        std::string line;

        while (auto sectionHeader = readSectionHeader(in)) {
            std::map<std::string, std::string> keyValueMap;
            while (in && in.peek() != '[' && std::getline(in, line)) {
				line = trim(line);
                if (line.empty() || line[0] == '#' || line[0] == ';') continue;
                auto keyValue = parseKeyValue(line);
                if (keyValue) {
                    keyValueMap[keyValue->first] = keyValue->second;
                }
            }
            processSection(*sectionHeader, keyValueMap);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error in loadData: ") + e.what());
    }
}

void Configuration::saveTimeControls(std::ofstream& out) {
    static uint64_t saveCnt = 0;
    saveCnt++;
    try {
        out << "#" << saveCnt << "\n";
		out << "\n[board]\n";
		out << "timecontrol=" << timeControlSettings_.getSelectionString() << "\n";
        out << "\n[timecontrol]\n";
        out << "name=BlitzTime\n";
        saveMap(out, timeControlSettings_.blitzTime.toMap());
        out << "\n[timecontrol]\n";
        out << "name=TournamentTime\n";
		saveMap(out, timeControlSettings_.tournamentTime.toMap());
        out << "\n[timecontrol]\n";
        out << "name=TimePerMove\n";
		saveMap(out, timeControlSettings_.timePerMove.toMap());
        out << "\n[timecontrol]\n";
        out << "name=FixedDepth\n";
		saveMap(out, timeControlSettings_.fixedDepth.toMap());
        out << "\n[timecontrol]\n";
        out << "name=NodesPerMoves\n";
		saveMap(out, timeControlSettings_.nodesPerMove.toMap());
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error in saveTimeControls: ") + e.what());
    }
}

void Configuration::saveMap(std::ofstream& outFile, const std::map<std::string, std::string>& map) {
    try {
        for (const auto& [key, value] : map) {
            outFile << key << "=" << value << "\n";
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error in saveMap: ") + e.what());
    }
}

void Configuration::processSection(const std::string& section, const std::map<std::string, std::string>& keyValueMap) {
    try {
        if (section == "timecontrol") {
            parseTimeControl(keyValueMap);
        }
        else if (section == "board") {
            parseBoard(keyValueMap);
        }
        else if (section == "engine") {
            // Engine section processing can be added here in the future
            Logger::testLogger().log("Engine section processing not implemented yet", TraceLevel::info);
        }
        else if (section == "tournament") {
            // Tournament section processing can be added here in the future
            Logger::testLogger().log("Tournament section processing not implemented yet", TraceLevel::info);
		}
        else {
            Logger::testLogger().log("Unknown section: " + section, TraceLevel::warning);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error processing section [" + section + "]: " + e.what());
    }
}

void Configuration::parseTimeControl(const std::map<std::string, std::string>& keyValueMap) {
    try {
        auto it = keyValueMap.find("name");
        if (it == keyValueMap.end()) {
			Logger::testLogger().log("Missing 'name' in timecontrol section", TraceLevel::error);
            return;
        }

        const std::string& name = it->second;

        if (name == "BlitzTime") {
            timeControlSettings_.blitzTime.fromMap(keyValueMap);
        }
        else if (name == "TournamentTime") {
            timeControlSettings_.tournamentTime.fromMap(keyValueMap);
        }
        else if (name == "TimePerMove") {
            timeControlSettings_.timePerMove.fromMap(keyValueMap);
        }
        else if (name == "FixedDepth") {
            timeControlSettings_.fixedDepth.fromMap(keyValueMap);
        }
        else if (name == "NodesPerMove") {
            timeControlSettings_.nodesPerMove.fromMap(keyValueMap);
        }
        else {
            Logger::testLogger().log("Unknown timecontrol name: " + name, TraceLevel::warning);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error parsing timecontrol: ") + e.what());
    }
}

void Configuration::parseBoard(const std::map<std::string, std::string>& keyValueMap) {
    try {
        auto it = keyValueMap.find("timecontrol");
        if (it == keyValueMap.end()) {
            return;
        }
        const std::string& timeControl = it->second;
        timeControlSettings_.setSelectionFromString(timeControl);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error parsing board section: ") + e.what());
    }
}
