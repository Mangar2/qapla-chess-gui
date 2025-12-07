#pragma once

#include "chatbot-step.h"
#include "chatbot-step-tournament-stop-running.h"

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to configure concurrency and start the tournament.
 * 
 * This step allows the user to set the concurrency level and start the tournament.
 * Supports both standard tournaments and SPRT tournaments.
 */
class ChatbotStepTournamentStart : public ChatbotStep {
public:
    explicit ChatbotStepTournamentStart(EngineSelectContext type = EngineSelectContext::Standard);
    ~ChatbotStepTournamentStart() override = default;

    [[nodiscard]] std::string draw() override;

private:
    EngineSelectContext type_;

    /**
     * @brief Checks if the tournament is finished.
     * @return true if finished, false otherwise.
     */
    [[nodiscard]] bool isFinished() const;

    /**
     * @brief Checks if the tournament is starting.
     * @return true if starting, false otherwise.
     */
    [[nodiscard]] bool isStarting() const;

    /**
     * @brief Checks if the tournament is running.
     * @return true if running, false otherwise.
     */
    [[nodiscard]] bool isRunning() const;

    /**
     * @brief Gets the external concurrency value.
     * @return The external concurrency value.
     */
    [[nodiscard]] uint32_t getExternalConcurrency() const;

    /**
     * @brief Sets the external concurrency value.
     * @param count The new external concurrency value.
     */
    void setExternalConcurrency(uint32_t count);

    /**
     * @brief Starts the tournament.
     */
    void startTournament();

    /**
     * @brief Sets the pool concurrency.
     * @param count The concurrency count.
     * @param nice Whether to stop nicely.
     * @param direct Whether to apply directly.
     */
    void setPoolConcurrency(uint32_t count, bool nice, bool direct);

    /**
     * @brief Gets the tournament name for display.
     * @return The tournament name string.
     */
    [[nodiscard]] const char* getTournamentName() const;

    /**
     * @brief Gets the switch view message callback name.
     * @return The callback message name.
     */
    [[nodiscard]] const char* getSwitchViewMessage() const;
};

} // namespace QaplaWindows::ChatBot
