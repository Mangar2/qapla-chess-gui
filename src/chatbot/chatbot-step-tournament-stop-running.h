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
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief Enum to distinguish between standard tournament and SPRT tournament.
 */
enum class TournamentType {
    Standard,
    SPRT
};

/**
 * @brief Step to check if a tournament is running and offer to stop it.
 * 
 * If no tournament is running, this step finishes automatically.
 * Supports both standard tournaments and SPRT tournaments.
 */
class ChatbotStepTournamentStopRunning : public ChatbotStep {
public:
    explicit ChatbotStepTournamentStopRunning(TournamentType type = TournamentType::Standard);

    [[nodiscard]] std::string draw() override;

private:
    TournamentType type_;
    std::string finishedMessage_;

    /**
     * @brief Checks if the tournament is currently running.
     * @return true if running, false otherwise.
     */
    [[nodiscard]] bool isRunning() const;

    /**
     * @brief Stops the tournament pool.
     * @param graceful If true, stops gracefully.
     */
    void stopPool(bool graceful) const;
};

} // namespace QaplaWindows::ChatBot
