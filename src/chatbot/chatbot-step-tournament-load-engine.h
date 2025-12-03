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
#include "chatbot-step-tournament-stop-running.h"
#include <vector>
#include <string>

namespace QaplaWindows {
    class ImGuiEngineSelect;
}

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to load additional engines from disk.
 * Supports both standard tournaments and SPRT tournaments.
 */
class ChatbotStepTournamentLoadEngine : public ChatbotStep {
public:
    explicit ChatbotStepTournamentLoadEngine(TournamentType type = TournamentType::Standard);
    ~ChatbotStepTournamentLoadEngine() override = default;

    [[nodiscard]] std::string draw() override;
private:
    TournamentType type_;
    std::string result_;
    
    enum class State {
        Input,
        Detecting,
        Summary
    };
    
    State state_ = State::Input;
    std::vector<std::string> addedEnginePaths_;
    bool detectionStarted_ = false;

    /**
     * @brief Gets the engine selection for the tournament.
     * @return Reference to the engine selection.
     */
    [[nodiscard]] ImGuiEngineSelect& getEngineSelect();

    void drawInput();
    void showAddedEngines();
    void drawDetecting();
    
    void addEngines();
    void startDetection();
    void selectAddedEngines();
};

} // namespace QaplaWindows::ChatBot
