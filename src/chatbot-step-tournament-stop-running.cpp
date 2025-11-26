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

ChatbotStepTournamentStopRunning::ChatbotStepTournamentStopRunning() {
    // If no tournament is running, finish immediately
    if (!TournamentData::instance().isRunning()) {
        finished_ = true;
    }
}

std::string ChatbotStepTournamentStopRunning::draw() {
    if (finished_) {
        if (!finishedMessage_.empty()) {
            QaplaWindows::ImGuiControls::textDisabled(finishedMessage_);
        }
        return "";
    }

    // Tournament is currently running - use error/red color
    ImGui::PushStyleColor(ImGuiCol_Text, StepColors::ERROR_COLOR);
    QaplaWindows::ImGuiControls::textWrapped(
        "A tournament is currently running!");
    ImGui::PopStyleColor();
    QaplaWindows::ImGuiControls::textWrapped(
        "\nTo configure a new tournament, the current one must be stopped first.\n\n"
        "If you stop the tournament:\n"
        "- All running games will be terminated\n"
        "- You can save the results in the next step\n\n"
        "If you cancel:\n"
        "- The tournament continues running\n"
        "- You can return to the tournament view");
    
    ImGui::Spacing();
    ImGui::Spacing();

    if (QaplaWindows::ImGuiControls::textButton("Stop Tournament")) {
        TournamentData::instance().stopPool(false);
        finishedMessage_ = "Tournament stopped.";
        finished_ = true;
    }

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finishedMessage_ = "Cancelled - tournament continues running.";
        finished_ = true;
        return "stop";
    }
    
    return "";
}

bool ChatbotStepTournamentStopRunning::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
