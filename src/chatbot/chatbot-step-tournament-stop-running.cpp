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
#include "sprt-tournament-data.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentStopRunning::ChatbotStepTournamentStopRunning(EngineSelectContext context)
    : context_(context) {}

bool ChatbotStepTournamentStopRunning::isRunning() const {
    if (context_ == EngineSelectContext::SPRT) {
        return SprtTournamentData::instance().isRunning();
    }
    return TournamentData::instance().isRunning();
}

void ChatbotStepTournamentStopRunning::stopPool(bool graceful) const {
    if (context_ == EngineSelectContext::SPRT) {
        SprtTournamentData::instance().stopPool(graceful);
    } else {
        TournamentData::instance().stopPool(graceful);
    }
}

std::string ChatbotStepTournamentStopRunning::draw() {
    if (finished_) {
        QaplaWindows::ImGuiControls::textDisabled(finishedMessage_);
        return "";
    }
    
    if (!isRunning()) {
        finished_ = true;
        return "existing";
    }

    const char* tournamentName = (context_ == EngineSelectContext::SPRT) ? "SPRT tournament" : "tournament";
    std::string message = std::string("A ") + tournamentName + " is currently running. Would you like to end it?";
    QaplaWindows::ImGuiControls::textWrapped(message.c_str());
    
    ImGui::Spacing();
    ImGui::Spacing();

    if (QaplaWindows::ImGuiControls::textButton("Yes, end tournament")) {
        stopPool(false);
        finishedMessage_ = std::string(tournamentName) + " ended.";
        finishedMessage_[0] = static_cast<char>(std::toupper(finishedMessage_[0]));
        finished_ = true;
        return "menu";
    }
    QaplaWindows::ImGuiControls::hooverTooltip("End the currently running tournament and stop all games. Progress will be saved to the PGN if configured.");

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Show Results")) {
        finished_ = true;
        return "show-result";
    }
    QaplaWindows::ImGuiControls::hooverTooltip("View detailed tournament results.");

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finishedMessage_ = std::string(tournamentName) + " continues.";
        finishedMessage_[0] = static_cast<char>(std::toupper(finishedMessage_[0]));
        finished_ = true;
        return "stop";
    }
    QaplaWindows::ImGuiControls::hooverTooltip("Keep the tournament running and close the chatbot.");
    
    return "";
}

} // namespace QaplaWindows::ChatBot
