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
#include "callback-manager.h"

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
    : Autosavable(CONFIG_FILE, ".bak", 60000, []() { return Autosavable::getConfigDirectory(); })
{
}

// getConfigDirectory, autosave, saveFile, loadFile are now handled by Autosavable base class

void Configuration::saveData(std::ofstream& out) {
	saveTimeControls(out);
	engineCapabilities_.save(out);
	EngineWorkerFactory::getConfigManager().saveToStream(out);
	QaplaWindows::TournamentData::instance().saveConfig(out);
    getConfigData().save(out);
}

void Configuration::loadData(std::ifstream& in) {
    try {
        auto sectionList = QaplaHelpers::IniFile::load(in);
        std::string line;

        for (const auto& section : sectionList) {
            if (!processSection(section) && !section.name.starts_with("tournament")) {
                configData_.addSection(section);
            }
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

bool Configuration::processSection(const QaplaHelpers::IniFile::Section& section) {
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
        else {
            return false;
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error processing section [" + sectionName + "]: " + e.what());
    }
    return true;
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
