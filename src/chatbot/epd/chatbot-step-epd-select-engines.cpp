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

#include "chatbot-step-epd-select-engines.h"
#include "epd-data.h"
#include "imgui-controls.h"
#include "engine-worker-factory.h"
#include <imgui.h>

using QaplaTester::EngineWorkerFactory;

namespace QaplaWindows::ChatBot {

ChatbotStepEpdSelectEngines::~ChatbotStepEpdSelectEngines() {
    EpdData::instance().engineSelect().setAlwaysShowEngines(false);
}

std::string ChatbotStepEpdSelectEngines::draw() {

    auto& engineSelect = EpdData::instance().engineSelect();

    // Check if engines are available
    auto& configManager = EngineWorkerFactory::getConfigManager();
    auto configs = configManager.getAllConfigs();
    if (configs.empty()) {
        finished_ = true;
        return "";
    }

    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped("Select engines for the EPD analysis:");
    } else {
        QaplaWindows::ImGuiControls::textWrapped("Selected engines for the EPD analysis:");
    }
    ImGui::Spacing();

    ImGui::PushID("epdEngineSelect");
    auto options = engineSelect.getOptions();
    engineSelect.getOptions().allowMultipleSelection = false;   // Use simple checkbox selection
    engineSelect.getOptions().directEditMode = true;            // Skips the collapsing header
    engineSelect.getOptions().allowEngineConfiguration = false; // Simplifies the UI
    engineSelect.draw();
    engineSelect.setOptions(options); // Restore options
    ImGui::PopID();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (finished_) {
        return "";
    }

    // Check if at least one engine is selected
    bool hasEngineSelected = !EpdData::instance().config().engines.empty();

    ImGui::BeginDisabled(!hasEngineSelected);
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
