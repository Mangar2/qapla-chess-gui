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
        
        // Check if this engine is already selected
        auto it = findSelectedEngine(config);
        if (it != selectedEngines_.end()) {
            engine = *it;
        }
        
        // Draw the engine configuration
        if (drawEngineConfiguration(engine, index)) {
            modified = true;
            
            if (it == selectedEngines_.end()) {
                // Engine was newly selected
                if (engine.selected) {
                    selectedEngines_.push_back(engine);
                }
            } else {
                // Existing engine was modified
                if (engine.selected) {
                    *it = engine;
                } else {
                    // Engine was deselected
                    selectedEngines_.erase(it);
                }
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
        // Gauntlet option (if enabled) - same as original implementation
        if (options_.allowGauntletEdit) {
            return ImGuiControls::checkbox("Gauntlet", config.config.gauntlet());
        }
        
        // Additional options can be added here based on options_
        return false;
    });
    
    if (changed) {
        modified = true;
    }
    
    ImGui::PopID();
    
    return modified;
}



const std::vector<ImGuiEngineSelect::EngineConfiguration>& ImGuiEngineSelect::getSelectedEngines() const {
    return selectedEngines_;
}

void ImGuiEngineSelect::setSelectedEngines(const std::vector<EngineConfiguration>& configurations) {
    selectedEngines_ = configurations;
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
ImGuiEngineSelect::findSelectedEngine(const EngineConfig& engineConfig) {
    return std::find_if(selectedEngines_.begin(), selectedEngines_.end(),
        [&engineConfig](const EngineConfiguration& selected) {
            return selected.config == engineConfig;
        });
}

void ImGuiEngineSelect::notifyConfigurationChanged() {
    if (configurationCallback_) {
        configurationCallback_(selectedEngines_);
    }
}