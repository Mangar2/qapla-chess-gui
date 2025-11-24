#pragma once

#include "chatbot-step.h"
#include "chatbot-tournament.h"

namespace QaplaWindows::ChatBot {

class ChatbotStepTournamentStart : public ChatbotStep {
public:
    ChatbotStepTournamentStart();
    ~ChatbotStepTournamentStart() override = default;

    void draw() override;
    [[nodiscard]] bool isFinished() const override;

private:
    bool finished_ = false;
    bool tournamentStarted_ = false;
    int concurrency_ = 1;
};

} // namespace QaplaWindows::ChatBot
