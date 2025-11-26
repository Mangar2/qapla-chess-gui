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
#include "chatbot-step-tournament-stop-running.h"
#include "chatbot-step-tournament-save-existing.h"
#include "chatbot-step-tournament-global-settings.h"
#include "chatbot-step-tournament-select-engines.h"
#include "chatbot-step-tournament-load-engine.h"
#include "chatbot-step-tournament-configuration.h"
#include "chatbot-step-tournament-pgn.h"
#include "chatbot-step-tournament-start.h"

namespace QaplaWindows::ChatBot {

void ChatbotTournament::start() {
    steps_.clear();
    activeStepIndices_.clear();
    stopped_ = false;

    // Step 0: Check if a tournament is running and offer to stop it
    steps_.push_back(std::make_unique<ChatbotStepTournamentStopRunning>());

    // Step 1: Check if existing tournament needs saving
    steps_.push_back(std::make_unique<ChatbotStepTournamentSaveExisting>());
    
    // Step 2: Configure global engine settings (hash, time control)
    steps_.push_back(std::make_unique<ChatbotStepTournamentGlobalSettings>());
    
    // Step 3: Select engines from existing list
    steps_.push_back(std::make_unique<ChatbotStepTournamentSelectEngines>());

    // Step 4: Load more engines
    steps_.push_back(std::make_unique<ChatbotStepTournamentLoadEngine>());

    // Step 5: Configure tournament settings (type, rounds, games)
    steps_.push_back(std::make_unique<ChatbotStepTournamentConfiguration>());

    // Step 6: Select PGN file for results
    steps_.push_back(std::make_unique<ChatbotStepTournamentPgn>());

    // Step 7: Start Tournament
    steps_.push_back(std::make_unique<ChatbotStepTournamentStart>());

    // Start with the first step
    activeStepIndices_.push_back(0);
}

void ChatbotTournament::draw() {
    if (stopped_ || activeStepIndices_.empty()) {
        return;
    }

    // Draw all active steps
    for (size_t activeIdx : activeStepIndices_) {
        if (activeIdx >= steps_.size()) {
            continue;
        }
        std::string result = steps_[activeIdx]->draw();
        if (result == "stop") {
            stopped_ = true;
            return;
        }
        if (result == "start") {
            // Jump to the last step (tournament-start)
            activeStepIndices_.push_back(steps_.size() - 1);
            return;
        }
    }

    // Check if we need to advance to the next step
    size_t lastActiveIdx = activeStepIndices_.back();
    if (steps_[lastActiveIdx]->isFinished()) {
        size_t nextIdx = lastActiveIdx + 1;
        if (nextIdx < steps_.size()) {
            activeStepIndices_.push_back(nextIdx);
        }
    }
}

bool ChatbotTournament::isFinished() const {
    if (stopped_) {
        return true;
    }
    if (activeStepIndices_.empty()) {
        return false;
    }
    size_t lastActiveIdx = activeStepIndices_.back();
    // Finished when the last step in steps_ is active and finished
    return lastActiveIdx == steps_.size() - 1 && steps_[lastActiveIdx]->isFinished();
}

std::unique_ptr<ChatbotThread> ChatbotTournament::clone() const {
    return std::make_unique<ChatbotTournament>();
}

void ChatbotTournament::addStep(std::unique_ptr<ChatbotStep> step) {
    // Insert after current active step in steps_ vector
    if (!activeStepIndices_.empty()) {
        size_t lastActiveIdx = activeStepIndices_.back();
        steps_.insert(steps_.begin() + lastActiveIdx + 1, std::move(step));
    } else {
        steps_.push_back(std::move(step));
    }
}

} // namespace QaplaWindows::ChatBot
