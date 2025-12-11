#pragma once

#include "../chatbot-thread.h"
#include "../chatbot-step.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace QaplaWindows::ChatBot {

/**
 * @brief Chatbot thread to prepare playing on an interactive board.
 */
class ChatbotBoard : public ChatbotThread {
public:
    [[nodiscard]] std::string getTitle() const override {
        return "Board";
    }

    void start() override;
    bool draw() override;
    [[nodiscard]] bool isFinished() const override;
    [[nodiscard]] std::unique_ptr<ChatbotThread> clone() const override;

private:
    std::vector<std::unique_ptr<ChatbotStep>> steps_;
    size_t currentStepIndex_ = 0;
    bool stopped_ = false;

    // Encoded board selection from first step (e.g. "board:1", "board:new").
    std::string selectedBoardToken_{};
    std::optional<uint32_t> boardId_{};
    bool boardCreated_ = false;
};

} // namespace QaplaWindows::ChatBot
