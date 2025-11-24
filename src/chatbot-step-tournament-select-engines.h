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
 * @author GitHub Copilot
 * @copyright Copyright (c) 2025 GitHub Copilot
 */

#pragma once

#include "chatbot-step.h"
#include "chatbot-tournament.h"
#include "engine-capabilities.h"
#include <vector>
#include <string>
#include <map>

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to select engines from the list of available engines.
 */
class ChatbotStepTournamentSelectEngines : public ChatbotStep {
public:
    explicit ChatbotStepTournamentSelectEngines(ChatbotTournament* thread);

    void draw() override;
    [[nodiscard]] bool isFinished() const override;

private:
    ChatbotTournament* thread_;
    bool finished_ = false;
    
    struct EngineEntry {
        std::string name;
        std::string path;
        QaplaTester::EngineProtocol protocol;
        bool selected = false;
    };
    
    std::vector<EngineEntry> availableEngines_;
};

} // namespace QaplaWindows::ChatBot
