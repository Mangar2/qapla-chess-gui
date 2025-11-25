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

#include "logger.h"
#include "string-helper.h"
#include "ini-file.h"
#include "timer.h"
#include "time-control.h"
#include "engine-worker-factory.h"
#include "i18n.h"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>

using QaplaTester::Logger;
using QaplaTester::TraceLevel;
using QaplaTester::EngineWorkerFactory;
#include <unordered_map>


using namespace QaplaConfiguration;
using namespace QaplaWindows;

Configuration::Configuration() 
    : Autosavable(CONFIG_FILE, ".bak", 60000, []() { return Autosavable::getConfigDirectory(); })
{
    saveCallbackHandle_ = QaplaWindows::StaticCallbacks::save().registerCallback([this]() {
        this->saveFile();
    });
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
        loadLoggerConfiguration();
        loadLanguageConfiguration();

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

void Configuration::loadLoggerConfiguration() {
    auto sections = Configuration::instance().
        getConfigData().getSectionList("logger", "logger").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!sections.empty()) {
        const auto& section = sections[0];
        auto config = QaplaTester::getLoggerConfig();
        config.logPath = section.getValue("logpath").value_or("./log");
        config.reportLogBaseName = section.getValue("reportlogbasename").value_or("report");
        config.engineLogBaseName = section.getValue("enginelogbasename").value_or("engine");
        
        auto strategyStr = section.getValue("enginelogstrategy").value_or("0");
        auto strategyInt = QaplaHelpers::to_uint32(strategyStr).value_or(0);
        config.engineLogStrategy = static_cast<QaplaTester::LogFileStrategy>(strategyInt);
        
        QaplaTester::setLoggerConfig(config);
    }
}

void Configuration::updateLoggerConfiguration() {
    const auto& config = QaplaTester::getLoggerConfig();
    
    QaplaHelpers::IniFile::Section section {
        .name = "logger",
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", "logger"},
            {"logpath", config.logPath},
            {"reportlogbasename", config.reportLogBaseName},
            {"enginelogbasename", config.engineLogBaseName},
            {"enginelogstrategy", std::to_string(static_cast<int>(config.engineLogStrategy))}
        }
    };
    
    Configuration::instance().getConfigData().setSectionList("logger", "logger", { section });
}

void Configuration::loadLanguageConfiguration() {
    auto sections = Configuration::instance().
        getConfigData().getSectionList("languagesettings", "general").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!sections.empty()) {
        const auto& section = sections[0];
        std::string languageCode = section.getValue("languagecode").value_or("eng");
        Translator::instance().setLanguageCode(languageCode);
    }
}

void Configuration::updateLanguageConfiguration(const std::string& languageCode) {
    QaplaHelpers::IniFile::Section section {
        .name = "languagesettings",
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", "general"},
            {"languagecode", languageCode}
        }
    };
    
    Configuration::instance().getConfigData().setSectionList("languagesettings", "general", { section });
}
