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
#include "tournament-data.h"
#include "interactive-board-window.h"

#include "qapla-tester/logger.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/ini-file.h"
#include "qapla-tester/timer.h"
#include "qapla-tester/time-control.h"
#include "qapla-tester/engine-worker-factory.h"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <unordered_map>


using namespace QaplaConfiguration;

Configuration::Configuration() 
{
}

static std::string getConfigDirectory() {
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
	engineCapabilities_.save(out);
	EngineWorkerFactory::getConfigManager().saveToStream(out);
    QaplaWindows::InteractiveBoardWindow::instance().saveConfig(out);
	QaplaWindows::TournamentData::instance().saveConfig(out);
}

void Configuration::loadData(std::ifstream& in) {
    try {
        auto sectionList = QaplaHelpers::IniFile::load(in);
        std::string line;

        for (const auto& section : sectionList) {
            processSection(section);
        }
        QaplaWindows::TournamentData::instance().loadConfig(sectionList);
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

        QaplaHelpers::IniFile::saveSection(out, timeControlSettings_.blitzTime.toSection("BlitzTime"));
        QaplaHelpers::IniFile::saveSection(out, timeControlSettings_.tournamentTime.toSection("TournamentTime"));
        QaplaHelpers::IniFile::saveSection(out, timeControlSettings_.timePerMove.toSection("TimePerMove"));
        QaplaHelpers::IniFile::saveSection(out, timeControlSettings_.fixedDepth.toSection("FixedDepth"));
        QaplaHelpers::IniFile::saveSection(out, timeControlSettings_.nodesPerMove.toSection("NodesPerMoves"));

    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error in saveTimeControls: ") + e.what());
    }
}

void Configuration::processSection(const QaplaHelpers::IniFile::Section& section) {
    const auto& sectionName = section.name;
    const auto& entries = section.entries;
    try {
        if (sectionName == "timecontrol") {
            parseTimeControl(section);
        }
        else if (sectionName == "board") {
            parseBoard(section);
        }
        else if (sectionName == "boardengine") {
            QaplaWindows::InteractiveBoardWindow::instance().loadBoardEngine(section);
        }
        else if (sectionName == "enginecapability") {
            engineCapabilities_.addOrReplace(section);
        }
        else if (sectionName == "engine") {
            EngineConfig config;
            config.setValues(section.getUnorderedMap());
            EngineWorkerFactory::getConfigManagerMutable().addOrReplaceConfig(config);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error processing section [" + sectionName + "]: " + e.what());
    }
}

void Configuration::parseTimeControl(const QaplaHelpers::IniFile::Section& section) {
    try {
        auto nameOpt = section.getValue("name");
        if (!nameOpt) {
			Logger::testLogger().log("Missing 'name' in timecontrol section", TraceLevel::error);
            return;
        }

        const std::string& name = *nameOpt;

        if (name == "BlitzTime") {
            timeControlSettings_.blitzTime.fromSection(section);
        }
        else if (name == "TournamentTime") {
            timeControlSettings_.tournamentTime.fromSection(section);
        }
        else if (name == "TimePerMove") {
            timeControlSettings_.timePerMove.fromSection(section);
        }
        else if (name == "FixedDepth") {
            timeControlSettings_.fixedDepth.fromSection(section);
        }
        else if (name == "NodesPerMove") {
            timeControlSettings_.nodesPerMove.fromSection(section);
        }
        else {
            Logger::testLogger().log("Unknown timecontrol name: " + name, TraceLevel::warning);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error parsing timecontrol: ") + e.what());
    }
}

void Configuration::parseBoard(const QaplaHelpers::IniFile::Section& section) {
    try {
        auto timeControlOpt = section.getValue("timecontrol");
        if (!timeControlOpt) {
            return;
        }
        timeControlSettings_.setSelectionFromString(*timeControlOpt);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error parsing board section: ") + e.what());
    }
}
