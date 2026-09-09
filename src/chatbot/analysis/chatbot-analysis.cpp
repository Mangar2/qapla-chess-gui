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

#include "chatbot-analysis.h"
#include "chatbot-step-analysis-stop-running.h"
#include "chatbot-step-analysis-input.h"
#include "chatbot-step-analysis-filter.h"
#include "chatbot-step-analysis-time.h"
#include "chatbot-step-analysis-output.h"
#include "chatbot-step-analysis-start.h"
#include "../chatbot-step-select-engines.h"
#include "../chatbot-step-load-engine.h"
#include "../../analysis-data.h"

#include <algorithm>

namespace QaplaWindows::ChatBot {

void ChatbotAnalysis::start() {
    steps_.clear();
    currentStepIndex_ = 0;
    stopped_ = false;

    snackbarCapture_.install();

    steps_.push_back(std::make_unique<ChatbotStepAnalysisStopRunning>());
}

void ChatbotAnalysis::addSetupSteps() {
    auto engineSelectProvider = []() -> ImGuiEngineSelect* {
        return &AnalysisData::instance().getEngineSelect();
    };
    steps_.push_back(std::make_unique<ChatbotStepAnalysisInput>());
    steps_.push_back(std::make_unique<ChatbotStepAnalysisFilter>());
    steps_.push_back(std::make_unique<ChatbotStepSelectEngines>(engineSelectProvider, "backward analysis"));
    steps_.push_back(std::make_unique<ChatbotStepLoadEngine>(engineSelectProvider, 1, "analysis"));
    steps_.push_back(std::make_unique<ChatbotStepAnalysisTime>());
    steps_.push_back(std::make_unique<ChatbotStepAnalysisOutput>());
    steps_.push_back(std::make_unique<ChatbotStepAnalysisStart>());
}

bool ChatbotAnalysis::draw() {
    bool contentChanged = false;

    if (stopped_ || steps_.empty()) {
        return false;
    }

    snackbarCapture_.insertCapturedSteps(steps_, std::max<size_t>(1, currentStepIndex_) - 1);

    for (size_t i = 0; i < currentStepIndex_ && i < steps_.size(); ++i) {
        static_cast<void>(steps_[i]->draw());
    }

    if (currentStepIndex_ < steps_.size()) {
        std::string result = steps_[currentStepIndex_]->draw();

        if (result == "stop") {
            stopped_ = true;
            return false;
        }
        if (result == "continue") {
            addSetupSteps();
        }

        if (steps_.size() > currentStepIndex_ && steps_[currentStepIndex_]->isFinished()) {
            ++currentStepIndex_;
            contentChanged = true;
        }
    }

    return contentChanged;
}

bool ChatbotAnalysis::isFinished() const {
    if (stopped_) {
        return true;
    }
    if (steps_.empty()) {
        return false;
    }
    return currentStepIndex_ >= steps_.size();
}

std::unique_ptr<ChatbotThread> ChatbotAnalysis::clone() const {
    return std::make_unique<ChatbotAnalysis>();
}

} // namespace QaplaWindows::ChatBot
