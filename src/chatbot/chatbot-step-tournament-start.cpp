#include "chatbot-step-tournament-start.h"

#include "tournament-data.h"
#include "sprt-tournament-data.h"
#include "imgui-controls.h"
#include "callback-manager.h"

#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentStart::ChatbotStepTournamentStart(EngineSelectContext type)
    : type_(type) {}

bool ChatbotStepTournamentStart::isStarting() const {
    if (type_ == EngineSelectContext::SPRT) {
        return SprtTournamentData::instance().isStarting();
    }
    return TournamentData::instance().isStarting();
}

bool ChatbotStepTournamentStart::isRunning() const {
    if (type_ == EngineSelectContext::SPRT) {
        return SprtTournamentData::instance().isRunning();
    }
    return TournamentData::instance().isRunning();
}

uint32_t ChatbotStepTournamentStart::getExternalConcurrency() const {
    if (type_ == EngineSelectContext::SPRT) {
        return SprtTournamentData::instance().getExternalConcurrency();
    }
    return TournamentData::instance().getExternalConcurrency();
}

void ChatbotStepTournamentStart::setExternalConcurrency(uint32_t count) {
    if (type_ == EngineSelectContext::SPRT) {
        SprtTournamentData::instance().setExternalConcurrency(count);
    } else {
        TournamentData::instance().setExternalConcurrency(count);
    }
}

void ChatbotStepTournamentStart::startTournament() {
    if (type_ == EngineSelectContext::SPRT) {
        SprtTournamentData::instance().startTournament();
    } else {
        TournamentData::instance().startTournament();
    }
}

void ChatbotStepTournamentStart::setPoolConcurrency(uint32_t count, bool nice, bool direct) {
    if (type_ == EngineSelectContext::SPRT) {
        SprtTournamentData::instance().setPoolConcurrency(count, nice, direct);
    } else {
        TournamentData::instance().setPoolConcurrency(count, nice, direct);
    }
}

const char* ChatbotStepTournamentStart::getTournamentName() const {
    return (type_ == EngineSelectContext::SPRT) ? "SPRT tournament" : "tournament";
}

const char* ChatbotStepTournamentStart::getSwitchViewMessage() const {
    return (type_ == EngineSelectContext::SPRT) ? "switch_to_sprt_view" : "switch_to_tournament_view";
}

std::string ChatbotStepTournamentStart::draw() {
    if (finished_) {
        return "";
    }

    if (isStarting()) {
        std::string message = std::string("The ") + getTournamentName() + " is starting up, please wait...";
        QaplaWindows::ImGuiControls::textWrapped(message.c_str());
        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
        }
        return "";
    }

    if (!isRunning()) {
        std::string configMessage = std::string("Configure ") + getTournamentName() + " concurrency and start:";
        QaplaWindows::ImGuiControls::textWrapped(configMessage.c_str());
        ImGui::Spacing();
        auto concurrency = getExternalConcurrency();
        ImGuiControls::sliderInt<uint32_t>("Concurrency", concurrency, 1, 16);

        setExternalConcurrency(concurrency);

        QaplaWindows::ImGuiControls::hooverTooltip("Number of games to run in parallel");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        std::string startLabel = std::string("Start ") + getTournamentName();
        startLabel[6] = static_cast<char>(std::toupper(startLabel[6])); // Capitalize first letter after "Start "
        if (QaplaWindows::ImGuiControls::textButton(startLabel.c_str())) {
            startTournament();
            setPoolConcurrency(static_cast<uint32_t>(concurrency), true, true);
        }
        QaplaWindows::ImGuiControls::hooverTooltip("Start the tournament now with the specified concurrency and engine settings.");

        ImGui::SameLine();

        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
            return "stop";
        }
    } else {
        std::string successMessage = std::string("The ") + getTournamentName() + " started successfully!";
        successMessage[4] = static_cast<char>(std::toupper(successMessage[4])); // Capitalize tournament name
        QaplaWindows::ImGuiControls::textWrapped(successMessage.c_str());
        ImGui::Spacing();
        QaplaWindows::ImGuiControls::textWrapped("You can switch between all running games and the chatbot using the tabs at the top of the window. "
            "Each game has its own tab, so you can easily navigate between them.");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        std::string switchLabel = std::string("Switch to ") + getTournamentName() + " View";
        switchLabel[10] = static_cast<char>(std::toupper(switchLabel[10])); // Capitalize first letter
        if (QaplaWindows::ImGuiControls::textButton(switchLabel.c_str())) {
            StaticCallbacks::message().invokeAll(getSwitchViewMessage());
            finished_ = true;
        }
        QaplaWindows::ImGuiControls::hooverTooltip("Switch to the tournament view to inspect running games and progress.");
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton("Stay in Chatbot")) {
            finished_ = true;
        }
        QaplaWindows::ImGuiControls::hooverTooltip("Remain in the chatbot interface. You can switch to tournament view later via the tabs.");
    }
    return "";
}

} // namespace QaplaWindows::ChatBot
