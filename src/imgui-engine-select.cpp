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

#include "imgui-engine-select.h"
#include "imgui-controls.h"
#include "imgui-engine-controls.h"
#include "configuration.h"
#include "qapla-tester/engine-worker-factory.h"

#include <imgui.h>
#include <algorithm>

using namespace QaplaWindows;

ImGuiEngineSelect::ImGuiEngineSelect(const Options& options, ConfigurationChangedCallback callback)
    : options_(options)
    , configurationCallback_(callback)
{
}

bool ImGuiEngineSelect::draw() {
    bool modified = false;
    
    // Get available engines from ConfigManager (same as original implementation)
    auto& configManager = EngineWorkerFactory::getConfigManagerMutable();
    auto configs = configManager.getAllConfigs();
    
    for (uint32_t index = 0; index < engineConfigurations_.size(); ) {
        const auto& usedConfig = engineConfigurations_[index];
        // Find base engine by cmd+protocol instead of name
        auto baseConfig = configManager.getConfigMutableByCmdAndProtocol(
            usedConfig.config.getCmd(), usedConfig.config.getProtocol());
        if (!baseConfig) {
            // No longer available engines must be removed
            engineConfigurations_.erase(engineConfigurations_.begin() + index);
            modified = true;
        } else {
            index++;
        }
    }
    uint32_t index = 0;
    for (auto& config : configs) {
        // Create engine configuration (same as original implementation)
        EngineConfiguration engine = {
            .config = config,
            .selected = false
        };
        
        // Check if this engine is already configured
        auto it = findEngineConfiguration(config);
        if (it != engineConfigurations_.end()) {
            engine = *it;
        }         

        // Draw the engine configuration
        if (drawEngineConfiguration(engine, index)) {
            modified = true;
            
            if (it == engineConfigurations_.end()) {
                // Engine was newly configured - add it to the list
                engineConfigurations_.push_back(engine);
            } else {
                // Engine was modified - update existing configuration
                *it = engine;
            }
        }
        
        index++;
    }
    
    if (modified) {
        notifyConfigurationChanged();
    }
    
    return modified;
}

bool ImGuiEngineSelect::drawEngineConfiguration(EngineConfiguration& config, int index) {
    const auto& name = config.config.getName().empty() ? "(unnamed)" : config.config.getName();
    bool modified = false;
    
    ImGui::PushID(index);
    
    ImGuiTreeNodeFlags flags = config.selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_Leaf;
    
    bool changed = ImGuiControls::collapsingSelection(name, config.selected, flags, [this, &config]() -> bool {
        bool configModified = false;
        
        ImGuiEngineControls::drawEngineReadOnlyInfo(config.config);
        ImGui::Separator();
        
        configModified |= ImGuiEngineControls::drawEngineName(config.config, options_.allowNameEdit);
        configModified |= ImGuiEngineControls::drawEngineGauntlet(config.config, options_.allowGauntletEdit);
        configModified |= ImGuiEngineControls::drawEnginePonder(config.config, options_.allowPonderEdit);
        configModified |= ImGuiEngineControls::drawEngineTimeControl(config.config, options_.allowTimeControlEdit);
        configModified |= ImGuiEngineControls::drawEngineTraceLevel(config.config, options_.allowTraceLevelEdit);
        configModified |= ImGuiEngineControls::drawEngineRestartOption(config.config, options_.allowRestartOptionEdit);
        configModified |= ImGuiEngineControls::drawEngineOptions(config.config, options_.allowEngineOptionsEdit);
        
        return configModified;
    });
    
    if (changed) {
        modified = true;
    }
    
    ImGui::PopID();
    
    return modified;
}



const std::vector<ImGuiEngineSelect::EngineConfiguration>& ImGuiEngineSelect::getEngineConfigurations() const {
    return engineConfigurations_;
}

void ImGuiEngineSelect::setEngineConfigurations(const std::vector<EngineConfiguration>& configurations) {
    engineConfigurations_ = configurations;
    notifyConfigurationChanged();
}

void ImGuiEngineSelect::setConfigurationChangedCallback(ConfigurationChangedCallback callback) {
    configurationCallback_ = callback;
    notifyConfigurationChanged();
}



std::vector<ImGuiEngineSelect::EngineConfiguration>::iterator 
ImGuiEngineSelect::findEngineConfiguration(const EngineConfig& engineConfig) {
    return std::find_if(engineConfigurations_.begin(), engineConfigurations_.end(),
        [&engineConfig](const EngineConfiguration& configured) {
            // Match by command line and protocol only (base engine identification)
            return configured.config.getCmd() == engineConfig.getCmd() && 
                   configured.config.getProtocol() == engineConfig.getProtocol();
        });
}

void ImGuiEngineSelect::notifyConfigurationChanged() {
    updateConfiguration();
    if (configurationCallback_) {
        configurationCallback_(engineConfigurations_);
    }
}

void ImGuiEngineSelect::updateConfiguration() const {
    QaplaHelpers::IniFile::SectionList sections;
    for (const auto& engine : engineConfigurations_) {
        // Store full configuration instead of just name reference
        QaplaHelpers::IniFile::KeyValueMap entries{
            {"id", id_},
            {"selected", engine.selected ? "true" : "false"},
            // Store all engine configuration attributes
            {"name", engine.config.getName()},
            {"author", engine.config.getAuthor()},
            {"cmd", engine.config.getCmd()},
            {"dir", engine.config.getDir()},
            {"protocol", engine.config.getProtocol() == EngineProtocol::Uci ? "uci" : "xboard"},
            {"ponder", engine.config.isPonderEnabled() ? "true" : "false"},
            {"gauntlet", engine.config.isGauntlet() ? "true" : "false"},
            {"tc", engine.config.getTimeControl().toPgnTimeControlString()},
            {"restart", to_string(engine.config.getRestartOption())},
            {"trace", engine.config.getTraceLevel() == TraceLevel::none ? "none" :
                     engine.config.getTraceLevel() == TraceLevel::info ? "all" : "command"}
        };
        
        // Add engine-specific options
        auto optionValues = engine.config.getOptionValues();
        for (const auto& [optionName, optionValue] : optionValues) {
            entries.emplace_back("option." + optionName, optionValue);
        }
        
        QaplaHelpers::IniFile::Section section{
            .name = "engineselection",
            .entries = std::move(entries)
        };
        sections.push_back(std::move(section));
    }
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("engineselection", id_, sections);
}

void ImGuiEngineSelect::setEngineConfiguration(const QaplaHelpers::IniFile::SectionList& sections) {
    engineConfigurations_.clear();
    for (const auto& section : sections) {
        if (section.name == "engineselection" && section.getValue("id") == id_) {
            EngineConfiguration engineConfig;
            
            // Parse selection state
            auto selectedValue = section.getValue("selected");
            engineConfig.selected = selectedValue ? (*selectedValue == "true") : false;
            
            // Check for required fields
            auto cmd = section.getValue("cmd");
            auto protocol = section.getValue("protocol");
            if (!cmd || !protocol) {
                continue; // Skip invalid configurations
            }
            
            // Build EngineConfig from stored values
            EngineConfig config;
            
            // Set basic attributes
            if (auto name = section.getValue("name")) {
                config.setName(*name);
            }
            if (auto author = section.getValue("author")) {
                config.setAuthor(*author);
            }
            config.setCmd(*cmd);
            if (auto dir = section.getValue("dir")) {
                config.setDir(*dir);
            }
            config.setProtocol(*protocol);
            
            // Set boolean options
            if (auto ponder = section.getValue("ponder")) {
                config.setPonder(*ponder == "true");
            }
            if (auto gauntlet = section.getValue("gauntlet")) {
                config.setGauntlet(*gauntlet == "true");
            }
            
            // Set time control
            if (auto tc = section.getValue("tc")) {
                try {
                    config.setTimeControl(*tc);
                } catch (const std::exception&) {
                    // Use default if parsing fails
                }
            }
            
            // Set trace level
            if (auto trace = section.getValue("trace")) {
                try {
                    config.setTraceLevel(*trace);
                } catch (const std::exception&) {
                    // Use default if parsing fails
                }
            }
            
            // Set restart option
            if (auto restart = section.getValue("restart")) {
                try {
                    config.setRestartOption(parseRestartOption(*restart));
                } catch (const std::exception&) {
                    // Use default if parsing fails
                }
            }
            
            // Set engine-specific options
            for (const auto& [key, value] : section.entries) {
                if (key.starts_with("option.")) {
                    std::string optionName = key.substr(7); // Remove "option." prefix
                    config.setOptionValue(optionName, value);
                }
            }
            
            // Store the configuration
            engineConfig.config = std::move(config);
            engineConfigurations_.push_back(std::move(engineConfig));
        }
    }
    notifyConfigurationChanged();
}






