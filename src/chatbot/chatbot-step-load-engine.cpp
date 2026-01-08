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

#include "chatbot-step-load-engine.h"
#include "imgui-engine-select.h"
#include "imgui-controls.h"
#include "os-dialogs.h"
#include "configuration.h"
#include "snackbar.h"

#include <engine-handling/engine-worker-factory.h>

#include <imgui.h>
#include <format>
#include <cmath>

namespace QaplaWindows::ChatBot {

ChatbotStepLoadEngine::ChatbotStepLoadEngine(
    EngineSelectProvider provider,
    size_t minEngines,
    const char* contextName)
    : provider_(std::move(provider)),
      minEngines_(minEngines),
      contextName_(contextName) {}

ImGuiEngineSelect* ChatbotStepLoadEngine::getEngineSelect() {
    return provider_();
}

std::string ChatbotStepLoadEngine::draw() {
    if (finished_) {
         return result_;
    }
    
    // Check if target still exists
    auto* engineSelect = getEngineSelect();
    if (engineSelect == nullptr) {
        QaplaWindows::ImGuiControls::textWrapped("Error: Target no longer exists.");
        finished_ = true;
        return "stop";
    }

    switch (state_) {
        case State::Input:
            drawInput();
            break;
        case State::Detecting:
            drawDetecting();
            break;
        default:
            // Intentionally left blank
            break;
    }
    return result_;
}

void ChatbotStepLoadEngine::drawInput() {
    auto* engineSelect = getEngineSelect();
    if (engineSelect == nullptr) {
        finished_ = true;
        result_ = "stop";
        return;
    }
    
    auto selectedEngines = engineSelect->getSelectedEngines();
    size_t numSelected = selectedEngines.size();
    showAddedEngines();

    // We automatically select engines that were added thus numSelected includes them
    if (numSelected == 0) {
        if (minEngines_ > 0) {
            QaplaWindows::ImGuiControls::textWrapped(
                std::format("No engines added. You need at least {} engine{} to start {}. Please add engines.",
                    minEngines_, minEngines_ == 1 ? "" : "s", contextName_).c_str());
        } else {
            QaplaWindows::ImGuiControls::textWrapped(
                "No engines added. You can add engines now if you like.");
        }
        ImGui::Spacing();
        if (QaplaWindows::ImGuiControls::textButton("Add Engines")) {
            addEngines();
        }
        QaplaWindows::ImGuiControls::hooverTooltip("Open a file dialog to add one or more engine binaries to the engine list.");
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
            result_ = "stop";
        }
    } else if (numSelected < minEngines_) {
        QaplaWindows::ImGuiControls::textWrapped(
            std::format("{} engine{} selected. You need at least {} to start {}. Please select at least {} more engine{}.",
                numSelected, numSelected == 1 ? "" : "s",
                minEngines_, contextName_,
                minEngines_ - numSelected, (minEngines_ - numSelected) == 1 ? "" : "s").c_str());
        ImGui::Spacing();
        if (QaplaWindows::ImGuiControls::textButton("Add Engines")) {
            addEngines();
        }
        QaplaWindows::ImGuiControls::hooverTooltip("Open a file dialog to add one or more engine binaries to the engine list.");
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
            result_ = "stop";
        }
    } else { // numSelected >= minEngines_
        QaplaWindows::ImGuiControls::textWrapped(
            std::format("Do you want to load additional engines for the {}?", contextName_).c_str());
        ImGui::Spacing();
        if (QaplaWindows::ImGuiControls::textButton("Add Engines")) {
            addEngines();
        }
        QaplaWindows::ImGuiControls::hooverTooltip("Open a file dialog to add one or more engine binaries to the engine list.");
        
        bool needsDetection = !ImGuiEngineSelect::areAllEnginesDetected();
        
        if (needsDetection) {
            ImGui::SameLine();
            if (QaplaWindows::ImGuiControls::textButton("Detect & Continue")) {
                startDetection();
                state_ = State::Detecting;
            }
            QaplaWindows::ImGuiControls::hooverTooltip("Automatically detect engine capabilities (options) now and continue.");
        }
        
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton(needsDetection ? "Skip Detection" : "Continue")) {
            finished_ = true;
        }
        if (needsDetection) {
            QaplaWindows::ImGuiControls::hooverTooltip("Skip automatic engine detection and continue without detected capabilities. Use this only if detection fails.");
        }
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
            result_ = "stop";
        }
    }
}

void ChatbotStepLoadEngine::showAddedEngines()
{
    if (addedEnginePaths_.empty()) {
        return;
    }
    ImGui::Spacing();
    QaplaWindows::ImGuiControls::textWrapped("Added Engines:");
    for (const auto &path : addedEnginePaths_)
    {
        ImGui::Bullet();
        ImGui::SameLine();
        QaplaWindows::ImGuiControls::textWrapped(path);
    }
}

void ChatbotStepLoadEngine::addEngines() {
    auto* engineSelect = getEngineSelect();
    if (engineSelect == nullptr) {
        return;
    }
    auto added = engineSelect->addEngines(true);
    for (const auto& path : added) {
        addedEnginePaths_.push_back(path);
    }
}

void ChatbotStepLoadEngine::startDetection() {
    try {
        QaplaConfiguration::Configuration::instance().getEngineCapabilities().autoDetect();
        detectionStarted_ = true;
    } catch (const std::exception& ex) {
        SnackbarManager::instance().showWarning(
            std::format("Engine auto-detect failed,\nsome engines may not be detected\n {}", ex.what()));
    }
    QaplaConfiguration::Configuration::instance().setModified();
}

void ChatbotStepLoadEngine::drawDetecting() {
    QaplaWindows::ImGuiControls::textWrapped("We are now checking the engines and reading their options (auto-detect)...");
    
    bool detecting = QaplaConfiguration::Configuration::instance().getEngineCapabilities().isDetecting();
    
    if (detecting) {
        // Indeterminate progress bar
        float progress = (float)std::sin(ImGui::GetTime() * 3.0f) * 0.5f + 0.5f;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "Detecting...");
    } else {
        state_ = State::Summary;
        finished_ = true;
    }
}


} // namespace QaplaWindows::ChatBot
