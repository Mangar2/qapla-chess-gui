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
#include "qapla-tester/engine-worker-factory.h"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>


using namespace QaplaConfiguration;

Configuration::Configuration() 
{
}

static std::string getConfigDirectory() {
    namespace fs = std::filesystem;

    // Fallback: Benutzerverzeichnis verwenden
#ifdef _WIN32
    return std::string(std::getenv("LOCALAPPDATA")) + "/qapla-chess-gui";
#else
    return std::string(std::getenv("HOME")) + "/.qapla-chess-gui";
#endif
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
        if (fs::exists(configFilePath_)) {
            fs::rename(configFilePath_, backupFilePath_);
        }

        // Open the configuration file for writing
        std::ofstream outFile(configFilePath_, std::ios::trunc);
        if (!outFile) {
            throw std::ios_base::failure("Failed to open configuration file for writing.");
        }

        // Save the configuration data
        saveData(outFile);
        outFile.close();

        // Remove the backup file if saving was successful
        if (fs::exists(configFilePath_)) {
            fs::remove(backupFilePath_);
        }
    }
    catch (const std::exception& e) {
        Logger::testLogger().log(std::string("Error saving configuration: ") + e.what(), TraceLevel::error);

        // Restore the backup if saving failed
        if (fs::exists(backupFilePath_)) {
            fs::rename(backupFilePath_, configFilePath_);
        }
    }
}

void Configuration::loadFile() {
    namespace fs = std::filesystem;

    try {
        const std::string configDir = getConfigDirectory();
        fs::create_directories(configDir);

        configFilePath_ = configDir + "/" + CONFIG_FILE;
		backupFilePath_ = configDir + "/" + BACKUP_FILE;

        std::ifstream inFile;

        if (fs::exists(configFilePath_)) {
            inFile.open(configFilePath_, std::ios::in);
        }
        else if (fs::exists(backupFilePath_)) {
            fs::rename(backupFilePath_, configFilePath_);
            inFile.open(configFilePath_, std::ios::in);
        }
        else {
            throw std::ios_base::failure("No configuration file found.");
        }

        if (!inFile.is_open()) {
            throw std::ios_base::failure("Failed to open configuration file for reading.");
        }

        loadData(inFile);
        inFile.close();

        lastSaveTimestamp_ = Timer::getCurrentTimeMs();
        changed_ = false;
    }
    catch (const std::exception& e) {
        Logger::testLogger().log(std::string("Error loading configuration: ") + e.what(), 
            TraceLevel::error);
    }
}

void Configuration::saveData(std::ofstream& out) {
	saveTimeControls(out);
	EngineWorkerFactory::getConfigManager().saveToStream(out);
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
		out << "[board]\n";
		out << "timecontrol=" << timeControlSettings_.getSelectionString() << "\n";
        out << "\n";
        out << "[timecontrol]\n";
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
        out << "\n";
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
            EngineConfig config;
			config.setValues(keyValueMap);
			EngineWorkerFactory::getConfigManagerMutable().addOrReplaceConfig(config);
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
