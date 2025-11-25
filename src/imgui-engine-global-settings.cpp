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
#include "imgui-engine-controls.h"
#include "imgui-controls.h"
#include "configuration.h"
#include "tutorial.h"

#include "engine-config.h"
#include "engine-option.h"

#include <imgui.h>

using namespace QaplaWindows;

ImGuiEngineGlobalSettings::ImGuiEngineGlobalSettings(const Options& options, ConfigurationChangedCallback callback)
    : options_(options)
    , configurationCallback_(std::move(callback))
    , timeControlCallback_(nullptr)
{
}

bool ImGuiEngineGlobalSettings::drawGlobalSettings(float controlWidth, float controlIndent, const Tutorial::TutorialContext& tutorialContext) {
    bool modified = false;
    ImGuiTreeNodeFlags flags = options_.alwaysOpen ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None;
    
    if (ImGuiControls::CollapsingHeaderWithDot("Global Engine Settings", flags, tutorialContext.highlight)) {
        ImGui::Indent(controlIndent);
        
        // Hash size control
        if (options_.showHash) {
            constexpr uint32_t maxHashMB = 64000;
            if (options_.showUseCheckboxes) {
                modified |= ImGui::Checkbox("##useHash", &globalSettings_.useGlobalHash);
                ImGuiControls::hooverTooltip("Enable global hash size setting for all engines");
                ImGui::SameLine();
            } else {
                globalSettings_.useGlobalHash = true;
            }
            ImGui::SetNextItemWidth(controlWidth);
            ImGui::BeginDisabled(!globalSettings_.useGlobalHash);
            modified |= ImGuiControls::inputInt<uint32_t>("Hash (MB)", globalSettings_.hashSizeMB, 1, maxHashMB);
            ImGuiControls::hooverTooltip("Hash table size in megabytes for engine memory");
            ImGui::EndDisabled();
            
            // Show tutorial annotation if present
            auto it = tutorialContext.annotations.find("Hash (MB)");
            if (it != tutorialContext.annotations.end()) {
                ImGuiControls::annotate(it->second);
            }
        }
        
        // Restart option control
        if (options_.showRestart) {
            if (options_.showUseCheckboxes) {
                modified |= ImGui::Checkbox("##useRestart", &globalSettings_.useGlobalRestart);
                ImGuiControls::hooverTooltip("Enable global restart policy for all engines");
                ImGui::SameLine();
            } else {
                globalSettings_.useGlobalRestart = true;
            }
            ImGui::SetNextItemWidth(controlWidth);
            ImGui::BeginDisabled(!globalSettings_.useGlobalRestart);
            modified |= ImGuiControls::selectionBox("Restart", globalSettings_.restart,
                {"Engine decides", "Always", "Never"});
            ImGuiControls::hooverTooltip("Whether to restart engine process between games");
            ImGui::EndDisabled();
            
            // Show tutorial annotation if present
            auto it = tutorialContext.annotations.find("Restart");
            if (it != tutorialContext.annotations.end()) {
                ImGuiControls::annotate(it->second);
            }
        }
        
        // Trace level control
        if (options_.showTrace) {
            if (options_.showUseCheckboxes) {
                modified |= ImGui::Checkbox("##useTrace", &globalSettings_.useGlobalTrace);
                ImGuiControls::hooverTooltip("Enable global trace level for all engines");
                ImGui::SameLine();
            } else {
                globalSettings_.useGlobalTrace = true;
            }
            ImGui::SetNextItemWidth(controlWidth);
            ImGui::BeginDisabled(!globalSettings_.useGlobalTrace);
            modified |= ImGuiControls::selectionBox("Trace", globalSettings_.traceLevel,
                {"None", "All", "Command"});
            ImGuiControls::hooverTooltip("Engine communication logging level (None/All/Command only)");
            ImGui::EndDisabled();
            
            // Show tutorial annotation if present
            auto it = tutorialContext.annotations.find("Trace");
            if (it != tutorialContext.annotations.end()) {
                ImGuiControls::annotate(it->second);
            }
        }
        
        // Ponder control
        if (options_.showPonder) {
            if (options_.showUseCheckboxes) {
                modified |= ImGui::Checkbox("##usePonder", &globalSettings_.useGlobalPonder);
                ImGuiControls::hooverTooltip("Enable global pondering setting for all engines");
                ImGui::SameLine();
            } else {
                globalSettings_.useGlobalPonder = true;
            }
            ImGui::SetNextItemWidth(controlWidth);
            ImGui::BeginDisabled(!globalSettings_.useGlobalPonder);
            modified |= ImGui::Checkbox("Ponder", &globalSettings_.ponder);
            ImGuiControls::hooverTooltip("Allow engines to think during opponent's time");
            ImGui::EndDisabled();
            
            // Show tutorial annotation if present
            auto it = tutorialContext.annotations.find("Ponder");
            if (it != tutorialContext.annotations.end()) {
                ImGuiControls::annotate(it->second);
            }
        }
        
        ImGui::Unindent(controlIndent);
    }
    
    if (modified) {
        notifyConfigurationChanged();
    }
    
    return modified;
}

bool ImGuiEngineGlobalSettings::drawTimeControl(float controlWidth, float controlIndent, 
    bool blitz, const Tutorial::TutorialContext& tutorialContext) {
    bool modified = false;
    ImGuiTreeNodeFlags flags = options_.alwaysOpen ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None;
    
    if (ImGuiControls::CollapsingHeaderWithDot("Time Control", flags, tutorialContext.highlight)) {
        ImGui::Indent(controlIndent);
        
        // Both controls work with the SAME variable (timeControlSettings_.timeControl)
        // This ensures automatic synchronization: selecting "50.0+0.10" in the dropdown
        // automatically updates the input field, and vice versa - no separate sync needed
        modified |= ImGuiControls::timeControlInput(timeControlSettings_.timeControl, blitz, controlWidth);
        ImGuiControls::hooverTooltip("Time control format: seconds+increment (e.g., '60.0+0.5' for 60s + 0.5s/move)");
        
        ImGui::SetNextItemWidth(controlWidth);
        modified |= ImGuiControls::selectionBox("Predefined time control", timeControlSettings_.timeControl, 
                                        timeControlSettings_.predefinedOptions);
        ImGuiControls::hooverTooltip(
            "Quick selection for common time controls.\n"
            "Selecting an option automatically fills the input fields above.\n"
            "Example: '20.0+0.02' sets Seconds=20, Increment Ms=20"
        );
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("Predefined time control");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
        
        ImGui::Unindent(controlIndent);
    }
    
    if (modified) {
        notifyTimeControlChanged();
    }
    
    return modified;
}

void ImGuiEngineGlobalSettings::setGlobalConfiguration(const GlobalConfiguration& globalSettings) {
    globalSettings_ = globalSettings;
    notifyConfigurationChanged();
}

void ImGuiEngineGlobalSettings::setTimeControlSettings(const TimeControlSettings& timeControlSettings) {
    timeControlSettings_ = timeControlSettings;
    notifyTimeControlChanged();
}

void ImGuiEngineGlobalSettings::setConfigurationChangedCallback(ConfigurationChangedCallback callback) {
    configurationCallback_ = std::move(callback);
}

void ImGuiEngineGlobalSettings::setTimeControlChangedCallback(TimeControlChangedCallback callback) {
    timeControlCallback_ = std::move(callback);
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
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "eachengine", id_, {{
            .name = "eachengine",
            .entries = QaplaHelpers::IniFile::KeyValueMap{
                {"id", id_},
                {"usehash", globalSettings_.useGlobalHash ? "true" : "false"},
                {"hash", std::to_string(globalSettings_.hashSizeMB)},
                {"useponder", globalSettings_.useGlobalPonder ? "true" : "false"},
                {"ponder", globalSettings_.ponder ? "true" : "false"},
                {"usetrace", globalSettings_.useGlobalTrace ? "true" : "false"},
                {"trace", globalSettings_.traceLevel},
                {"userestart", globalSettings_.useGlobalRestart ? "true" : "false"},
                {"restart", globalSettings_.restart}
            }
    }});
}

void ImGuiEngineGlobalSettings::updateTimeControlConfiguration() const {
    // Build predefined options entries
    QaplaHelpers::IniFile::KeyValueMap entries{
        {"id", id_},
        {"timeControl", timeControlSettings_.timeControl}
    };
    
    // Add predefined options
    for (size_t i = 0; i < timeControlSettings_.predefinedOptions.size(); ++i) {
        entries.emplace_back("predefinedOption" + std::to_string(i), timeControlSettings_.predefinedOptions[i]);
    }
    
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "timecontroloptions", id_, {{
            .name = "timecontroloptions",
            .entries = std::move(entries)
    }});
}

void ImGuiEngineGlobalSettings::loadHashSettings(const QaplaHelpers::IniFile::Section& section, GlobalConfiguration& settings) {
    if (auto value = section.getValue("usehash")) {
        settings.useGlobalHash = (*value == "true" || *value == "1");
    }
    if (auto value = section.getValue("hash")) {
        try {
            settings.hashSizeMB = std::stoul(*value);
        } catch (...) {
            settings.hashSizeMB = 32;
        }
    }
}

void ImGuiEngineGlobalSettings::loadPonderSettings(const QaplaHelpers::IniFile::Section& section, GlobalConfiguration& settings) {
    if (auto value = section.getValue("useponder")) {
        settings.useGlobalPonder = (*value == "true" || *value == "1");
    }
    if (auto value = section.getValue("ponder")) {
        settings.ponder = (*value == "true" || *value == "1");
    }
}

void ImGuiEngineGlobalSettings::loadTraceSettings(const QaplaHelpers::IniFile::Section& section, GlobalConfiguration& settings) {
    if (auto value = section.getValue("usetrace")) {
        settings.useGlobalTrace = (*value == "true" || *value == "1");
    }
    if (auto value = section.getValue("trace")) {
        settings.traceLevel = *value;
    }
}

void ImGuiEngineGlobalSettings::loadRestartSettings(const QaplaHelpers::IniFile::Section& section, GlobalConfiguration& settings) {
    if (auto value = section.getValue("userestart")) {
        settings.useGlobalRestart = (*value == "true" || *value == "1");
    }
    if (auto value = section.getValue("restart")) {
        settings.restart = *value;
    }
}

void ImGuiEngineGlobalSettings::setGlobalConfiguration(const QaplaHelpers::IniFile::SectionList& sections) {
    for (const auto& section : sections) {
        if (section.name == "eachengine") {
            GlobalConfiguration newGlobalSettings;
            
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
            
            // Load predefined options
            newTimeControlSettings.predefinedOptions.clear();
            for (size_t i = 0; ; ++i) {
                auto value = section.getValue("predefinedOption" + std::to_string(i));
                if (!value) {
                    break;
                }
                newTimeControlSettings.predefinedOptions.push_back(*value);
            }
            
            // If no predefined options were loaded, use defaults
            if (newTimeControlSettings.predefinedOptions.empty()) {
                newTimeControlSettings.predefinedOptions = timeControlSettings_.predefinedOptions;
            }
            
            timeControlSettings_ = newTimeControlSettings;
            notifyTimeControlChanged();
            break;
        }
    }
}

void ImGuiEngineGlobalSettings::applyGlobalConfig(QaplaTester::EngineConfig& engine, 
                                                   const GlobalConfiguration& globalSettings, 
                                                   const TimeControlSettings& timeControlSettings) {
    if (globalSettings.useGlobalPonder) {
        engine.setPonder(globalSettings.ponder);
    }
    
    engine.setTimeControl(timeControlSettings.timeControl);
    
    if (globalSettings.useGlobalRestart) {
        engine.setRestartOption(QaplaTester::parseRestartOption(globalSettings.restart));
    }
    
    if (globalSettings.useGlobalTrace) {
        engine.setTraceLevel(globalSettings.traceLevel);
    }
    
    if (globalSettings.useGlobalHash) {
        engine.setOptionValue("Hash", std::to_string(globalSettings.hashSizeMB));
    }
}
