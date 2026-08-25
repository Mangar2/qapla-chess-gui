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
#include "config-group-loader.h"
#include "tournament-data.h"
#include "interactive-board-window.h"
#include "callback-manager.h"

#include <base-elements/logger.h>
#include <base-elements/string-helper.h>
#include <base-elements/ini-file.h>
#include <base-elements/timer.h>
#include <base-elements/time-control.h>
#include <engine-handling/engine-worker-factory.h>
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
    // Before anything is read: EngineConfig resolves the key names of an engine section through
    // the central parameter definition, so reading the engine catalogue in loadData() throws
    // while that definition is unregistered. Registering it here rather than relying on some
    // earlier caller keeps loading independent of startup order.
    ensureSettingsRegistered();

    saveCallbackHandle_ = QaplaWindows::StaticCallbacks::save().registerCallback([this]() {
        this->saveFile();
    });
}

// getConfigDirectory, autosave, saveFile, loadFile are now handled by Autosavable base class

QaplaTester::Settings::Manager& QaplaConfiguration::loadGroupIntoManager(
    const std::string& sectionName, const std::string& id) {
    auto sections = Configuration::instance().getConfigData()
        .getSectionList(sectionName, id)
        .value_or(QaplaHelpers::IniFile::SectionList{});
    return loadGroupIntoManager(sectionName, sections);
}

void Configuration::saveData(std::ofstream& out) {
	engineCapabilities_.save(out);
	{
	    QaplaHelpers::IniFile::SectionList sections;
	    for (const auto& config : EngineWorkerFactory::getConfigManager().getAllConfigs()) {
	        sections.push_back(config.toSection("engine"));
	    }
	    QaplaHelpers::IniFile::saveSections(out, sections);
	}
    getConfigData().save(out);
    out.flush();
}

void Configuration::loadData(std::ifstream& in) {
    try {
        auto sectionList = QaplaHelpers::IniFile::load(in);
        std::string line;

        for (const auto& section : sectionList) {
            // A section this build cannot make sense of must not cost the user the rest of the
            // file. Letting the exception through used to abandon the load at the first such
            // section, and since the sections read so far were then all there was to save, the
            // remainder was written out of existence on the next save.
            try {
                if (!processSection(section)) {
                    configData_.addSection(section);
                }
            }
            catch (const std::exception& e) {
                Logger::reportLogger().log(
                    std::string("Skipping unreadable section [") + section.name + "]: " + e.what(),
                    TraceLevel::error);
                markLoadIncomplete("section [" + section.name + "] could not be read");
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
        else if (sectionName == "engine" && !section.getValue("id").has_value()) {
            // Only the global engine catalog has no "id": per-window engine selections
            // (tournament, sprt-tournament, enginetest, epd, ...) reuse the "engine" section
            // name but carry an "id" and must stay in configData_ so their owning window's
            // id-scoped getSectionList("engine", id) lookup can find them again.
            QaplaTester::EngineConfig config;
            config.setValues(section.getUnorderedMap());
            EngineWorkerFactory::getConfigManagerMutable().addOrReplaceByCmd(config);
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

    // BaseLogger's built-in default (TraceLevel::info) logs everything to the console,
    // including every engine's raw UCI/xboard "info" lines -- a debug-only verbosity level
    // meant for the standalone qapla-engine-tester CLI. That CLI actually runs quieter than
    // its own default: QaplaSettings::applyLoggerConfig() dials the console down to
    // error-only for engine communication (still logging everything to file) and to
    // result-only for game/tournament progress, before running anything. The GUI never went
    // through that CLI init path, so it kept the noisy library default. Match the CLI here.
    QaplaTester::EngineLogger::engineLogger().setTraceLevel(
        QaplaTester::TraceLevel::error, QaplaTester::TraceLevel::info);
    QaplaTester::Logger::reportLogger().setTraceLevel(
        QaplaTester::TraceLevel::result, QaplaTester::TraceLevel::info);
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

namespace {
    /** @brief Set by --remote-desktop; see Configuration::setRemoteDesktopOverride(). */
    bool remoteDesktopOverride = false;
}

void Configuration::setRemoteDesktopOverride() {
    remoteDesktopOverride = true;
}

bool Configuration::isRemoteDesktopMode() {
    if (remoteDesktopOverride) {
        return true;
    }
    auto sections = Configuration::instance().
        getConfigData().getSectionList("performance", "general").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!sections.empty()) {
        const auto& section = sections[0];
        std::string value = section.getValue("remotedesktopmode").value_or("false");
        return (value == "true" || value == "1");
    }
    return false;
}

void Configuration::setRemoteDesktopMode(bool enabled) {
    QaplaHelpers::IniFile::Section section {
        .name = "performance",
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", "general"},
            {"remotedesktopmode", enabled ? "true" : "false"}
        }
    };
    
    Configuration::instance().getConfigData().setSectionList("performance", "general", { section });
    Configuration::instance().setModified();
}

Configuration::LlmChatConfig Configuration::getLlmChatConfig() {
    LlmChatConfig result;
    auto sections = Configuration::instance().
        getConfigData().getSectionList("llmchat", "general").value_or(std::vector<QaplaHelpers::IniFile::Section>{});

    if (!sections.empty()) {
        const auto& section = sections[0];
        std::string enabledValue = section.getValue("enabled").value_or("true");
        result.enabled = (enabledValue == "true" || enabledValue == "1");
        result.host = section.getValue("host").value_or(result.host);
        result.port = static_cast<int>(
            QaplaHelpers::to_uint32(section.getValue("port").value_or("")).value_or(static_cast<uint32_t>(result.port)));
        std::string logTrafficValue = section.getValue("logtraffic").value_or("true");
        result.logTraffic = (logTrafficValue == "true" || logTrafficValue == "1");
        result.responseTimeoutSeconds = static_cast<int>(
            QaplaHelpers::to_uint32(section.getValue("responsetimeoutseconds").value_or(""))
                .value_or(static_cast<uint32_t>(result.responseTimeoutSeconds)));
        // "host_history" is a repeatable key (KeyValueMap allows duplicates), one entry per
        // remembered host, stored in most-recently-used order.
        for (const auto& [key, value] : section.entries) {
            if (key == "host_history") {
                result.hostHistory.push_back(value);
            }
        }
    }
    return result;
}

void Configuration::setLlmChatConfig(const LlmChatConfig& config) {
    QaplaHelpers::IniFile::Section section {
        .name = "llmchat",
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", "general"},
            {"enabled", config.enabled ? "true" : "false"},
            {"host", config.host},
            {"port", std::to_string(config.port)},
            {"logtraffic", config.logTraffic ? "true" : "false"},
            {"responsetimeoutseconds", std::to_string(config.responseTimeoutSeconds)}
        }
    };
    for (const auto& host : config.hostHistory) {
        section.addEntry("host_history", host);
    }

    Configuration::instance().getConfigData().setSectionList("llmchat", "general", { section });
    Configuration::instance().setModified();
}
