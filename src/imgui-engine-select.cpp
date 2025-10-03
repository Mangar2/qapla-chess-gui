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
#include "qapla-tester/engine-config-manager.h"

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
    
    // Clean up engines that are no longer available
    auto& configManager = EngineWorkerFactory::getConfigManagerMutable();
    for (uint32_t index = 0; index < engineConfigurations_.size(); ) {
        const auto& usedConfig = engineConfigurations_[index];
        auto baseConfig = configManager.getConfigMutableByCmdAndProtocol(
            usedConfig.config.getCmd(), usedConfig.config.getProtocol());
        if (!baseConfig) {
            engineConfigurations_.erase(engineConfigurations_.begin() + index);
            modified = true;
        } else {
            index++;
        }
    }
    
    if (options_.allowMultipleSelection) {
        // Multiple selection mode: show selected engines first, then all available engines
        modified |= drawSelectedEngines();
        ImGui::Separator();
        modified |= drawAvailableEngines();
    } else {
        // Original single selection mode
        auto configs = configManager.getAllConfigs();
        uint32_t index = 0;
        for (auto& config : configs) {
            EngineConfiguration engine = {
                .config = config,
                .selected = false
            };
            
            auto it = findEngineConfiguration(config);
            if (it != engineConfigurations_.end()) {
                engine = *it;
            }         

            if (drawEngineConfiguration(engine, index)) {
                modified = true;
                
                if (it == engineConfigurations_.end()) {
                    engineConfigurations_.push_back(engine);
                } else {
                    *it = engine;
                }
            }
            index++;
        }
    }
    
    if (modified) {
        updateUniqueDisplayNames();
        notifyConfigurationChanged();
    }
    
    return modified;
}

bool ImGuiEngineSelect::drawEngineConfiguration(EngineConfiguration& config, int index) {
    auto name = config.config.getName().empty() ? std::to_string(index) : config.config.getName();
    name += "###" + std::to_string(index);
    bool modified = false;
    
    ImGui::PushID(index);
    
    ImGuiTreeNodeFlags flags = config.selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_Leaf;
    
    bool changed = ImGuiControls::collapsingSelection(name, config.selected, flags, [this, &config]() -> bool {
        bool configModified = false;
        
        ImGuiEngineControls::drawEngineReadOnlyInfo(config.config);
        ImGui::Separator();
        
        configModified |= ImGuiEngineControls::drawEngineName(config.config, options_.allowNameEdit);
        // If name was changed by user, update originalName to reflect the new user choice
        if (configModified && options_.allowNameEdit) {
            config.originalName = config.config.getName();
        }
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
    // Initialize originalName for all configurations
    for (auto& config : engineConfigurations_) {
        if (config.originalName.empty()) {
            config.originalName = config.config.getName();
        }
    }
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

std::vector<ImGuiEngineSelect::EngineConfiguration>::iterator 
ImGuiEngineSelect::findDeselectedEngineConfiguration(const EngineConfig& engineConfig) {
    return std::find_if(engineConfigurations_.begin(), engineConfigurations_.end(),
        [&engineConfig](const EngineConfiguration& configured) {
            // Match by command line and protocol, but only if not selected
            return !configured.selected && 
                   configured.config.getCmd() == engineConfig.getCmd() && 
                   configured.config.getProtocol() == engineConfig.getProtocol();
        });
}

bool ImGuiEngineSelect::drawSelectedEngines() {
    bool modified = false;
    
    if (ImGui::CollapsingHeader("Selected Engines", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0f);
        
        // Draw all selected engines with checkboxes
        int index = 0;
        for (auto& engine : engineConfigurations_) {
            if (engine.selected) {
                if (drawEngineConfiguration(engine, index)) {
                    modified = true;
                }
            }
            index++;
        }
        
        ImGui::Unindent(10.0f);
    }
    
    return modified;
}

bool ImGuiEngineSelect::drawAvailableEngines() {
    bool modified = false;
    
    if (ImGui::CollapsingHeader("Available Engines", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0f);
        
        auto& configManager = EngineWorkerFactory::getConfigManagerMutable();
        auto configs = configManager.getAllConfigs();
        
        int index = 0;
        for (auto& config : configs) {
            if (config.getName().empty()) {
                continue; 
            }
            ImGui::PushID(("available_" + std::to_string(index)).c_str());
           
            // Use a simple button to add the engine instead of checkbox
            if (ImGui::Button("+")) {
                // Find if there's a deselected instance of this engine
                auto it = findDeselectedEngineConfiguration(config);
                if (it != engineConfigurations_.end()) {
                    // Mark existing deselected instance as selected
                    it->selected = true;
                } else {
                    // Add new selected instance
                    EngineConfiguration newEngine = {
                        .config = config,
                        .selected = true,
                        .originalName = config.getName()
                    };
                    engineConfigurations_.push_back(newEngine);
                }
                modified = true;
            }
            
            // Show engine info in a collapsing header (read-only)
            ImGui::SameLine();
            ImGui::CollapsingHeader(config.getName().c_str(), ImGuiTreeNodeFlags_Leaf);
            
            ImGui::PopID();
            index++;
        }
        
        ImGui::Unindent(10.0f);
    }
    
    return modified;
}

void ImGuiEngineSelect::notifyConfigurationChanged() {
    updateConfiguration();
    if (configurationCallback_) {
        configurationCallback_(engineConfigurations_);
    }
}

void ImGuiEngineSelect::resetNamesToOriginal() {
    for (auto& engineConfig : engineConfigurations_) {
        if (engineConfig.selected && !engineConfig.originalName.empty()) {
            engineConfig.config.setName(engineConfig.originalName);
        }
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
            {"originalName", engine.originalName},  // Store original name separately
            {"author", engine.config.getAuthor()},
            {"cmd", engine.config.getCmd()},
            {"proto", to_string(engine.config.getProtocol())},
        };

        // Only store non-default or enabled options to keep the configuration concise
        auto& config = engine.config;
        if (engine.config.getDir() != ".") {
            entries.emplace_back("dir", engine.config.getDir());
        }
        if (config.getRestartOption() != RestartOption::EngineDecides) {
            entries.emplace_back("restart", to_string(config.getRestartOption()));
        }
        if (config.isGauntlet()) { entries.emplace_back("gauntlet", "true"); }
        if (config.isPonderEnabled()) { entries.emplace_back("ponder", "true"); }

        if (config.getTraceLevel() != TraceLevel::command) {
            entries.emplace_back("trace", ImGuiEngineControls::to_string(config.getTraceLevel()));
        }

        if (config.getTimeControl().isValid()) {
            entries.emplace_back("timecontrol", config.getTimeControl().toPgnTimeControlString());
        }
        
        // Add engine-specific options
        auto optionValues = config.getOptionValues();
        for (const auto& [originalName, optionValue] : optionValues) {
            entries.emplace_back(originalName, optionValue);
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
            
            engineConfig.config.setValues(section.getUnorderedMap());
            // Load originalName if stored, otherwise use current name
            auto originalNameValue = section.getValue("originalName");
            engineConfig.originalName = originalNameValue ? *originalNameValue : engineConfig.config.getName();
            // Parse selection state
            auto selectedValue = section.getValue("selected");
            engineConfig.selected = selectedValue ? (*selectedValue == "true") : false;            
            engineConfigurations_.push_back(std::move(engineConfig));
        }
    }
    updateUniqueDisplayNames();
    notifyConfigurationChanged();
}

void ImGuiEngineSelect::updateUniqueDisplayNames() {
    if (!options_.allowMultipleSelection) {
        return; // Only update names in multiple selection mode
    }

    resetNamesToOriginal();
    
    // Extract only selected engine configs
    std::vector<EngineConfig> selectedConfigs;
    for (auto& engineConfig : engineConfigurations_) {
        if (engineConfig.selected) {
            selectedConfigs.push_back(engineConfig.config);
        }
    }
    
    // Assign unique names
    EngineConfigManager::assignUniqueDisplayNames(selectedConfigs);
    
    // Update the original configurations
    size_t selectedIndex = 0;
    for (auto& engineConfig : engineConfigurations_) {
        if (engineConfig.selected) {
            engineConfig.config = selectedConfigs[selectedIndex++];
        }
    }
}






