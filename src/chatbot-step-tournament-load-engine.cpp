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
#include "snackbar.h"
#include <imgui.h>
#include <format>
#include <cmath>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentLoadEngine::ChatbotStepTournamentLoadEngine() = default;

void ChatbotStepTournamentLoadEngine::draw() {
    if (finished_) {
         drawSummary();
         return;
    }

    switch (state_) {
        case State::Input:
            drawInput();
            break;
        case State::Detecting:
            drawDetecting();
            break;
        case State::Summary:
            drawSummary();
            break;
    }
}

bool ChatbotStepTournamentLoadEngine::isFinished() const {
    return finished_;
}

void ChatbotStepTournamentLoadEngine::drawInput() {
    auto selectedEngines = TournamentData::instance().engineSelect().getSelectedEngines();
    size_t numSelected = selectedEngines.size();
    size_t numAdded = addedEnginePaths_.size();
    size_t totalEngines = numSelected + numAdded;

    if (totalEngines == 0) {
        QaplaWindows::ImGuiControls::textWrapped("No engines selected. You need at least two engines to start a tournament. Please select engines.");
        ImGui::Spacing();
        if (QaplaWindows::ImGuiControls::textButton("Add Engines")) {
            addEngines();
        }
    } else if (totalEngines == 1) {
        QaplaWindows::ImGuiControls::textWrapped("One engine selected. You need at least two engines to start a tournament. Please select at least one more engine.");
        ImGui::Spacing();
        if (QaplaWindows::ImGuiControls::textButton("Add Engines")) {
            addEngines();
        }
        if (!addedEnginePaths_.empty()) {
            ImGui::Spacing();
            QaplaWindows::ImGuiControls::textWrapped("Added Engines:");
            for (const auto& path : addedEnginePaths_) {
                ImGui::Bullet();
                ImGui::SameLine();
                QaplaWindows::ImGuiControls::textWrapped(path);
            }
        }
    } else { // totalEngines >= 2
        QaplaWindows::ImGuiControls::textWrapped("Do you want to load additional engines for the tournament?");
        ImGui::Spacing();
        if (QaplaWindows::ImGuiControls::textButton("Add Engines")) {
            addEngines();
        }
        if (!addedEnginePaths_.empty()) {
            ImGui::Spacing();
            QaplaWindows::ImGuiControls::textWrapped("Added Engines:");
            for (const auto& path : addedEnginePaths_) {
                ImGui::Bullet();
                ImGui::SameLine();
                QaplaWindows::ImGuiControls::textWrapped(path);
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (QaplaWindows::ImGuiControls::textButton("Done & Detect")) {
                startDetection();
                state_ = State::Detecting;
            }
        } else {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (QaplaWindows::ImGuiControls::textButton("Skip / No more engines")) {
                finished_ = true;
            }
        }
    }
}

void ChatbotStepTournamentLoadEngine::addEngines() {
    auto commands = OsDialogs::openFileDialog(true);
    for (auto &command : commands)
    {
        QaplaTester::EngineWorkerFactory::getConfigManagerMutable().addConfig(
            QaplaTester::EngineConfig::createFromPath(command)
        );
        addedEnginePaths_.push_back(command);
    }
    if (!commands.empty()) {
        QaplaConfiguration::Configuration::instance().setModified();
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
        if (detectionStarted_) {
            selectAddedEngines();
            state_ = State::Summary;
            finished_ = true; 
        } else {
            state_ = State::Summary;
            finished_ = true;
        }
    }
}

void ChatbotStepTournamentLoadEngine::selectAddedEngines() {
    auto& engineSelect = TournamentData::instance().engineSelect();
    auto configs = engineSelect.getEngineConfigurations();
    
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    
    for (const auto& path : addedEnginePaths_) {
        auto allConfigs = configManager.getAllConfigs();
        for (const auto& globalConfig : allConfigs) {
            if (globalConfig.getCmd() == path) {
                bool foundInTournament = false;
                for (auto& tConfig : configs) {
                    if (tConfig.config.getCmd() == globalConfig.getCmd() && 
                        tConfig.config.getProtocol() == globalConfig.getProtocol()) {
                        tConfig.selected = true;
                        foundInTournament = true;
                        break;
                    }
                }
                
                if (!foundInTournament) {
                    ImGuiEngineSelect::EngineConfiguration newConfig;
                    newConfig.config = globalConfig;
                    newConfig.selected = true;
                    newConfig.originalName = globalConfig.getName();
                    configs.push_back(newConfig);
                }
            }
        }
    }
    
    engineSelect.setEngineConfigurations(configs);
}

void ChatbotStepTournamentLoadEngine::drawSummary() {
    QaplaWindows::ImGuiControls::textWrapped("Engine detection complete.");
    ImGui::Spacing();
    QaplaWindows::ImGuiControls::textWrapped("Selected engines for tournament:");
    
    const auto& configs = TournamentData::instance().engineSelect().getEngineConfigurations();
    for (const auto& config : configs) {
        if (config.selected) {
            ImGui::Bullet();
            ImGui::SameLine();
            std::string text = std::format("{} ({})", config.config.getName(), QaplaTester::to_string(config.config.getProtocol()));
            QaplaWindows::ImGuiControls::textWrapped(text);
        }
    }
}

} // namespace QaplaWindows::ChatBot
