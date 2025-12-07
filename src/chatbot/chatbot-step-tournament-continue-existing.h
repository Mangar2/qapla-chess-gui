/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#pragma once

#include "chatbot-step.h"
#include "chatbot-step-tournament-stop-running.h"
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to ask the user if they want to continue an existing tournament.
 * 
 * This step is only active if there are scheduled tasks and the tournament is not finished.
 * If no tournament is in progress, this step finishes automatically.
 * Supports both standard tournaments and SPRT tournaments.
 */
class ChatbotStepTournamentContinueExisting : public ChatbotStep {
public:
    explicit ChatbotStepTournamentContinueExisting(EngineSelectContext type = EngineSelectContext::Standard);

    [[nodiscard]] std::string draw() override;

private:
    EngineSelectContext type_;
    std::string finishedMessage_;

    /**
     * @brief Checks if the tournament has tasks scheduled.
     * @return true if tasks are scheduled, false otherwise.
     */
    [[nodiscard]] bool hasTasksScheduled() const;

    /**
     * @brief Checks if the tournament is finished.
     * @return true if finished, false otherwise.
     */
    [[nodiscard]] bool isFinished() const;

    /**
     * @brief Gets the tournament name for display.
     * @return The tournament name string.
     */
    [[nodiscard]] const char* getTournamentName() const;
};

} // namespace QaplaWindows::ChatBot
