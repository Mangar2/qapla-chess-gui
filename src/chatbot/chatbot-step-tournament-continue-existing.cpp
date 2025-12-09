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

#include "chatbot-step-tournament-continue-existing.h"
#include "tournament-data.h"
#include "sprt-tournament-data.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentContinueExisting::ChatbotStepTournamentContinueExisting(EngineSelectContext type)
    : type_(type) {}

bool ChatbotStepTournamentContinueExisting::hasTasksScheduled() const {
    if (type_ == EngineSelectContext::SPRT) {
        return SprtTournamentData::instance().hasResults();
    }
    return TournamentData::instance().hasTasksScheduled();
}

bool ChatbotStepTournamentContinueExisting::isFinished() const {
    if (type_ == EngineSelectContext::SPRT) {
        return SprtTournamentData::instance().isFinished();
    }
    return TournamentData::instance().isFinished();
}

const char* ChatbotStepTournamentContinueExisting::getTournamentName() const {
    return (type_ == EngineSelectContext::SPRT) ? "SPRT tournament" : "tournament";
}

std::string ChatbotStepTournamentContinueExisting::draw() {
    if (!hasTasksScheduled() || isFinished()) {
        finished_ = true;
        return "menu";
    }
    if (finished_) {
        QaplaWindows::ImGuiControls::textDisabled(finishedMessage_);
        return "";
    }

    std::string message = std::string("There is an existing ") + getTournamentName() + 
        " that can be continued. Would you like to continue it?";
    QaplaWindows::ImGuiControls::textWrapped(message.c_str());
    
    ImGui::Spacing();
    ImGui::Spacing();

    std::string yesLabel = std::string("Yes, continue ") + getTournamentName();
    if (QaplaWindows::ImGuiControls::textButton(yesLabel.c_str())) {
        std::string msg = std::string("Continuing existing ") + getTournamentName() + ".";
        msg[0] = static_cast<char>(std::toupper(msg[0]));
        finishedMessage_ = msg;
        finished_ = true;
        return "start";
    }
    QaplaWindows::ImGuiControls::hooverTooltip("Resume the existing tournament and continue scheduled tasks.");
    
    ImGui::SameLine();
    if (QaplaWindows::ImGuiControls::textButton("Show Result")) {
        finishedMessage_ = "";
        finished_ = true;
        return "show-result";
    }
    QaplaWindows::ImGuiControls::hooverTooltip("View the current tournament results without continuing.");
    
    ImGui::SameLine();
    if (QaplaWindows::ImGuiControls::textButton("No")) {
        finishedMessage_ = "";
        finished_ = true;
        return "menu";
    }
    QaplaWindows::ImGuiControls::hooverTooltip("Do not continue the existing tournament; return to the menu.");

    ImGui::SameLine();
    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finishedMessage_ = "";
        finished_ = true;
        return "stop";
    }
    
    return "";
}

} // namespace QaplaWindows::ChatBot
