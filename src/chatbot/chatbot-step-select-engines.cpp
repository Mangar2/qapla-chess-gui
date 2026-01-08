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

#include "chatbot-step-select-engines.h"
#include "imgui-engine-select.h"
#include "imgui-controls.h"
#include "i18n.h"
#include "engine-handling/engine-worker-factory.h"
#include <imgui.h>

using QaplaTester::EngineWorkerFactory;

namespace QaplaWindows::ChatBot {

ChatbotStepSelectEngines::ChatbotStepSelectEngines(
    EngineSelectProvider provider,
    const char* contextName)
    : provider_(std::move(provider)), contextName_(contextName) {}

ChatbotStepSelectEngines::~ChatbotStepSelectEngines() {
}

ImGuiEngineSelect* ChatbotStepSelectEngines::getEngineSelect() {
    return provider_();
}

std::string ChatbotStepSelectEngines::draw() {

    auto* engineSelect = getEngineSelect();
    
    // Check if target still exists (e.g., board not closed)
    if (engineSelect == nullptr) {
        QaplaWindows::ImGuiControls::textWrapped("Error: Target no longer exists.");
        finished_ = true;
        return "stop";
    }

    // Prüfe, ob Engines verfügbar sind
    auto& configManager = EngineWorkerFactory::getConfigManager();
    auto configs = configManager.getAllConfigs();
    if (configs.empty()) {
        finished_ = true;
        return "";
    }

    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            std::format("Select engines for the {}:", contextName_).c_str());
    } else {
        QaplaWindows::ImGuiControls::textWrapped(
            std::format("Selected engines for the {}:", contextName_).c_str());
    }
    ImGui::Spacing();

    auto options = engineSelect->getOptions();

    if (!showMoreOptions_) {
        // Simplified view: single selection, no engine configuration editing
        auto& mutableOptions = engineSelect->getOptions();
        mutableOptions.allowMultipleSelection = false;
        mutableOptions.directEditMode = true;
        mutableOptions.allowEngineConfiguration = false;
    } else {
        // Advanced view: use full engine selection with configuration editing and multi-select
        auto& mutableOptions = engineSelect->getOptions();
        mutableOptions.directEditMode = true;
        mutableOptions.allowEngineConfiguration = true;
        mutableOptions.allowMultipleSelection = true;
    }
    ImGui::PushID("tutorial");
    engineSelect->draw();
    ImGui::PopID();
    engineSelect->setOptions(options); // Restore options

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

} // namespace QaplaWindows::ChatBot
