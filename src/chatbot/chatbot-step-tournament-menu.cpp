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
#include "imgui-controls.h"
#include "os-dialogs.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepTournamentMenu::draw() {
    if (finished_) {
        return "";
    }

    QaplaWindows::ImGuiControls::textWrapped("What would you like to do?\n");
    if (!saved_) {
        ImGui::Spacing();
        QaplaWindows::ImGuiControls::textWrapped(
            "Starting a new tournament will delete the existing one.\n"
            "Save the current tournament to a file if you want to keep it.");
    }
    
    ImGui::Spacing();
    ImGui::Spacing();

    if (QaplaWindows::ImGuiControls::textButton("New tournament")) {
        TournamentData::instance().clear(false);
        finished_ = true;
        return "new";
    }

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Save tournament")) {
        auto path = OsDialogs::saveFileDialog({{"Qapla Tournament Files", "qtour"}});
        if (!path.empty()) {
            TournamentData::saveTournament(path);
            saved_ = true;
        }
    }

    ImGui::SameLine();
    if (QaplaWindows::ImGuiControls::textButton("Load tournament")) {
        finished_ = true;
        return "load";
    }

    ImGui::SameLine();
    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }
    
    return "";
}

bool ChatbotStepTournamentMenu::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
