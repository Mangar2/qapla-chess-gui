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

#include "chatbot-step-tournament-menu.h"
#include "tournament-data.h"
#include "sprt-tournament-data.h"
#include "imgui-controls.h"
#include "os-dialogs.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentMenu::ChatbotStepTournamentMenu(EngineSelectContext type)
    : type_(type) {}

void ChatbotStepTournamentMenu::clearTournament() {
    if (type_ == EngineSelectContext::SPRT) {
        SprtTournamentData::instance().clear();
    } else {
        TournamentData::instance().clear(false);
    }
}

void ChatbotStepTournamentMenu::saveTournament(const std::string& path) {
    if (type_ == EngineSelectContext::SPRT) {
        SprtTournamentData::saveTournament(path);
    } else {
        TournamentData::saveTournament(path);
    }
}

std::pair<std::string, std::string> ChatbotStepTournamentMenu::getFileFilter() const {
    if (type_ == EngineSelectContext::SPRT) {
        return {"Qapla SPRT Files", "qsprt"};
    }
    return {"Qapla Tournament Files", "qtour"};
}

const char* ChatbotStepTournamentMenu::getTournamentName() const {
    return (type_ == EngineSelectContext::SPRT) ? "SPRT tournament" : "tournament";
}

std::string ChatbotStepTournamentMenu::draw() {
    if (finished_) {
        return "";
    }

    QaplaWindows::ImGuiControls::textWrapped("What would you like to do?\n");
    if (!saved_) {
        ImGui::Spacing();
        std::string warningText = std::string("Starting a new ") + getTournamentName() + 
            " will delete the existing one.\n"
            "Save the current " + getTournamentName() + " to a file if you want to keep it.";
        QaplaWindows::ImGuiControls::textWrapped(warningText.c_str());
    }
    
    ImGui::Spacing();
    ImGui::Spacing();

    std::string newButtonLabel = std::string("New ") + getTournamentName();
    if (QaplaWindows::ImGuiControls::textButton(newButtonLabel.c_str())) {
        clearTournament();
        finished_ = true;
        return "new";
    }
    QaplaWindows::ImGuiControls::hooverTooltip("Create a new tournament. The currently loaded tournament will be discarded unless you save it first.");

    ImGui::SameLine();

    std::string saveButtonLabel = std::string("Save ") + getTournamentName();
    if (QaplaWindows::ImGuiControls::textButton(saveButtonLabel.c_str())) {
        auto filter = getFileFilter();
        auto path = OsDialogs::saveFileDialog({{filter.first, filter.second}});
        if (!path.empty()) {
            saveTournament(path);
            saved_ = true;
        }
    }
    QaplaWindows::ImGuiControls::hooverTooltip("Save the current tournament to a file so you can load it later.");

    ImGui::SameLine();
    std::string loadButtonLabel = std::string("Load ") + getTournamentName();
    if (QaplaWindows::ImGuiControls::textButton(loadButtonLabel.c_str())) {
        finished_ = true;
        return "load";
    }
    QaplaWindows::ImGuiControls::hooverTooltip("Load a previously saved tournament from disk and replace the current tournament.");

    ImGui::SameLine();
    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }
    
    return "";
}

} // namespace QaplaWindows::ChatBot
