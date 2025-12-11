#include "chatbot-board.h"
#include "../chatbot-step.h"
#include "chatbot-step-board-select.h"

#include "../../interactive-board-window.h"

namespace QaplaWindows::ChatBot {

void ChatbotBoard::start() {
    steps_.clear();
    currentStepIndex_ = 0;
    stopped_ = false;
    selectedBoardToken_.clear();
    boardId_.reset();
    boardCreated_ = false;

    steps_.push_back(std::make_unique<ChatbotStepBoardSelect>());
}

bool ChatbotBoard::draw() {
    bool contentChanged = false;

    if (stopped_ || steps_.empty()) {
        return false;
    }

    // Draw all completed steps
    for (size_t index = 0; index < currentStepIndex_ && index < steps_.size(); ++index) {
        static_cast<void>(steps_[index]->draw());
    }

    // Draw and handle current step
    if (currentStepIndex_ < steps_.size()) {
        std::string result = steps_[currentStepIndex_]->draw();

        if (result == "stop") {
            stopped_ = true;
            return false;
        }

        // Store board selection token from first step, e.g. "board:1" or "board:new"
        if (selectedBoardToken_.empty() && result.rfind("board:", 0) == 0) {
            selectedBoardToken_ = result;
        }

        if (!boardCreated_ && selectedBoardToken_ == "board:new") {
            auto newBoardId = QaplaWindows::InteractiveBoardWindow::createBoardViaMessage();
            if (newBoardId.has_value()) {
                boardId_ = *newBoardId;
                boardCreated_ = true;
            }
        }

        if (steps_[currentStepIndex_]->isFinished()) {
            ++currentStepIndex_;
            contentChanged = true;
        }
    }

    return contentChanged;
}

bool ChatbotBoard::isFinished() const {
    if (stopped_) {
        return true;
    }
    if (steps_.empty()) {
        return false;
    }
    return currentStepIndex_ >= steps_.size();
}

std::unique_ptr<ChatbotThread> ChatbotBoard::clone() const {
    return std::make_unique<ChatbotBoard>();
}

} // namespace QaplaWindows::ChatBot
