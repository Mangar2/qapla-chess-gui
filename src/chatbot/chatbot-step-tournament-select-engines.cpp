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
#include "tournament-data.h"
#include "imgui-controls.h"
#include "i18n.h"
#include "engine-worker-factory.h"
#include <imgui.h>

using QaplaTester::EngineWorkerFactory;

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentSelectEngines::ChatbotStepTournamentSelectEngines() = default;

ChatbotStepTournamentSelectEngines::~ChatbotStepTournamentSelectEngines() {
    TournamentData::instance().engineSelect().setAlwaysShowEngines(false);
}

std::string ChatbotStepTournamentSelectEngines::draw() {

    auto& engineSelect = TournamentData::instance().engineSelect();

    // Prüfe, ob Engines verfügbar sind
    auto& configManager = EngineWorkerFactory::getConfigManager();
    auto configs = configManager.getAllConfigs();
    if (configs.empty()) {
        finished_ = true;
        return "";
    }

    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped("Select engines for the tournament:");
    } else {
        QaplaWindows::ImGuiControls::textWrapped("Selected engines for the tournament:");
    }
    ImGui::Spacing();

    auto options = engineSelect.getOptions();
    engineSelect.getOptions().allowMultipleSelection = false; // Simplifies engine selection
    engineSelect.getOptions().directEditMode = true;          // Scips the collapsing header
    engineSelect.getOptions().allowEngineConfiguration = false; // Simplefies the UI
    engineSelect.draw();
    engineSelect.setOptions(options); // Restore options

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (finished_) {
        return "";
    }

    if (QaplaWindows::ImGuiControls::textButton("Continue")) {
        finished_ = true;
        return "";
    }

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }

    return "";

}

} // namespace QaplaWindows::ChatBot
