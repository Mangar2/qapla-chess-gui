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
#include "sprt-manager.h"
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

    auto& sprtConfig = SprtTournamentData::instance().sprtConfig();
    constexpr float inputWidth = 150.0F;

    ImGui::Indent(10.0F);

    // Elo Lower (H0)
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiControls::inputInt<int>("Elo Lower (H0)", sprtConfig.eloLower, -1000, 1000);
    ImGuiControls::hooverTooltip("Lower Elo bound (H0): null hypothesis threshold for SPRT test.\n"
                                  "If true Elo difference is below this, H0 is accepted (no improvement).");
    
    // Elo Upper (H1)
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiControls::inputInt<int>("Elo Upper (H1)", sprtConfig.eloUpper, -1000, 1000);
    ImGuiControls::hooverTooltip("Upper Elo bound (H1): alternative hypothesis threshold for SPRT test.\n"
                                  "If true Elo difference is above this, H1 is accepted (improvement confirmed).");

    // Validation hint for Elo bounds
    if (sprtConfig.eloLower >= sprtConfig.eloUpper) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::ERROR_COLOR);
        QaplaWindows::ImGuiControls::textWrapped("Elo Lower must be less than Elo Upper.");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    
    // Alpha
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiControls::inputPromille("Alpha (\xe2\x80\xb0)", sprtConfig.alpha, 0.001, 0.5, 0.001);
    ImGuiControls::hooverTooltip("Type I error rate (false positive): probability of rejecting H0 when it's true.\n"
                                  "Lower values mean more confidence but require more games.");
    ImGui::SameLine();
    ImGui::Text("(%.3f)", sprtConfig.alpha);
    
    // Beta
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiControls::inputPromille("Beta (\xe2\x80\xb0)", sprtConfig.beta, 0.001, 0.5, 0.001);
    ImGuiControls::hooverTooltip("Type II error rate (false negative): probability of accepting H0 when H1 is true.\n"
                                  "Lower values mean more confidence but require more games.");
    ImGui::SameLine();
    ImGui::Text("(%.3f)", sprtConfig.beta);

    ImGui::Spacing();
    
    // Max Games
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiControls::inputInt<uint32_t>("Max Games", sprtConfig.maxGames, 1, 1000000);
    ImGuiControls::hooverTooltip("Maximum number of games before test terminates inconclusively.\n"
                                  "If neither H0 nor H1 is accepted within this limit, the result is inconclusive.");

    ImGui::Unindent(10.0F);
}

bool ChatbotStepSprtConfiguration::isConfigurationValid() const {
    const auto& sprtConfig = SprtTournamentData::instance().sprtConfig();
    
    // Elo Lower must be less than Elo Upper
    if (sprtConfig.eloLower >= sprtConfig.eloUpper) {
        return false;
    }
    
    // Alpha and Beta must be positive
    if (sprtConfig.alpha <= 0.0 || sprtConfig.beta <= 0.0) {
        return false;
    }
    
    // Max Games must be at least 1
    if (sprtConfig.maxGames < 1) {
        return false;
    }
    
    return true;
}

} // namespace QaplaWindows::ChatBot
