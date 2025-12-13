#include "chatbot-board.h"
#include "../chatbot-step.h"
#include "chatbot-step-board-select.h"
#include "../chatbot-step-global-settings.h"
#include "chatbot-step-board-time-control.h"
#include "../chatbot-step-select-engines.h"
#include "../chatbot-step-load-engine.h"
#include "chatbot-step-board-set-engines.h"

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
    
    // Add global settings step with a callback that provides the board's settings
    // The callback returns nullptr if the board no longer exists
    auto globalSettingsProvider = [this]() -> ImGuiEngineGlobalSettings* {
        if (!boardId_.has_value()) {
            return nullptr;
        }
        auto* board = InteractiveBoardWindow::getBoard(*boardId_);
        if (board == nullptr) {
            return nullptr;
        }
        return &board->getGlobalSettings();
    };
    steps_.push_back(std::make_unique<ChatbotStepGlobalSettings>(globalSettingsProvider, false));
    
    // Add time control step with a callback that provides the board's time control window
    auto timeControlProvider = [this]() -> TimeControlWindow* {
        if (!boardId_.has_value()) {
            return nullptr;
        }
        auto* board = InteractiveBoardWindow::getBoard(*boardId_);
        if (board == nullptr) {
            return nullptr;
        }
        return &board->getTimeControlWindow();
    };
    steps_.push_back(std::make_unique<ChatbotStepBoardTimeControl>(timeControlProvider));
    
    // Add engine selection step with a callback that provides the board's engine selection
    auto engineSelectProvider = [this]() -> ImGuiEngineSelect* {
        if (!boardId_.has_value()) {
            return nullptr;
        }
        auto* board = InteractiveBoardWindow::getBoard(*boardId_);
        if (board == nullptr) {
            return nullptr;
        }
        return &board->getEngineSelect();
    };
    steps_.push_back(std::make_unique<ChatbotStepSelectEngines>(engineSelectProvider, "board"));
    
    // Add load-engine step (reusing engineSelectProvider)
    steps_.push_back(std::make_unique<ChatbotStepLoadEngine>(engineSelectProvider, 1, "board"));
    
    // Add step to activate the selected engines (board-specific, no UI)
    // This must come AFTER load-engine step
    auto boardProvider = [this]() -> InteractiveBoardWindow* {
        if (!boardId_.has_value()) {
            return nullptr;
        }
        return InteractiveBoardWindow::getBoard(*boardId_);
    };
    steps_.push_back(std::make_unique<ChatbotStepBoardSetEngines>(boardProvider));
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
            
            // Parse board ID from token
            if (selectedBoardToken_ == "board:new") {
                auto newBoardId = QaplaWindows::InteractiveBoardWindow::createBoardViaMessage();
                if (newBoardId.has_value()) {
                    boardId_ = *newBoardId;
                    boardCreated_ = true;
                }
            } else {
                // Extract ID from "board:<id>" format
                try {
                    auto idStr = selectedBoardToken_.substr(6);  // Skip "board:"
                    boardId_ = static_cast<uint32_t>(std::stoul(idStr));
                } catch (...) {
                    // Invalid format, stop
                    stopped_ = true;
                    return false;
                }
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
