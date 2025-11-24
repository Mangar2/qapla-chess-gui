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

#include "chatbot-step-tournament-save-existing.h"
#include "tournament-data.h"
#include "imgui-controls.h"
#include "i18n.h"
#include "os-dialogs.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentSaveExisting::ChatbotStepTournamentSaveExisting() = default;

void ChatbotStepTournamentSaveExisting::draw() {
    if (!askUser_) {
        finished_ = true;
        return;
    }

    QaplaWindows::ImGuiControls::textWrapped("Do you want to save the current tournament before creating a new one?");
    ImGui::Spacing();

    if (finished_) {
        QaplaWindows::ImGuiControls::textDisabled("Step completed.");
        return;
    }

    if (QaplaWindows::ImGuiControls::textButton("Yes, Save")) {
        auto path = OsDialogs::saveFileDialog({{"Qapla Tournament Files", "qtour"}});
        if (!path.empty()) {
            TournamentData::saveTournament(path);
        }
        finished_ = true;
    }
    
    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("No, Discard")) {
        TournamentData::instance().clear();
        finished_ = true;
    }
}

bool ChatbotStepTournamentSaveExisting::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
