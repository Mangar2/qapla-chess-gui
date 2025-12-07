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

#include "chatbot-step-tournament-load-engine.h"
#include "imgui-controls.h"
#include "os-dialogs.h"
#include "engine-worker-factory.h"
#include "configuration.h"
#include "tournament-data.h"
#include "sprt-tournament-data.h"
#include "epd-data.h"
#include "snackbar.h"
#include <imgui.h>
#include <format>
#include <cmath>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentLoadEngine::ChatbotStepTournamentLoadEngine(
    EngineSelectContext context, size_t minEngines)
    : context_(context), minEngines_(minEngines) {}

ImGuiEngineSelect& ChatbotStepTournamentLoadEngine::getEngineSelect() {
    if (context_ == EngineSelectContext::EpdAnalysis) {
        return EpdData::instance().engineSelect();
    }
    if (context_ == EngineSelectContext::SPRT) {
        return SprtTournamentData::instance().engineSelect();
    }
    return TournamentData::instance().engineSelect();
}

const char* ChatbotStepTournamentLoadEngine::getContextName() const {
    return context_ == EngineSelectContext::EpdAnalysis ? "analysis" : "tournament";
}

std::string ChatbotStepTournamentLoadEngine::draw() {
    if (finished_) {
         return result_;
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

void ChatbotStepTournamentLoadEngine::drawInput() {
    auto selectedEngines = getEngineSelect().getSelectedEngines();
    size_t numSelected = selectedEngines.size();
    showAddedEngines();

    // We automatically select engines that were added thus numSelected includes them
    if (numSelected == 0) {
        QaplaWindows::ImGuiControls::textWrapped(
            std::format("No engines selected. You need at least {} engine{} to start {}. Please select engines.",
                minEngines_, minEngines_ == 1 ? "" : "s", getContextName()).c_str());
        ImGui::Spacing();
        if (QaplaWindows::ImGuiControls::textButton("Add Engines")) {
            addEngines();
        }
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
            result_ = "stop";
        }
    } else if (numSelected < minEngines_) {
        QaplaWindows::ImGuiControls::textWrapped(
            std::format("{} engine{} selected. You need at least {} to start {}. Please select at least {} more engine{}.",
                numSelected, numSelected == 1 ? "" : "s",
                minEngines_, getContextName(),
                minEngines_ - numSelected, (minEngines_ - numSelected) == 1 ? "" : "s").c_str());
        ImGui::Spacing();
        if (QaplaWindows::ImGuiControls::textButton("Add Engines")) {
            addEngines();
        }
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
            result_ = "stop";
        }
    } else { // numSelected >= minEngines_
        QaplaWindows::ImGuiControls::textWrapped(
            std::format("Do you want to load additional engines for the {}?", getContextName()).c_str());
        ImGui::Spacing();
        if (QaplaWindows::ImGuiControls::textButton("Add Engines")) {
            addEngines();
        }
        
        bool needsDetection = !ImGuiEngineSelect::areAllEnginesDetected();
        
        if (needsDetection) {
            ImGui::SameLine();
            if (QaplaWindows::ImGuiControls::textButton("Detect & Continue")) {
                startDetection();
                state_ = State::Detecting;
            }
        }
        
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton(needsDetection ? "Skip Detection" : "Continue")) {
            finished_ = true;
        }
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
            result_ = "stop";
        }
    }
}

void ChatbotStepTournamentLoadEngine::showAddedEngines()
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

void ChatbotStepTournamentLoadEngine::addEngines() {
    auto& engineSelect = getEngineSelect();
    auto added = engineSelect.addEngines(true);
    for (const auto& path : added) {
        addedEnginePaths_.push_back(path);
    }
}

void ChatbotStepTournamentLoadEngine::startDetection() {
    try {
        QaplaConfiguration::Configuration::instance().getEngineCapabilities().autoDetect();
        detectionStarted_ = true;
    } catch (const std::exception& ex) {
        SnackbarManager::instance().showWarning(
            std::format("Engine auto-detect failed,\nsome engines may not be detected\n {}", ex.what()));
    }
    QaplaConfiguration::Configuration::instance().setModified();
}

void ChatbotStepTournamentLoadEngine::drawDetecting() {
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
