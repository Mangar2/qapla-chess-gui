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
#include "tournament-data.h"
#include "configuration.h"
#include "imgui-controls.h"
#include "i18n.h"
#include "os-dialogs.h"
#include "engine-capability.h"
#include <imgui.h>
#include <filesystem>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentLoadEngine::ChatbotStepTournamentLoadEngine(ChatbotTournament* thread)
    : thread_(thread) {
}

void ChatbotStepTournamentLoadEngine::draw() {
    QaplaWindows::ImGuiControls::textWrapped("Do you want to load another engine from disk?");
    ImGui::Spacing();

    if (QaplaWindows::ImGuiControls::textButton("Yes, Load Engine")) {
        auto paths = OsDialogs::openFileDialog(false, {{"Executables", "*.exe"}, {"All Files", "*.*"}});
        if (!paths.empty() && !paths[0].empty()) {
            std::string path = paths[0];
            std::string name = std::filesystem::path(path).stem().string();
            
            // Create capability and add to configuration
            // We assume UCI for now or try to detect? 
            // EngineCapability::createFromPath?
            // Let's assume UCI as default or ask? 
            // For simplicity, we assume UCI.
            QaplaConfiguration::EngineCapability capability;
            capability.setPath(path);
            capability.setName(name);
            capability.setProtocol(QaplaTester::EngineProtocol::Uci); // Default
            
            QaplaConfiguration::Configuration::instance().getEngineCapabilities().addOrReplace(capability);
            
            // Add to tournament
            ImGuiEngineSelect::EngineConfiguration config;
            config.config.setName(name);
            config.config.setCmd(path);
            config.config.setProtocol(QaplaTester::EngineProtocol::Uci);
            config.selected = true;
            config.originalName = name;
            
            auto currentConfigs = TournamentData::instance().engineSelect().getEngineConfigurations();
            currentConfigs.push_back(config);
            TournamentData::instance().setEngineConfigurations(currentConfigs);
            
            // We stay in this step to allow adding more.
        }
    }

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("No, Continue")) {
        finished_ = true;
    }
}

bool ChatbotStepTournamentLoadEngine::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
