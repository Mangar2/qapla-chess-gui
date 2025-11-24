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
#include <vector>
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to load additional engines from disk.
 */
class ChatbotStepTournamentLoadEngine : public ChatbotStep {
public:
    ChatbotStepTournamentLoadEngine();
    ~ChatbotStepTournamentLoadEngine() override = default;

    void draw() override;
    [[nodiscard]] bool isFinished() const override;

private:
    bool finished_ = false;
    
    enum class State {
        Input,
        Detecting,
        Summary
    };
    
    State state_ = State::Input;
    std::vector<std::string> addedEnginePaths_;
    bool detectionStarted_ = false;

    void drawInput();
    void drawDetecting();
    void drawSummary();
    
    void addEngines();
    void startDetection();
    void selectAddedEngines();
};

} // namespace QaplaWindows::ChatBot
