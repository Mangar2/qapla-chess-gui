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

#include "chatbot-step-tournament-global-settings.h"
#include "tournament-data.h"
#include "sprt-tournament-data.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentGlobalSettings::ChatbotStepTournamentGlobalSettings(TournamentType type)
    : type_(type) {}

ImGuiEngineGlobalSettings& ChatbotStepTournamentGlobalSettings::getGlobalSettings() {
    if (type_ == TournamentType::Sprt) {
        return SprtTournamentData::instance().globalSettings();
    }
    return TournamentData::instance().globalSettings();
}

std::string ChatbotStepTournamentGlobalSettings::draw() {

    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            "You can configure global engine settings such as hash size and time control here. "
            "These settings will apply to all engines participating in the tournament.");
        ImGui::Spacing();
    }

    auto& globalSettings = getGlobalSettings();
    
    auto savedOptions = globalSettings.getOptions();
    
    // Set simplified options for chatbot
    auto options = savedOptions;
    options.showTrace = false;
    options.showRestart = false;
    options.showUseCheckboxes = false;
    options.alwaysOpen = true;
    globalSettings.setOptions(options);

    globalSettings.drawGlobalSettings();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    globalSettings.drawTimeControl({ .controlWidth = 150.0F, .controlIndent = 10.0F }, true);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    globalSettings.setOptions(savedOptions);

    if (finished_) {
        return "";
    }


    if (QaplaWindows::ImGuiControls::textButton("Continue")) {
        finished_ = true;
    }

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }

    return "";
}

} // namespace QaplaWindows::ChatBot
