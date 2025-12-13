#include "chatbot-step-board-finish.h"

#include "../../imgui-controls.h"
#include "../../callback-manager.h"
#include "../../interactive-board-window.h"

#include <imgui.h>
#include <format>

namespace QaplaWindows::ChatBot {

ChatbotStepBoardFinish::ChatbotStepBoardFinish(BoardProvider provider)
    : provider_(std::move(provider)) {}

InteractiveBoardWindow* ChatbotStepBoardFinish::getBoard() {
    return provider_();
}

std::string ChatbotStepBoardFinish::draw() {
    if (finished_) {
        return "";
    }

    auto* board = getBoard();
    
    // Check if board still exists
    if (board == nullptr) {
        QaplaWindows::ImGuiControls::textWrapped("Error: Board no longer exists.");
        finished_ = true;
        return "stop";
    }
    
    // Cache board ID on first draw
    if (!boardId_.has_value()) {
        boardId_ = board->getId();
    }

    QaplaWindows::ImGuiControls::textWrapped("Board setup complete! You can now:");
    ImGui::Spacing();
    QaplaWindows::ImGuiControls::textWrapped(
        "• Switch to the board view to start playing or analyzing");
    QaplaWindows::ImGuiControls::textWrapped(
        "• Return to the main menu to configure something else");
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    if (QaplaWindows::ImGuiControls::textButton("Switch to Board View")) {
        // Activate engines only once when button is clicked
        if (!enginesActivated_) {
            board->setActiveEngines();
            enginesActivated_ = true;
        }
        std::string message = std::format("switch_to_board_{}", *boardId_);
        StaticCallbacks::message().invokeAll(message);
        finished_ = true;
    }
    QaplaWindows::ImGuiControls::hooverTooltip(
        "Open the board view to start playing or analyzing positions.");
    
    ImGui::SameLine();
    
    if (QaplaWindows::ImGuiControls::textButton("Finish")) {
        // Activate engines only once when button is clicked
        if (!enginesActivated_) {
            board->setActiveEngines();
            enginesActivated_ = true;
        }
        finished_ = true;
        return "stop";
    }
    QaplaWindows::ImGuiControls::hooverTooltip(
        "Return to the main menu. You can access the board later via the tabs.");

    return "";
}

} // namespace QaplaWindows::ChatBot
