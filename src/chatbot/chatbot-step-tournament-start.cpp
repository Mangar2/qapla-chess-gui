#include "chatbot-step-tournament-start.h"

#include "tournament-data.h"
#include "imgui-controls.h"
#include "i18n.h"
#include "callback-manager.h"
#include "imgui-controls.h"

#include <imgui.h>
#include <algorithm>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentStart::ChatbotStepTournamentStart() {
}

std::string ChatbotStepTournamentStart::draw() {
    if (finished_) {
        return "";
    }

    bool isFinished = TournamentData::instance().isFinished();

    if (isFinished) {
        QaplaWindows::ImGuiControls::textWrapped("The tournament is already finished.");
        ImGui::Spacing();
        if (QaplaWindows::ImGuiControls::textButton("Return")) {
            finished_ = true;
        }
        return "";
    }

    if (TournamentData::instance().isStarting()) {
        QaplaWindows::ImGuiControls::textWrapped("Tournament is starting up, please wait...");
        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
        }
        return "";
    }
    if (!TournamentData::instance().isRunning()) {
        QaplaWindows::ImGuiControls::textWrapped("Configure tournament concurrency and start:");
        ImGui::Spacing();
        auto concurrency = TournamentData::instance().getExternalConcurrency();
        ImGuiControls::sliderInt<uint32_t>("Concurrency", concurrency, 1, 16);

        TournamentData::instance().setExternalConcurrency(concurrency);

        QaplaWindows::ImGuiControls::hooverTooltip("Number of games to run in parallel");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (QaplaWindows::ImGuiControls::textButton("Start Tournament")) {
            TournamentData::instance().startTournament();
            TournamentData::instance().setPoolConcurrency(static_cast<uint32_t>(concurrency), true, true);
        }

        ImGui::SameLine();

        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
            return "stop";
        }
    } else {
        QaplaWindows::ImGuiControls::textWrapped("Tournament started successfully!");
        ImGui::Spacing();
        QaplaWindows::ImGuiControls::textWrapped("You can switch between all running games and the chatbot using the tabs at the top of the window. "
            "Each game has its own tab, so you can easily navigate between them.");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (QaplaWindows::ImGuiControls::textButton("Switch to Tournament View")) {
            StaticCallbacks::message().invokeAll("switch_to_tournament_view");
            finished_ = true;
        }
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton("Stay in Chatbot")) {
            finished_ = true;
        }
    }
    return "";
}

bool ChatbotStepTournamentStart::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
