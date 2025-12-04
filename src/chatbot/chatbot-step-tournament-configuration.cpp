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
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "chatbot-step-tournament-configuration.h"
#include "tournament-data.h"
#include "imgui-controls.h"
#include "tournament.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentConfiguration::ChatbotStepTournamentConfiguration() = default;

std::string ChatbotStepTournamentConfiguration::draw() {
    // Always draw the configuration
    drawConfiguration();
    
    // Draw gauntlet selection if in gauntlet mode
    if (isGauntletMode()) {
        drawGauntletSelection();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (finished_) {
        return "";
    }

    // Check if we can continue
    bool canContinue = !isGauntletMode() || hasValidGauntletSelection();
    
    ImGui::BeginDisabled(!canContinue);
    if (QaplaWindows::ImGuiControls::textButton("Continue")) {
        finished_ = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }

    return "";
}

void ChatbotStepTournamentConfiguration::drawConfiguration() {
    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            "Configure the tournament settings. Choose the tournament type, "
            "number of rounds, and games per pairing.");
        ImGui::Spacing();
    }

    auto& tournamentConfig = TournamentData::instance().tournamentConfiguration();
    
    // Draw options for chatbot - simplified view, with more options toggle
    ImGuiTournamentConfiguration::DrawOptions options {
        .alwaysOpen = true,
        .showEvent = true,         
        .showType = true,          // Important: gauntlet vs round-robin
        .showRounds = true,        // How many rounds
        .showGamesPerPairing = true, // Games per pairing
        .showSameOpening = showMoreOptions_,   // Additional option
        .showNoColorSwap = showMoreOptions_,   // Advanced option
        .showAverageElo = showMoreOptions_     // Advanced option
    };

    tournamentConfig.draw(options, 150.0F, 10.0F);

    // More/Less Options button
    if (!finished_) {
        ImGui::Spacing();
        const char* optionsLabel = showMoreOptions_ ? "Less Options" : "More Options";
        if (QaplaWindows::ImGuiControls::textButton(optionsLabel)) {
            showMoreOptions_ = !showMoreOptions_;
        }
    }
}

void ChatbotStepTournamentConfiguration::drawGauntletSelection() {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            "Select the gauntlet engine (the engine that will play against all others):");
        ImGui::Spacing();
    }

    auto selectedEngines = TournamentData::instance().engineSelect().getSelectedEngines();
    
    if (selectedEngines.empty()) {
        QaplaWindows::ImGuiControls::textWrapped("No engines selected for the tournament.");
        return;
    }

    // Find current gauntlet index
    int currentGauntletIndex = findCurrentGauntletIndex();

    // Build combo box items
    std::vector<std::string> engineNames;
    for (const auto& engine : selectedEngines) {
        engineNames.push_back(engine.config.getName());
    }

    // Determine display text
    const char* previewText = (currentGauntletIndex >= 0 && currentGauntletIndex < static_cast<int>(engineNames.size()))
        ? engineNames[currentGauntletIndex].c_str()
        : "-- Select Engine --";

    ImGui::SetNextItemWidth(300.0F);
    if (ImGui::BeginCombo("Gauntlet Engine", previewText)) {
        for (int i = 0; i < static_cast<int>(engineNames.size()); ++i) {
            bool isSelected = (currentGauntletIndex == i);
            if (ImGui::Selectable(engineNames[i].c_str(), isSelected)) {
                if (i != currentGauntletIndex) {
                    applyGauntletSelection(i);
                }
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

void ChatbotStepTournamentConfiguration::applyGauntletSelection(int selectedIndex) {
    auto& engineSelect = TournamentData::instance().engineSelect();
    auto configurations = engineSelect.getEngineConfigurations();
    auto selectedEngines = engineSelect.getSelectedEngines();
    
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(selectedEngines.size())) {
        return;
    }
    
    // Get the name of the selected gauntlet engine
    std::string gauntletEngineName = selectedEngines[selectedIndex].config.getName();
    
    // Update all configurations: set gauntlet for the selected one, clear for others
    for (auto& config : configurations) {
        if (config.selected && config.config.getName() == gauntletEngineName) {
            config.config.setGauntlet(true);
        } else {
            config.config.setGauntlet(false);
        }
    }
    
    engineSelect.setEngineConfigurations(configurations);
}

int ChatbotStepTournamentConfiguration::findCurrentGauntletIndex() const {
    auto selectedEngines = TournamentData::instance().engineSelect().getSelectedEngines();
    
    for (int i = 0; i < static_cast<int>(selectedEngines.size()); ++i) {
        if (selectedEngines[i].config.isGauntlet()) {
            return i;
        }
    }
    
    return -1;  // No gauntlet selected
}

bool ChatbotStepTournamentConfiguration::isGauntletMode() const {
    auto& tournamentConfig = TournamentData::instance().tournamentConfiguration();
    return tournamentConfig.config().type == "gauntlet";
}

bool ChatbotStepTournamentConfiguration::hasValidGauntletSelection() const {
    return findCurrentGauntletIndex() >= 0;
}

} // namespace QaplaWindows::ChatBot
