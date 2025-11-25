#include "chatbot-step-tournament-start.h"
#include "tournament-data.h"
#include "imgui-controls.h"
#include "i18n.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentStart::ChatbotStepTournamentStart() {
    concurrency_ = TournamentData::instance().concurrency();
    if (concurrency_ < 1) concurrency_ = 1;
    if (concurrency_ > 32) concurrency_ = 32;
}

std::string ChatbotStepTournamentStart::draw() {
    if (finished_) {
        return "";
    }

    if (!tournamentStarted_) {
        QaplaWindows::ImGuiControls::textWrapped("Configure tournament concurrency and start:");
        ImGui::Spacing();

        ImGui::SliderInt("Concurrency", &concurrency_, 1, 32);
        QaplaWindows::ImGuiControls::hooverTooltip("Number of games to run in parallel");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (QaplaWindows::ImGuiControls::textButton("Start Tournament")) {
            TournamentData::instance().startTournament();
            TournamentData::instance().setPoolConcurrency(concurrency_, true, true);
            
            if (TournamentData::instance().isRunning()) {
                tournamentStarted_ = true;
            } else {
                // Failed to start
                finished_ = true; 
            }
        }

        ImGui::SameLine();

        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
        }
    } else {
        QaplaWindows::ImGuiControls::textWrapped("Tournament started successfully.");
        QaplaWindows::ImGuiControls::textWrapped("We will now switch to the tournament view.");
        
        ImGui::Spacing();
        
        if (QaplaWindows::ImGuiControls::textButton("Finish")) {
            finished_ = true;
        }
    }
    return "";
}

bool ChatbotStepTournamentStart::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
