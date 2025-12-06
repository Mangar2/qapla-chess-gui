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

#include "chatbot-step-sprt-configuration.h"
#include "sprt-tournament-data.h"
#include "imgui-sprt-configuration.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepSprtConfiguration::draw() {
    drawConfiguration();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (finished_) {
        return "";
    }

    bool canContinue = isConfigurationValid();

    ImGui::BeginDisabled(!canContinue);
    if (QaplaWindows::ImGuiControls::textButton("Continue")) {
        finished_ = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    const char* optionsLabel = showMoreOptions_ ? "Less Options" : "More Options";
    if (QaplaWindows::ImGuiControls::textButton(optionsLabel)) {
        showMoreOptions_ = !showMoreOptions_;
    }

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }

    return "";
}

void ChatbotStepSprtConfiguration::drawConfiguration() {
    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            "Configure the SPRT (Sequential Probability Ratio Test) parameters. "
            "These settings define when the test concludes that the engine under test "
            "is stronger, weaker, or inconclusive compared to the reference engine.");
        ImGui::Spacing();
    }

    auto& sprtConfiguration = SprtTournamentData::instance().sprtConfiguration();
    
    ImGuiSprtConfiguration::DrawOptions options;
    options.inputWidth = 150.0F;
    options.indent = 10.0F;
    options.alwaysOpen = true;
    options.showCollapsingHeader = false;
    options.showEloLower = true;
    options.showEloUpper = true;
    options.showAlpha = showMoreOptions_;
    options.showBeta = showMoreOptions_;
    options.showMaxGames = true;

    sprtConfiguration.draw(options);
    
}

bool ChatbotStepSprtConfiguration::isConfigurationValid() const {
    return SprtTournamentData::instance().sprtConfiguration().isValid();
}

} // namespace QaplaWindows::ChatBot
