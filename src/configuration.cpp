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

using QaplaTester::Logger;
using QaplaTester::TraceLevel;
using QaplaTester::EngineWorkerFactory;
#include <unordered_map>


using namespace QaplaConfiguration;

Configuration::Configuration() 
    : Autosavable(CONFIG_FILE, ".bak", 60000, []() { return Autosavable::getConfigDirectory(); })
{
}

// getConfigDirectory, autosave, saveFile, loadFile are now handled by Autosavable base class

void Configuration::saveData(std::ofstream& out) {
	engineCapabilities_.save(out);
	EngineWorkerFactory::getConfigManager().saveToStream(out);
    getConfigData().save(out);
    out.flush();
}

void Configuration::loadData(std::ifstream& in) {
    try {
        auto sectionList = QaplaHelpers::IniFile::load(in);
        std::string line;

        for (const auto& section : sectionList) {
            if (!processSection(section)) {
                configData_.addSection(section);
            }
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error in loadData: ") + e.what());
    }
}

bool Configuration::processSection(const QaplaHelpers::IniFile::Section& section) {
    const auto& sectionName = section.name;
    try {
        if (sectionName == "enginecapability") {
            engineCapabilities_.addOrReplace(section);
        }
        else if (sectionName == "engine") {
            QaplaTester::EngineConfig config;
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
