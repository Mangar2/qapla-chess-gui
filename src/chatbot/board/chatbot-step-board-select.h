#pragma once

#include "../chatbot-step.h"
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to select which interactive board to use.
 *
 * Returns a token string via draw():
 *  - "board:1" to use the default board
 *  - "board:new" to create a new board
 */
class ChatbotStepBoardSelect : public ChatbotStep {
public:
    ChatbotStepBoardSelect() = default;
    ~ChatbotStepBoardSelect() override = default;

    [[nodiscard]] std::string draw() override;
};

} // namespace QaplaWindows::ChatBot
