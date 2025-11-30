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
            // If this was the selection step, add the runner step based on result
            if (currentStepIndex_ == 0 && !result.empty() && result != "stop") {
                auto selectedTutorial = Tutorial::stringToTutorialName(result);
                if (selectedTutorial != Tutorial::TutorialName::Count) {
                    // User selected a tutorial, add the runner step
                    steps_.push_back(std::make_unique<ChatbotStepTutorialRunner>(selectedTutorial));
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

const std::vector<std::string> ChatbotStepTutorialSelect::availableTutorials_ = {
    "Tournament"
};

std::string ChatbotStepTutorialSelect::draw() {
    if (finished_) {
        // Show what was selected
        if (!selectedTutorialName_.empty()) {
            ImGuiControls::textWrapped(
                std::format("Selected tutorial: {}", selectedTutorialName_));
        } else {
            ImGuiControls::textWrapped("Tutorial selection cancelled.");
        }
        return "";
    }

    ImGuiControls::textWrapped("Select a tutorial to run:");
    ImGui::Spacing();

    // Draw tutorial buttons from vector
    for (const auto& tutorialName : availableTutorials_) {
        if (ImGuiControls::textButton(tutorialName.c_str())) {
            selectedTutorialName_ = tutorialName;
            finished_ = true;
            return tutorialName;  // Return the selected tutorial name
        }
        ImGui::Spacing();
    }

    // Cancel button
    if (ImGuiControls::textButton("Cancel")) {
        selectedTutorialName_.clear();
        finished_ = true;
        return "stop";
    }

    return "";
}

// ============================================================================
// ChatbotStepTutorialRunner implementation
// ============================================================================

ChatbotStepTutorialRunner::ChatbotStepTutorialRunner(Tutorial::TutorialName tutorialName)
    : tutorialName_(tutorialName)
    , tutorialTopicName_(getTutorialTopicName(tutorialName))
{
}

ChatbotStepTutorialRunner::~ChatbotStepTutorialRunner() {
    removeFilter();
}

std::string ChatbotStepTutorialRunner::getTutorialTopicName(Tutorial::TutorialName name) {
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

void ChatbotStepTutorialRunner::installFilter() {
    if (filterInstalled_) {
        return;
    }

    // Capture messages for "tutorial" topic and the specific tutorial topic
    SnackbarManager::instance().setFilterCallback(
        [this](const SnackbarManager::SnackbarEntry& entry) -> bool {
            // Capture messages from "tutorial" topic or the specific tutorial topic
            if (entry.topic == "tutorial" || entry.topic == tutorialTopicName_) {
                capturedMessages_.push_back(entry);
                return false;  // Don't display in snackbar, we show it in chat
            }
            return true;  // Let other messages through
        }
    );
    filterInstalled_ = true;
}

void ChatbotStepTutorialRunner::removeFilter() {
    if (filterInstalled_) {
        SnackbarManager::instance().setFilterCallback(nullptr);
        filterInstalled_ = false;
    }
}

std::string ChatbotStepTutorialRunner::draw() {
    if (finished_) {
        ImGuiControls::textWrapped("Tutorial completed.");
        return "";
    }

    // Install filter if not already done
    if (!filterInstalled_) {
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
