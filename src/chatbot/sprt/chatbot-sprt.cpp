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

#include "chatbot-sprt.h"
#include "../chatbot-step-tournament-stop-running.h"
#include "../chatbot-step-tournament-menu.h"
#include "../chatbot-step-tournament-global-settings.h"
#include "../chatbot-step-tournament-select-engines.h"
#include "../chatbot-step-tournament-load-engine.h"
#include "../chatbot-step-tournament-opening.h"
#include "../chatbot-step-tournament-pgn.h"
#include "chatbot-step-sprt-configuration.h"

namespace QaplaWindows::ChatBot {

void ChatbotSprt::start() {
    steps_.clear();
    currentStepIndex_ = 0;
    stopped_ = false;

    // Only add the initial steps - more steps are added dynamically based on user choice
    steps_.push_back(std::make_unique<ChatbotStepTournamentStopRunning>(TournamentType::Sprt));
}

void ChatbotSprt::addNewSprtSteps() {
    steps_.push_back(std::make_unique<ChatbotStepTournamentGlobalSettings>(TournamentType::Sprt));
    steps_.push_back(std::make_unique<ChatbotStepTournamentSelectEngines>(TournamentType::Sprt));
    steps_.push_back(std::make_unique<ChatbotStepTournamentLoadEngine>(TournamentType::Sprt));
    steps_.push_back(std::make_unique<ChatbotStepSprtConfiguration>());
    steps_.push_back(std::make_unique<ChatbotStepTournamentOpening>(TournamentType::Sprt));
    steps_.push_back(std::make_unique<ChatbotStepTournamentPgn>(TournamentType::Sprt));
    // TODO: steps_.push_back(std::make_unique<ChatbotStepSprtStart>());
}

bool ChatbotSprt::draw() {
    bool contentChanged = false;
    
    if (stopped_ || steps_.empty()) {
        return false;
    }

    // Draw all completed steps 
    for (size_t i = 0; i < currentStepIndex_ && i < steps_.size(); ++i) {
        static_cast<void>(steps_[i]->draw());
    }

    // Draw and handle current step
    if (currentStepIndex_ < steps_.size()) {
        std::string result = steps_[currentStepIndex_]->draw();
        
        if (result == "stop") {
            stopped_ = true;
            return false;
        }

        if (result == "menu") {
            steps_.push_back(std::make_unique<ChatbotStepTournamentMenu>(TournamentType::Sprt));
        }
        
        if (result == "new") {
            addNewSprtSteps();
        }
        
        if (result == "load") {
            // TODO: steps_.push_back(std::make_unique<ChatbotStepSprtLoad>());
        }
        
        if (result == "start") {
            // TODO: steps_.push_back(std::make_unique<ChatbotStepSprtStart>());
        }

        if (result == "existing") {
            // TODO: steps_.push_back(std::make_unique<ChatbotStepSprtContinueExisting>());
        }
        
        // Advance to next step if current is finished
        if (steps_.size() > currentStepIndex_ && steps_[currentStepIndex_]->isFinished()) {
            ++currentStepIndex_;
            contentChanged = true;
        }
    }
    
    return contentChanged;
}

bool ChatbotSprt::isFinished() const {
    if (stopped_) {
        return true;
    }
    if (steps_.empty()) {
        return false;
    }
    // Finished when past the last step
    return currentStepIndex_ >= steps_.size();
}

std::unique_ptr<ChatbotThread> ChatbotSprt::clone() const {
    return std::make_unique<ChatbotSprt>();
}

} // namespace QaplaWindows::ChatBot
