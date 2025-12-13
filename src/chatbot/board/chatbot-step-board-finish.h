#pragma once

#include "../chatbot-step.h"
#include <functional>
#include <cstdint>
#include <optional>

namespace QaplaWindows {
    class InteractiveBoardWindow;
}

namespace QaplaWindows::ChatBot {

/**
 * @brief Final step for board setup - offers to switch to board view or finish.
 * 
 * This step presents the user with a choice after board configuration:
 * - Switch to the board view to start playing
 * - Return to main menu (finish)
 */
class ChatbotStepBoardFinish : public ChatbotStep {
public:
    /**
     * @brief Callback function type that returns a pointer to the board.
     * The callback may return nullptr if the board no longer exists.
     */
    using BoardProvider = std::function<InteractiveBoardWindow*()>;

    /**
     * @brief Constructs the finish step with a board provider callback.
     * @param provider Callback to get the board instance.
     */
    explicit ChatbotStepBoardFinish(BoardProvider provider);
    ~ChatbotStepBoardFinish() override = default;

    [[nodiscard]] std::string draw() override;

private:
    BoardProvider provider_;
    std::optional<uint32_t> boardId_;
    bool enginesActivated_ = false;
    
    /**
     * @brief Gets the board from the provider.
     * @return Pointer to board, or nullptr if target no longer exists.
     */
    [[nodiscard]] InteractiveBoardWindow* getBoard();
};

} // namespace QaplaWindows::ChatBot
