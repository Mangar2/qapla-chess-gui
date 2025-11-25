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
#include "os-dialogs.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentSaveExisting::ChatbotStepTournamentSaveExisting() = default;

std::string ChatbotStepTournamentSaveExisting::draw() {
    if (finished_) {
        if (!actionMessage_.empty()) {
            QaplaWindows::ImGuiControls::textDisabled(actionMessage_);
        } 
        return result_;
    }

    auto& tournament = TournamentData::instance();
    bool isRunning = tournament.getState() == TournamentData::State::Running;
    bool hasTasksScheduled = tournament.hasTasksScheduled();

    // Determine what state we're in and explain to the user
    if (isRunning) {
        // Tournament is currently running
        QaplaWindows::ImGuiControls::textWrapped(
            "A tournament is currently running!\n\n"
            "Please press cancel, if you want to continue the current tournament.\n\n"
            "If you continue without saving:\n"
            "- The running tournament will be stopped\n"
            "- All game results will be lost\n"
            "- All tournament settings will be reset\n\n"
            "If you save first:\n"
            "- The running tournament will be stopped\n"
            "- Tournament configuration and results are preserved\n"
            "- You can load and continue the tournament later");
    } else if (hasTasksScheduled) {
        // Tournament was started but is now stopped (has results)
        QaplaWindows::ImGuiControls::textWrapped(
            "A previous tournament has results that haven't been saved.\n\n"
            "If you continue without saving:\n"
            "- All game results will be lost\n"
            "- All tournament settings will be reset\n\n"
            "If you save first:\n"
            "- Tournament configuration and results are preserved\n"
            "- You can load and review the results later");
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

    if (QaplaWindows::ImGuiControls::textButton("Save Tournament")) {
        auto path = OsDialogs::saveFileDialog({{"Qapla Tournament Files", "qtour"}});
        if (!path.empty()) {
            TournamentData::saveTournament(path);
            if (isRunning) {
                actionMessage_ = "Tournament saved. Running tournament stopped and results cleared.";
            } else if (hasTasksScheduled) {
                actionMessage_ = "Tournament saved. Previous results cleared.";
            } else {
                actionMessage_ = "Tournament settings saved.";
            }
            TournamentData::instance().clear(false);
        }
        finished_ = true;
        result_ = "";
    }
    
    ImGui::SameLine();

    // Adjust button text based on state
    const char* skipButtonText = hasTasksScheduled ? "Discard & Continue" : "Skip";
    
    if (QaplaWindows::ImGuiControls::textButton(skipButtonText)) {
        if (isRunning) {
            actionMessage_ = "Running tournament stopped and results discarded.";
        } else if (hasTasksScheduled) {
            actionMessage_ = "Previous tournament results discarded.";
        } else {
            actionMessage_ = "";
        }
        TournamentData::instance().clear(false);
        finished_ = true;
        result_ = "";
    }

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        actionMessage_ = "Tournament setup cancelled.";
        finished_ = true;
        result_ = "stop";
    }

    return result_;
}

bool ChatbotStepTournamentSaveExisting::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
