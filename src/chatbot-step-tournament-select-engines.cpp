/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author GitHub Copilot
 * @copyright Copyright (c) 2025 GitHub Copilot
 */

#include "chatbot-step-tournament-select-engines.h"
#include "configuration.h"
#include "tournament-data.h"
#include "imgui-controls.h"
#include "i18n.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentSelectEngines::ChatbotStepTournamentSelectEngines(ChatbotTournament* thread)
    : thread_(thread) {
    
    const auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
    for (const auto& [key, capability] : capabilities) {
        availableEngines_.push_back({
            capability.getName(),
            capability.getPath(),
            capability.getProtocol(),
            false
        });
    }
}

void ChatbotStepTournamentSelectEngines::draw() {
    if (availableEngines_.empty()) {
        QaplaWindows::ImGuiControls::textWrapped("No engines found. Skipping selection.");
        if (QaplaWindows::ImGuiControls::textButton("Continue")) {
            finished_ = true;
        }
        return;
    }

    QaplaWindows::ImGuiControls::textWrapped("Select engines for the tournament:");
    ImGui::Spacing();

    for (auto& engine : availableEngines_) {
        QaplaWindows::ImGuiControls::checkbox(engine.name.c_str(), engine.selected);
    }

    ImGui::Spacing();

    if (QaplaWindows::ImGuiControls::textButton("Continue")) {
        // Add selected engines to tournament data
        std::vector<ImGuiEngineSelect::EngineConfiguration> configs;
        // First, get existing configs if we want to append? 
        // Or are we starting fresh? 
        // The user might have cleared data in previous step.
        // Let's assume we append or set.
        // TournamentData::instance().engineSelect().getEngineConfigurations()
        
        // Actually, we should probably just add them.
        // But we need to convert EngineEntry to EngineConfiguration.
        
        for (const auto& engine : availableEngines_) {
            if (engine.selected) {
                ImGuiEngineSelect::EngineConfiguration config;
                config.config.setName(engine.name);
                config.config.setCmd(engine.path);
                config.config.setProtocol(engine.protocol);
                config.selected = true;
                config.originalName = engine.name;
                
                // We need to add this to TournamentData.
                // TournamentData::instance().engineSelect().addEngine(config)?
                // ImGuiEngineSelect doesn't have addEngine, it has setEngineConfigurations.
                // So we need to get existing, add new, and set back.
                configs.push_back(config);
            }
        }
        
        if (!configs.empty()) {
             auto currentConfigs = TournamentData::instance().engineSelect().getEngineConfigurations();
             currentConfigs.insert(currentConfigs.end(), configs.begin(), configs.end());
             TournamentData::instance().setEngineConfigurations(currentConfigs);
        }

        finished_ = true;
    }
}

bool ChatbotStepTournamentSelectEngines::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
