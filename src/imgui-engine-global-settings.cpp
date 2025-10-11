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

#include "imgui-engine-global-settings.h"
#include "imgui-controls.h"
#include "configuration.h"

#include <imgui.h>

using namespace QaplaWindows;

ImGuiEngineGlobalSettings::ImGuiEngineGlobalSettings(const Options& options, ConfigurationChangedCallback callback)
    : options_(options)
    , configurationCallback_(std::move(callback))
    , timeControlCallback_(nullptr)
{
}

bool ImGuiEngineGlobalSettings::drawGlobalSettings(float controlWidth, float controlIndent) {
    bool modified = false;
    
    ImGui::Indent(controlIndent);
    if (ImGui::CollapsingHeader("Global Engine Settings")) {
        ImGui::Indent(controlIndent);
        
        // Hash size control
        if (options_.showHash) {
            constexpr uint32_t maxHashMB = 64000;
            modified |= ImGui::Checkbox("##useHash", &globalSettings_.useGlobalHash);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(controlWidth);
            ImGui::BeginDisabled(!globalSettings_.useGlobalHash);
            modified |= ImGuiControls::inputInt<uint32_t>("Hash (MB)", globalSettings_.hashSizeMB, 1, maxHashMB);
            ImGui::EndDisabled();
        }
        
        // Restart option control
        if (options_.showRestart) {
            modified |= ImGui::Checkbox("##useRestart", &globalSettings_.useGlobalRestart);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(controlWidth);
            ImGui::BeginDisabled(!globalSettings_.useGlobalRestart);
            modified |= ImGuiControls::selectionBox("Restart", globalSettings_.restart, 
                {"auto", "on", "off"});
            ImGui::EndDisabled();
        }
        
        // Trace level control
        if (options_.showTrace) {
            modified |= ImGui::Checkbox("##useTrace", &globalSettings_.useGlobalTrace);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(controlWidth);
            ImGui::BeginDisabled(!globalSettings_.useGlobalTrace);
            modified |= ImGuiControls::selectionBox("Trace", globalSettings_.traceLevel,
                {"none", "all", "command"});
            ImGui::EndDisabled();
        }
        
        // Ponder control
        if (options_.showPonder) {
            modified |= ImGui::Checkbox("##usePonder", &globalSettings_.useGlobalPonder);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(controlWidth);
            ImGui::BeginDisabled(!globalSettings_.useGlobalPonder);
            modified |= ImGui::Checkbox("Ponder", &globalSettings_.ponder);
            ImGui::EndDisabled();
        }
        
        ImGui::Unindent(controlIndent);
    }
    ImGui::Unindent(controlIndent);
    
    if (modified) {
        notifyConfigurationChanged();
    }
    
    return modified;
}

bool ImGuiEngineGlobalSettings::drawTimeControl(float controlWidth, float controlIndent, bool blitz) {
    bool modified = false;
    
    ImGui::Indent(controlIndent);
    if (ImGui::CollapsingHeader("Time Control")) {
        ImGui::Indent(controlIndent);
        
        modified |= ImGuiControls::timeControlInput(timeControlSettings_.timeControl, blitz, controlWidth);
        
        ImGui::SetNextItemWidth(controlWidth);
        if (ImGuiControls::selectionBox("Predefined time control", timeControlSettings_.predefinedTimeControl, 
                                        timeControlSettings_.predefinedOptions)) {
            modified = true;
            if (timeControlSettings_.predefinedTimeControl != "Custom") {
                timeControlSettings_.timeControl = timeControlSettings_.predefinedTimeControl;
            }
        }
        
        ImGui::Unindent(controlIndent);
    }
    ImGui::Unindent(controlIndent);
    
    if (modified) {
        notifyTimeControlChanged();
    }
    
    return modified;
}

void ImGuiEngineGlobalSettings::setGlobalSettings(const GlobalSettings& globalSettings) {
    globalSettings_ = globalSettings;
    notifyConfigurationChanged();
}

void ImGuiEngineGlobalSettings::setTimeControlSettings(const TimeControlSettings& timeControlSettings) {
    timeControlSettings_ = timeControlSettings;
    notifyTimeControlChanged();
}

void ImGuiEngineGlobalSettings::setConfigurationChangedCallback(ConfigurationChangedCallback callback) {
    configurationCallback_ = std::move(callback);
    notifyConfigurationChanged();
}

void ImGuiEngineGlobalSettings::setTimeControlChangedCallback(TimeControlChangedCallback callback) {
    timeControlCallback_ = std::move(callback);
    notifyTimeControlChanged();
}

void ImGuiEngineGlobalSettings::notifyConfigurationChanged() {
    updateConfiguration();
    if (configurationCallback_) {
        configurationCallback_(globalSettings_);
    }
}

void ImGuiEngineGlobalSettings::notifyTimeControlChanged() {
    updateTimeControlConfiguration();
    if (timeControlCallback_) {
        timeControlCallback_(timeControlSettings_);
    }
}

void ImGuiEngineGlobalSettings::updateConfiguration() const {
    QaplaHelpers::IniFile::KeyValueMap entries;

    // Set the id for this configuration
    entries.emplace_back("id", id_);
    
    // Save hash settings
    entries.emplace_back("useGlobalHash", globalSettings_.useGlobalHash ? "true" : "false");
    entries.emplace_back("hashSizeMB", std::to_string(globalSettings_.hashSizeMB));
    
    // Save ponder settings
    entries.emplace_back("useGlobalPonder", globalSettings_.useGlobalPonder ? "true" : "false");
    entries.emplace_back("ponder", globalSettings_.ponder ? "true" : "false");
    
    // Save trace settings
    entries.emplace_back("useGlobalTrace", globalSettings_.useGlobalTrace ? "true" : "false");
    entries.emplace_back("traceLevel", globalSettings_.traceLevel);
    
    // Save restart settings
    entries.emplace_back("useGlobalRestart", globalSettings_.useGlobalRestart ? "true" : "false");
    entries.emplace_back("restart", globalSettings_.restart);
    
    QaplaHelpers::IniFile::Section section{
        .name = "eachengine",
        .entries = std::move(entries)
    };
    
    QaplaHelpers::IniFile::SectionList sections;
    sections.push_back(std::move(section));
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("eachengine", id_, sections);
}

void ImGuiEngineGlobalSettings::updateTimeControlConfiguration() const {
    QaplaHelpers::IniFile::KeyValueMap entries;
    
    // Set the id for this configuration
    entries.emplace_back("id", id_);
    
    // Save time control settings
    entries.emplace_back("timeControl", timeControlSettings_.timeControl);
    entries.emplace_back("predefinedTimeControl", timeControlSettings_.predefinedTimeControl);
    
    // Save predefined options
    for (size_t i = 0; i < timeControlSettings_.predefinedOptions.size(); ++i) {
        entries.emplace_back("predefinedOption" + std::to_string(i), timeControlSettings_.predefinedOptions[i]);
    }
    
    QaplaHelpers::IniFile::Section section{
        .name = "timecontroloptions",
        .entries = std::move(entries)
    };
    
    QaplaHelpers::IniFile::SectionList sections;
    sections.push_back(std::move(section));
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("timecontroloptions", id_, sections);
}

void ImGuiEngineGlobalSettings::loadHashSettings(const QaplaHelpers::IniFile::Section& section, GlobalSettings& settings) {
    if (auto value = section.getValue("useGlobalHash")) {
        settings.useGlobalHash = (*value == "true" || *value == "1");
    }
    if (auto value = section.getValue("hashSizeMB")) {
        try {
            settings.hashSizeMB = std::stoul(*value);
        } catch (...) {
            settings.hashSizeMB = 128;
        }
    }
}

void ImGuiEngineGlobalSettings::loadPonderSettings(const QaplaHelpers::IniFile::Section& section, GlobalSettings& settings) {
    if (auto value = section.getValue("useGlobalPonder")) {
        settings.useGlobalPonder = (*value == "true" || *value == "1");
    }
    if (auto value = section.getValue("ponder")) {
        settings.ponder = (*value == "true" || *value == "1");
    }
}

void ImGuiEngineGlobalSettings::loadTraceSettings(const QaplaHelpers::IniFile::Section& section, GlobalSettings& settings) {
    if (auto value = section.getValue("useGlobalTrace")) {
        settings.useGlobalTrace = (*value == "true" || *value == "1");
    }
    if (auto value = section.getValue("traceLevel")) {
        settings.traceLevel = *value;
    }
}

void ImGuiEngineGlobalSettings::loadRestartSettings(const QaplaHelpers::IniFile::Section& section, GlobalSettings& settings) {
    if (auto value = section.getValue("useGlobalRestart")) {
        settings.useGlobalRestart = (*value == "true" || *value == "1");
    }
    if (auto value = section.getValue("restart")) {
        settings.restart = *value;
    }
}

void ImGuiEngineGlobalSettings::setGlobalConfiguration(const QaplaHelpers::IniFile::SectionList& sections) {
    for (const auto& section : sections) {
        if (section.name == "eachengine") {
            GlobalSettings newGlobalSettings;
            
            loadHashSettings(section, newGlobalSettings);
            loadPonderSettings(section, newGlobalSettings);
            loadTraceSettings(section, newGlobalSettings);
            loadRestartSettings(section, newGlobalSettings);
            
            globalSettings_ = newGlobalSettings;
            notifyConfigurationChanged();
            break;
        }
    }
}

void ImGuiEngineGlobalSettings::setTimeControlConfiguration(const QaplaHelpers::IniFile::SectionList& sections) {
    for (const auto& section : sections) {
        if (section.name == "timecontroloptions") {
            TimeControlSettings newTimeControlSettings;
            
            // Load time control
            if (auto value = section.getValue("timeControl")) {
                newTimeControlSettings.timeControl = *value;
            }
            
            // Load predefined time control
            if (auto value = section.getValue("predefinedTimeControl")) {
                newTimeControlSettings.predefinedTimeControl = *value;
            }
            
            // Load predefined options
            newTimeControlSettings.predefinedOptions.clear();
            for (size_t i = 0; ; ++i) {
                auto value = section.getValue("predefinedOption" + std::to_string(i));
                if (!value) break;
                newTimeControlSettings.predefinedOptions.push_back(*value);
            }
            
            // If no predefined options were loaded, use defaults
            if (newTimeControlSettings.predefinedOptions.empty()) {
                newTimeControlSettings.predefinedOptions = {
                    "Custom", "10.0+0.02", "20.0+0.02", "50.0+0.10", "60.0+0.20"
                };
            }
            
            timeControlSettings_ = newTimeControlSettings;
            notifyTimeControlChanged();
            break;
        }
    }
}
