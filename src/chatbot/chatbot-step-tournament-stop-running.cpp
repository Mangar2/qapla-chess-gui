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

#include "chatbot-step-tournament-stop-running.h"
#include "chatbot-step.h"
#include "tournament-data.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentStopRunning::ChatbotStepTournamentStopRunning() = default;


std::string ChatbotStepTournamentStopRunning::draw() {
    if (finished_) {
        QaplaWindows::ImGuiControls::textDisabled(finishedMessage_);
        return "";
    }
    
    if (!TournamentData::instance().isRunning()) {
        finished_ = true;
        return "existing";
    }

    QaplaWindows::ImGuiControls::textWrapped(
        "A tournament is currently running. Would you like to end it?");
    
    ImGui::Spacing();
    ImGui::Spacing();

    if (QaplaWindows::ImGuiControls::textButton("Yes, end tournament")) {
        TournamentData::instance().stopPool(false);
        finishedMessage_ = "Tournament ended.";
        finished_ = true;
        return "menu";
    }

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finishedMessage_ = "Tournament continues.";
        finished_ = true;
        return "stop";
    }
    
    return "";
}

bool ChatbotStepTournamentStopRunning::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
