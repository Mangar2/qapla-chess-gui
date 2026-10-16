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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include "chatbot-bookeval.h"
#include "chatbot-step-bookeval-files.h"
#include "chatbot-step-bookeval-limit.h"
#include "chatbot-step-bookeval-start.h"
#include "../chatbot-step-select-engines.h"
#include "../chatbot-step-load-engine.h"
#include "../../book-eval-data.h"

#include <algorithm>

namespace QaplaWindows::ChatBot {

void ChatbotBookEval::start() {
    steps_.clear();
    currentStepIndex_ = 0;
    stopped_ = false;
    snackbarCapture_.install();

    if (BookEvalData::instance().isBusy()) {
        steps_.push_back(std::make_unique<ChatbotStepBookEvalStart>(true));
        return;
    }
    auto engineSelectProvider = []() -> ImGuiEngineSelect* {
        return &BookEvalData::instance().getEngineSelect();
    };
    steps_.push_back(std::make_unique<ChatbotStepBookEvalFiles>());
    steps_.push_back(std::make_unique<ChatbotStepSelectEngines>(engineSelectProvider, "book evaluation"));
    steps_.push_back(std::make_unique<ChatbotStepLoadEngine>(engineSelectProvider, 1, "book evaluation"));
    steps_.push_back(std::make_unique<ChatbotStepBookEvalLimit>());
    steps_.push_back(std::make_unique<ChatbotStepBookEvalStart>());
}

bool ChatbotBookEval::draw() {
    bool contentChanged = false;
    if (stopped_ || steps_.empty()) {
        return false;
    }
    snackbarCapture_.insertCapturedSteps(steps_, std::max<size_t>(1, currentStepIndex_) - 1);

    for (size_t i = 0; i < currentStepIndex_ && i < steps_.size(); ++i) {
        static_cast<void>(steps_[i]->draw());
    }
    if (currentStepIndex_ < steps_.size()) {
        const std::string result = steps_[currentStepIndex_]->draw();
        if (result == "stop") {
            stopped_ = true;
            return false;
        }
        if (steps_.size() > currentStepIndex_ && steps_[currentStepIndex_]->isFinished()) {
            ++currentStepIndex_;
            contentChanged = true;
        }
    }
    return contentChanged;
}

bool ChatbotBookEval::isFinished() const {
    if (stopped_) {
        return true;
    }
    if (steps_.empty()) {
        return false;
    }
    return currentStepIndex_ >= steps_.size();
}

std::unique_ptr<ChatbotThread> ChatbotBookEval::clone() const {
    return std::make_unique<ChatbotBookEval>();
}

} // namespace QaplaWindows::ChatBot
