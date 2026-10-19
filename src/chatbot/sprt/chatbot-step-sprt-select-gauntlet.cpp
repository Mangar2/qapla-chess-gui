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

#include "chatbot-step-sprt-select-gauntlet.h"
#include "sprt-tournament-data.h"
#include "imgui-controls.h"
#include "ui/qapla-ui.h"

namespace QaplaWindows::ChatBot {

std::string ChatbotStepSprtSelectGauntlet::draw() {
    drawGauntletSelection();
    
    QaplaUi::spacing();
    QaplaUi::separator();
    QaplaUi::spacing();

    if (finished_) {
        return "";
    }

    // Check if we can continue - need exactly one gauntlet engine selected
    bool canContinue = hasValidGauntletSelection();
    
    QaplaUi::beginDisabled(!canContinue);
    if (QaplaWindows::ImGuiControls::textButton("Continue")) {
        finished_ = true;
    }
    QaplaUi::endDisabled();
    
    if (!canContinue && !finished_) {
        QaplaUi::sameLine();
        QaplaUi::textColored(QaplaUi::Vec4(1.0F, 0.5F, 0.0F, 1.0F), "Please select the engine under test");
    }

    QaplaUi::sameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }

    return "";
}

void ChatbotStepSprtSelectGauntlet::drawGauntletSelection() {
    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            "Select which engine should be tested (gauntlet engine). "
            "This is the engine whose performance will be measured. "
            "The other engine will be used as the comparison baseline.");
        QaplaUi::spacing();
    }

    auto& engineSelect = SprtTournamentData::instance().getEngineSelect();
    auto selectedEngines = engineSelect.getSelectedEngines();
    
    if (selectedEngines.size() != 2) {
        QaplaWindows::ImGuiControls::textWrapped("Error: SPRT tournament requires exactly 2 engines.");
        return;
    }

    // Find current gauntlet index
    int currentGauntletIndex = findCurrentGauntletIndex();
    
    // If no gauntlet is set yet, default to first engine
    if (currentGauntletIndex < 0) {
        applyGauntletSelection(0);
        currentGauntletIndex = 0;
    }

    // Build combo box items
    std::vector<std::string> engineNames;
    engineNames.push_back(selectedEngines[0].getName());
    engineNames.push_back(selectedEngines[1].getName());

    // Determine display text
    const char* previewText = (currentGauntletIndex >= 0 && currentGauntletIndex < 2)
        ? engineNames[currentGauntletIndex].c_str()
        : "-- Select Engine Under Test --";

    QaplaUi::text("Engine Under Test:");
    QaplaUi::sameLine();
    QaplaUi::setNextItemWidth(300.0F);
    if (QaplaUi::beginCombo("##GauntletEngine", previewText)) {
        for (int idx = 0; idx < 2; ++idx) {
            bool isSelected = (currentGauntletIndex == idx);
            if (QaplaUi::selectable(engineNames[idx].c_str(), isSelected)) {
                if (idx != currentGauntletIndex) {
                    applyGauntletSelection(idx);
                }
            }
            if (isSelected) {
                QaplaUi::setItemDefaultFocus();
            }
        }
        QaplaUi::endCombo();
    }
    
    QaplaUi::spacing();
    
    // Show which engine is the comparison baseline
    int comparisonIndex = (currentGauntletIndex == 0) ? 1 : 0;
    if (currentGauntletIndex >= 0 && currentGauntletIndex < 2) {
        QaplaUi::textColored(QaplaUi::Vec4(0.6F, 0.6F, 0.6F, 1.0F), 
            "Comparison Engine: " + engineNames[comparisonIndex]);
    }
}

void ChatbotStepSprtSelectGauntlet::applyGauntletSelection(int selectedIndex) {
    auto& engineSelect = SprtTournamentData::instance().getEngineSelect();
    auto configurations = engineSelect.getEngineConfigurations();
    auto selectedEngines = engineSelect.getSelectedEngines();
    
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(selectedEngines.size())) {
        return;
    }
    
    // Get the name of the selected gauntlet engine
    std::string gauntletEngineName = selectedEngines[selectedIndex].getName();

    // Update all configurations: set gauntlet for the selected one, clear for others
    for (auto& config : configurations) {
        if (config.isSelected() && config.getName() == gauntletEngineName) {
            config.setGauntlet(true);
        } else {
            config.setGauntlet(false);
        }
    }
    
    engineSelect.setEngineConfigurations(configurations);
}

int ChatbotStepSprtSelectGauntlet::findCurrentGauntletIndex() const {
    auto selectedEngines = SprtTournamentData::instance().getEngineSelect().getSelectedEngines();
    
    for (int idx = 0; idx < static_cast<int>(selectedEngines.size()); ++idx) {
        if (selectedEngines[idx].isGauntlet()) {
            return idx;
        }
    }
    
    return -1;  // No gauntlet selected
}

bool ChatbotStepSprtSelectGauntlet::hasValidGauntletSelection() const {
    return findCurrentGauntletIndex() >= 0;
}

} // namespace QaplaWindows::ChatBot
