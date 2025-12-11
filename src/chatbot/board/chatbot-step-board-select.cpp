#include "chatbot-step-board-select.h"
#include "interactive-board-window.h"
#include "imgui-controls.h"
#include "i18n.h"

#include <imgui.h>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepBoardSelect::draw() {
    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            Translator::instance().translate("Chatbot", "Select the board you want to use.").c_str());
        ImGui::Spacing();

        if (QaplaWindows::ImGuiControls::textButton(
                Translator::instance().translate("Chatbot", "Use Board 1").c_str())) {
            finished_ = true;
            return "board:1";
        }

        ImGui::SameLine();

        if (QaplaWindows::ImGuiControls::textButton(
                Translator::instance().translate("Chatbot", "Create new board").c_str())) {
            finished_ = true;
            return "board:new";
        }

        ImGui::Spacing();
    } else {
        QaplaWindows::ImGuiControls::textWrapped(
            Translator::instance().translate("Chatbot", "Board selection completed.").c_str());
    }

    return "";
}

} // namespace QaplaWindows::ChatBot
