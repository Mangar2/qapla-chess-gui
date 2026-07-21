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

#include "chatbot-add-engines.h"
#include "chatbot-step-add-engines-welcome.h"
#include "../chatbot-step-load-engine.h"
#include "../chatbot-step-finish.h"
#include <engine-handling/engine-worker-factory.h>

using QaplaTester::EngineWorkerFactory;

namespace QaplaWindows::ChatBot {

ChatbotAddEngines::ChatbotAddEngines() {
    // Create engine select instance with appropriate options
    QaplaWindows::ImGuiEngineSelect::Options options;
    options.allowProtocolEdit = true;
    options.allowGauntletEdit = false;
    options.allowNameEdit = true;
    options.allowPonderEdit = true;
    options.allowTimeControlEdit = true;
    options.allowTraceLevelEdit = true;
    options.allowRestartOptionEdit = true;
    options.allowEngineOptionsEdit = true;
    options.allowMultipleSelection = false;
    options.directEditMode = true;
    options.enginesDefaultOpen = true;
    options.allowEngineConfiguration = true;
    
    engineSelect_ = std::make_unique<QaplaWindows::ImGuiEngineSelect>(options);
    engineSelect_->setId("add-engines-chatbot");
    
    // Initialize with all existing engines to enable duplicate detection
    auto& configManager = EngineWorkerFactory::getConfigManagerMutable();
    auto configs = configManager.getAllConfigs();
    std::vector<QaplaWindows::ImGuiEngineSelect::EngineConfiguration> existingEngines;
    for (const auto& config : configs) {
        existingEngines.push_back({
            .config = config,
            .selected = false,
            .originalName = config.getName()
        });
    }
    engineSelect_->setEngineConfigurations(existingEngines);
}

void ChatbotAddEngines::start() {
    steps_.clear();
    currentStepIndex_ = 0;
    stopped_ = false;
    addInitialSteps();
}

void ChatbotAddEngines::addInitialSteps() {
    steps_.push_back(std::make_unique<ChatbotStepAddEnginesWelcome>());
    
    // Use existing load engine step with our own engine select instance
    auto engineSelectProvider = [this]() -> QaplaWindows::ImGuiEngineSelect* {
        return engineSelect_.get();
    };
    steps_.push_back(std::make_unique<ChatbotStepLoadEngine>(engineSelectProvider, 0, "engine list"));
    
    steps_.push_back(std::make_unique<ChatbotStepFinish>(
        "Engines have been added to the global engine list. You can now use them in tournaments, "
        "EPD analysis, and interactive boards. Note: Each engine can only be added once (file path is unique), "
        "but you can use the same engine multiple times with different configurations."));
}

bool ChatbotAddEngines::draw() {
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

        if (steps_[currentStepIndex_]->isFinished()) {
            ++currentStepIndex_;
            return true;
        }
    }

    return false;
}

bool ChatbotAddEngines::isFinished() const {
    return stopped_ || currentStepIndex_ >= steps_.size();
}

std::unique_ptr<ChatbotThread> ChatbotAddEngines::clone() const {
    return std::make_unique<ChatbotAddEngines>();
}

} // namespace QaplaWindows::ChatBot
