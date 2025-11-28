#pragma once

#include "chatbot-step.h"
#include "chatbot-tournament.h"

namespace QaplaWindows::ChatBot {

class ChatbotStepTournamentStart : public ChatbotStep {
public:
    ChatbotStepTournamentStart();
    ~ChatbotStepTournamentStart() override = default;

    [[nodiscard]] std::string draw() override;

};

} // namespace QaplaWindows::ChatBot
