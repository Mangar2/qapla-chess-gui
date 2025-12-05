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

#include "chatbot-step-epd-configuration.h"
#include "epd-data.h"
#include "imgui-epd-configuration.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepEpdConfiguration::draw() {
    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            "Configure the EPD analysis settings. Set the analysis time limits "
            "and select an EPD or RAW position file.");
        ImGui::Spacing();
    }

    // Draw EPD configuration
    ImGuiEpdConfiguration epdConfig;
    ImGuiEpdConfiguration::DrawOptions options {
        .alwaysOpen = true,
        .showSeenPlies = true,
        .showMaxTime = true,
        .showMinTime = true,
        .showFilePath = true
    };
    epdConfig.draw(options, 150.0F, 10.0F);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (finished_) {
        return "";
    }

    // Check if configuration is valid (file path must be set)
    bool hasValidConfig = !EpdData::instance().config().filepath.empty();

    ImGui::BeginDisabled(!hasValidConfig);
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

} // namespace QaplaWindows::ChatBot
