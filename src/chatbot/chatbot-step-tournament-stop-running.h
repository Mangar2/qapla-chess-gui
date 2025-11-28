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
 * @brief Step to check if a tournament is running and offer to stop it.
 * 
 * If no tournament is running, this step finishes automatically.
 */
class ChatbotStepTournamentStopRunning : public ChatbotStep {
public:
    ChatbotStepTournamentStopRunning();

    [[nodiscard]] std::string draw() override;

private:
    std::string finishedMessage_;
};

} // namespace QaplaWindows::ChatBot
