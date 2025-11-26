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
#include "chatbot-step.h"
#include "tournament-data.h"
#include "imgui-controls.h"
#include "os-dialogs.h"
#include "callback-manager.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentSaveExisting::ChatbotStepTournamentSaveExisting() = default;

std::string ChatbotStepTournamentSaveExisting::draw() {
    if (finished_) {
        QaplaWindows::ImGuiControls::textDisabled(finishedMessage_);
        return "";
    }

    auto& tournament = TournamentData::instance();
    bool hasTasksScheduled = tournament.hasTasksScheduled();

    // Determine what state we're in and explain to the user
    if (hasTasksScheduled) {
        if (saved_) {
            QaplaWindows::ImGuiControls::textWrapped(
                "Tournament has been saved. You can now continue the existing tournament or start a new one.");
        } else {
            // Tournament was started but is now stopped (has results) - use warning/orange color
            ImGui::PushStyleColor(ImGuiCol_Text, StepColors::WARNING_COLOR);
            QaplaWindows::ImGuiControls::textWrapped(
                "A previous tournament has results that haven't been saved.");
            ImGui::PopStyleColor();
            QaplaWindows::ImGuiControls::textWrapped(
                "\nYou can continue the existing tournament, or start a new one.\n\n"
                "If you continue without saving:\n"
                "- All game results will be lost\n"
                "- All tournament settings will be reset\n\n"
                "If you save first:\n"
                "- Tournament configuration and results are preserved\n"
                "- You can load and review the results later");
        }
    } else {
        // No tournament results, but we might have configuration
        QaplaWindows::ImGuiControls::textWrapped(
            "We will now configure a new tournament.\n\n"
            "Saving is recommended because:\n"
            "- Current tournament settings will be replaced\n"
            "- During this chat, we will modify tournament settings\n"
            "- Saving preserves your current configuration\n\n"
            "If you have no important settings, you can skip this step.");
    }
    
    ImGui::Spacing();
    ImGui::Spacing();

    bool stop = drawContinueButton(hasTasksScheduled);
    bool start = false;

    ImGui::SameLine();

    // Show "Continue Tournament" button if there are tasks scheduled
    if (hasTasksScheduled) {
        start |= drawContinueTournamentButton();
        ImGui::SameLine();
    }

    if (!saved_) {
        stop |= drawSaveTournamentButton();
        ImGui::SameLine();
    }

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finishedMessage_ = "Tournament setup cancelled.";
        finished_ = true;
        stop = true;
    }
    
    return start ? "start" : (stop ? "stop" : "");
}

bool ChatbotStepTournamentSaveExisting::drawContinueTournamentButton() {
    if (QaplaWindows::ImGuiControls::textButton("Continue Existing")) {
        finished_ = true;
        return true;  
    }
    return false;
}

bool ChatbotStepTournamentSaveExisting::drawSaveTournamentButton() {
    if (QaplaWindows::ImGuiControls::textButton("Save Tournament")) {
        auto path = OsDialogs::saveFileDialog({{"Qapla Tournament Files", "qtour"}});
        if (!path.empty()) {
            TournamentData::saveTournament(path);
            saved_ = true;
        }
    }
    return false;
}

bool ChatbotStepTournamentSaveExisting::drawContinueButton(bool hasTasksScheduled) {
    // Adjust button text based on state
    const char* continueButtonText = hasTasksScheduled ? "New Tournament" : "Continue";
    
    if (QaplaWindows::ImGuiControls::textButton(continueButtonText)) {
        if (hasTasksScheduled) {
            finishedMessage_ = "Previous tournament results discarded.";
        } else {
            finishedMessage_ = "";
        }
        TournamentData::instance().clear(false);
        finished_ = true;
    }
    return false;
}

bool ChatbotStepTournamentSaveExisting::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
