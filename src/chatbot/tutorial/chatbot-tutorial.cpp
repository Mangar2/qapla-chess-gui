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

#include "chatbot-tutorial.h"
#include "imgui-controls.h"
#include "tutorial.h"

#include <imgui.h>
#include <format>

namespace QaplaWindows::ChatBot {

namespace {

/**
 * @brief Gets the display name for a tutorial.
 * @param name The tutorial name.
 * @return The display name string.
 */
std::string getTutorialDisplayName(Tutorial::TutorialName name) {
    switch (name) {
        case Tutorial::TutorialName::Tournament:
            return "Tournament";
        case Tutorial::TutorialName::EngineSetup:
            return "Engine Setup";
        case Tutorial::TutorialName::BoardEngines:
            return "Board Engines";
        case Tutorial::TutorialName::BoardWindow:
            return "Board Window";
        case Tutorial::TutorialName::BoardCutPaste:
            return "Board Cut & Paste";
        case Tutorial::TutorialName::Epd:
            return "EPD Analysis";
        case Tutorial::TutorialName::Snackbar:
            return "Snackbar";
        default:
            return "";
    }
}

static std::string getTutorialTopicName(Tutorial::TutorialName name) {
    switch (name) {
        case Tutorial::TutorialName::Tournament:
            return "tournament";
        case Tutorial::TutorialName::EngineSetup:
            return "engine";
        case Tutorial::TutorialName::BoardEngines:
            return "board";
        case Tutorial::TutorialName::BoardWindow:
            return "board";
        case Tutorial::TutorialName::BoardCutPaste:
            return "board";
        case Tutorial::TutorialName::Epd:
            return "epd";
        case Tutorial::TutorialName::Snackbar:
            return "snackbar";
        default:
            return "";
    }
}

} // anonymous namespace

// ============================================================================
// ChatbotTutorial implementation
// ============================================================================

void ChatbotTutorial::start() {
    steps_.clear();
    currentStepIndex_ = 0;
    stopped_ = false;

    // First step: Select tutorial
    steps_.push_back(std::make_unique<ChatbotStepTutorialSelect>());
}

bool ChatbotTutorial::draw() {
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
            // If this was the selection step, add the runner step based on selected tutorial
            if (currentStepIndex_ == 0) {
                auto* selectionStep = dynamic_cast<ChatbotStepTutorialSelect*>(steps_[0].get());
                if (selectionStep != nullptr && selectionStep->getSelectedTutorial() != Tutorial::TutorialName::Count) {
                    steps_.push_back(std::make_unique<ChatbotStepTutorialRunner>(selectionStep->getSelectedTutorial()));
                }
            }
            ++currentStepIndex_;
            return true;  // Content changed, need scroll
        }
    }

    return false;
}

bool ChatbotTutorial::isFinished() const {
    return stopped_ || (currentStepIndex_ >= steps_.size());
}

std::unique_ptr<ChatbotThread> ChatbotTutorial::clone() const {
    return std::make_unique<ChatbotTutorial>();
}

// ============================================================================
// ChatbotStepTutorialSelect implementation
// ============================================================================

const std::vector<Tutorial::TutorialName> ChatbotStepTutorialSelect::availableTutorials_ = {
    Tutorial::TutorialName::Tournament,
    Tutorial::TutorialName::Epd
};

ChatbotStepTutorialSelect::ChatbotStepTutorialSelect() {
    // Build options for selection
    std::vector<ChatbotStepOptionList::Option> options;
    options.reserve(availableTutorials_.size() + 1);
    
    for (size_t idx = 0; idx < availableTutorials_.size(); ++idx) {
        const auto tutorialName = availableTutorials_[idx];
        options.push_back({
            getTutorialDisplayName(tutorialName),
            [this, idx]() {
                selectedTutorial_ = availableTutorials_[idx];
            }
        });
    }
    
    // Add cancel option
    options.push_back({
        "Cancel",
        []() { /* selectedTutorial_ remains Count */ }
    });
    
    // Create option selector
    optionSelector_ = std::make_unique<ChatbotStepOptionList>(
        "Select a tutorial to run:",
        std::move(options)
    );
}

std::string ChatbotStepTutorialSelect::draw() {
    if (finished_) {
        // Show what was selected
        if (selectedTutorial_ != Tutorial::TutorialName::Count) {
            ImGuiControls::textWrapped(
                std::format("Selected tutorial: {}", getTutorialDisplayName(selectedTutorial_)));
        } else {
            ImGuiControls::textWrapped("Tutorial selection cancelled.");
        }
        return "";
    }

    // Delegate to option selector
    const std::string result = optionSelector_->draw();
    
    if (optionSelector_->isFinished()) {
        finished_ = true;
        // If cancel was selected, return "stop"
        if (selectedTutorial_ == Tutorial::TutorialName::Count) {
            return "stop";
        }
    }
    
    return result;
}

// ============================================================================
// ChatbotStepTutorialRunner implementation
// ============================================================================

ChatbotStepTutorialRunner::ChatbotStepTutorialRunner(Tutorial::TutorialName tutorialName)
    : tutorialName_(tutorialName)
{
}

ChatbotStepTutorialRunner::~ChatbotStepTutorialRunner() {
    removeFilter();
}

void ChatbotStepTutorialRunner::installFilter() {
    if (filterHandle_) {
        return;
    }

    // Capture messages for "tutorial" topic and the specific tutorial topic
    filterHandle_ = SnackbarManager::instance().registerFilterCallback(
        [this](const SnackbarManager::SnackbarEntry& entry) -> bool {
            // Capture messages from "tutorial" topic or the specific tutorial topic
            if (entry.topic == "tutorial" || entry.topic == getTutorialTopicName(tutorialName_) ) {
                capturedMessages_.push_back(entry);
                return false;  // Don't display in snackbar, we show it in chat
            }
            return true;  // Let other messages through
        }
    );
}

void ChatbotStepTutorialRunner::removeFilter() {
    filterHandle_.reset();  // RAII: automatically unregisters the callback
}

std::string ChatbotStepTutorialRunner::draw() {
    if (finished_) {
        ImGuiControls::textWrapped("Tutorial completed.");
        return "";
    }

    // Install filter if not already done
    if (!filterHandle_) {
        installFilter();
    }

    // Start the tutorial if not already started
    if (!tutorialStarted_) {
        Tutorial::instance().startTutorial(tutorialName_);
        tutorialStarted_ = true;
    }

    const auto& entry = Tutorial::instance().getEntry(tutorialName_);

    // Display header
    ImGui::TextColored(StepColors::SUCCESS_COLOR, "Tutorial: %s", entry.displayName.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Display all captured messages
    if (capturedMessages_.empty()) {
        ImGuiControls::textWrapped("Waiting for tutorial messages...");
    } else {
        for (size_t i = 0; i < capturedMessages_.size(); ++i) {
            const auto& msg = capturedMessages_[i];

            // Topic if present and different from "tutorial"
            if (!msg.topic.empty() && msg.topic != "tutorial") {
                ImGuiControls::textDisabled("System Message:");
            }

            // Message content
            ImGuiControls::textWrapped(msg.message);

            // Separator between messages
            if (i < capturedMessages_.size() - 1) {
                ImGui::Separator();
            }
        }
    }

    ImGui::Spacing();

    // Check if waiting for user acknowledgement
    if (Tutorial::instance().doWaitForUserInput()) {
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGuiControls::textButton("Continue")) {
            // Acknowledge and advance to next step
            Tutorial::instance().requestNextTutorialStep(tutorialName_, false);
        }
        ImGui::SameLine();
        if (ImGuiControls::textButton("Stop Tutorial")) {
            removeFilter();
            Tutorial::instance().finishTutorial(tutorialName_);
            finished_ = true;
            return "stop";
        }
        return "";
    }

    // Check if tutorial is completed
    if (entry.completed()) {
        ImGui::TextColored(StepColors::SUCCESS_COLOR, "Tutorial completed!");
        ImGui::Spacing();
        
        if (ImGuiControls::textButton("Close")) {
            removeFilter();
            finished_ = true;
        }
    } else {
        // Show progress
        ImGuiControls::textDisabled(std::format("Progress: {} / {}", entry.counter, entry.messages.size()));
        ImGui::Spacing();

        // Cancel button
        if (ImGuiControls::textButton("Stop Tutorial")) {
            removeFilter();
            Tutorial::instance().finishTutorial(tutorialName_);
            finished_ = true;
            return "stop";
        }
    }

    return "";
}

} // namespace QaplaWindows::ChatBot
