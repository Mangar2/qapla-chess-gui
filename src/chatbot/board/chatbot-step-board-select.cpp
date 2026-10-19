#include "chatbot-step-board-select.h"
#include "interactive-board-window.h"
#include "imgui-controls.h"
#include "i18n.h"

#include "ui/qapla-ui.h"

namespace QaplaWindows::ChatBot {

std::string ChatbotStepBoardSelect::draw() {
    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped("Select the board you want to use.");
        QaplaUi::spacing();
        
        if (QaplaWindows::ImGuiControls::textButton("Use Board 1")) {
            finished_ = true;
            return "board:1";
        }

        QaplaUi::sameLine();

        if (QaplaWindows::ImGuiControls::textButton("Create new board")) {
            finished_ = true;
            return "board:new";
        }

        QaplaUi::spacing();
    } else {
        QaplaWindows::ImGuiControls::textWrapped("Board selection completed.");
    }

    return "";
}

} // namespace QaplaWindows::ChatBot
