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

#include "chatbot-tournament.h"
#include "chatbot-step-tournament-save-existing.h"
#include "chatbot-step-tournament-global-settings.h"
#include "chatbot-step-tournament-select-engines.h"
#include "chatbot-step-tournament-load-engine.h"
#include "chatbot-step-tournament-pgn.h"
#include "chatbot-step-tournament-start.h"

namespace QaplaWindows::ChatBot {

void ChatbotTournament::start() {
    steps_.clear();
    currentStep_ = 0;

    // Step 1: Check if existing tournament needs saving
    steps_.push_back(std::make_unique<ChatbotStepTournamentSaveExisting>());
    
    // Step 2: Configure global engine settings (hash, time control)
    steps_.push_back(std::make_unique<ChatbotStepTournamentGlobalSettings>());
    
    // Step 3: Select engines from existing list
    steps_.push_back(std::make_unique<ChatbotStepTournamentSelectEngines>());

    // Step 4: Load more engines
    steps_.push_back(std::make_unique<ChatbotStepTournamentLoadEngine>());

    // Step 5: Select PGN file for results
    steps_.push_back(std::make_unique<ChatbotStepTournamentPgn>());

    // Step 6: Start Tournament
    steps_.push_back(std::make_unique<ChatbotStepTournamentStart>());
}

void ChatbotTournament::draw() {
    for (size_t i = 0; i < steps_.size(); ++i) {
        if (i <= currentStep_) {
            std::string result = steps_[i]->draw();
            if (result == "stop") {
                // Abort the thread
                currentStep_ = steps_.size();
                return;
            }
        }
    }

    if (currentStep_ < steps_.size()) {
        if (steps_[currentStep_]->isFinished()) {
            ++currentStep_;
        }
    }
}

bool ChatbotTournament::isFinished() const {
    return currentStep_ >= steps_.size();
}

std::unique_ptr<ChatbotThread> ChatbotTournament::clone() const {
    return std::make_unique<ChatbotTournament>();
}

void ChatbotTournament::addStep(std::unique_ptr<ChatbotStep> step) {
    // Insert after current step
    if (currentStep_ < steps_.size()) {
        steps_.insert(steps_.begin() + currentStep_ + 1, std::move(step));
    } else {
        steps_.push_back(std::move(step));
    }
}

} // namespace QaplaWindows::ChatBot
