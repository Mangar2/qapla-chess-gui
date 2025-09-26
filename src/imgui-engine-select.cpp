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
    
    int index = 0;
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
        bool modified = false;
        if (options_.allowGauntletEdit) {
            modified |= ImGuiControls::checkbox("Gauntlet", config.config.gauntlet());
        }
        
        // Additional options can be added here based on options_
        return modified;
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
}

void ImGuiEngineSelect::setOptions(const Options& options) {
    options_ = options;
}

const ImGuiEngineSelect::Options& ImGuiEngineSelect::getOptions() const {
    return options_;
}

std::vector<ImGuiEngineSelect::EngineConfiguration>::iterator 
ImGuiEngineSelect::findEngineConfiguration(const EngineConfig& engineConfig) {
    return std::find_if(engineConfigurations_.begin(), engineConfigurations_.end(),
        [&engineConfig](const EngineConfiguration& configured) {
            return configured.config == engineConfig;
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
        // Create Section object explicitly for clarity
        QaplaHelpers::IniFile::Section section{
            .name = "engineselection",  // section name
            .entries = QaplaHelpers::IniFile::KeyValueMap{  // key-value entries
                {"id", id_},
                {"name", engine.config.getName()},
                {"selected", engine.selected ? "true" : "false"},
                {"gauntlet", engine.config.isGauntlet() ? "true" : "false"}
            }
        };
        sections.push_back(std::move(section));
    }
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("engineselection", "tournament", sections);
}

void ImGuiEngineSelect::setEngineConfiguration(const QaplaHelpers::IniFile::SectionList& sections) {
    engineConfigurations_.clear();
    for (const auto& section : sections) {
        if (section.name == "engineselection" && section.getValue("id") == id_) {
            EngineConfiguration config;
            auto selectedValue = section.getValue("selected");
            config.selected = selectedValue ? (*selectedValue == "true") : false;
            auto engineName = section.getValue("name");
            if (!engineName) {
                continue; 
            }
            auto configDef = EngineWorkerFactory::getConfigManager().getConfig(*engineName);
            if (configDef) {
                config.config = *configDef;
            }
            for (const auto& [key, value] : section.entries) {
                if (key == "gauntlet") {
                    config.config.setGauntlet(value == "true");
                }
            }
            engineConfigurations_.push_back(std::move(config));
        }
    }
}
