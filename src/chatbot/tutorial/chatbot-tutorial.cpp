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
#include "callback-manager.h"
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
            return "Add Engines";
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
            // If this was the selection step, add the appropriate step based on selected tutorial
            if (currentStepIndex_ == 0) {
                auto* selectionStep = dynamic_cast<ChatbotStepTutorialSelect*>(steps_[0].get());
                if (selectionStep != nullptr) {
                    auto selectedConfig = selectionStep->getSelectedTutorial();
                    if (selectedConfig.has_value()) {
                        if (selectedConfig->runsInChatbot) {
                            // Tutorial runs in chatbot - use runner step
                            steps_.push_back(std::make_unique<ChatbotStepTutorialRunner>(selectedConfig->name));
                        } else {
                            // Tutorial runs via snackbar - show info step
                            steps_.push_back(std::make_unique<ChatbotStepTutorialSnackbarInfo>(selectedConfig->name));
                        }
                    }
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

const std::vector<TutorialConfig> ChatbotStepTutorialSelect::availableTutorials_ = {
    { Tutorial::TutorialName::Tournament, true },   // Runs in chatbot
    { Tutorial::TutorialName::Epd, true },          // Runs in chatbot
    { Tutorial::TutorialName::EngineSetup, true },  // Runs in chatbot
    { Tutorial::TutorialName::BoardEngines, false },   // Runs via snackbar
    { Tutorial::TutorialName::BoardWindow, false },    // Runs via snackbar
    { Tutorial::TutorialName::BoardCutPaste, false }   // Runs via snackbar
};

ChatbotStepTutorialSelect::ChatbotStepTutorialSelect() {
    // Build options for selection
    std::vector<ChatbotStepOptionList::Option> options;
    options.reserve(availableTutorials_.size() + 1);
    
    for (size_t idx = 0; idx < availableTutorials_.size(); ++idx) {
        const auto& config = availableTutorials_[idx];
        options.push_back({
            getTutorialDisplayName(config.name),
            [this, idx]() {
                selectedTutorial_ = availableTutorials_[idx];
            }
        });
    }
    
    // Add cancel option
    options.push_back({
        "Cancel",
        []() { /* selectedTutorial_ remains nullopt */ }
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
        if (selectedTutorial_.has_value()) {
            ImGuiControls::textWrapped(
                std::format("Selected tutorial: {}", getTutorialDisplayName(selectedTutorial_->name)));
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
        if (!selectedTutorial_.has_value()) {
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
        QaplaWindows::ImGuiControls::hooverTooltip("Stop and exit the tutorial. Progress may be lost; saved settings remain.");
        return "";
    }

    // Check if tutorial is completed
    if (!entry.running()) {
        ImGui::TextColored(StepColors::SUCCESS_COLOR, "Tutorial completed!");
        ImGui::Spacing();
        
        if (ImGuiControls::textButton("Close")) {
            removeFilter();
            finished_ = true;
        }
        QaplaWindows::ImGuiControls::hooverTooltip("Close the tutorial dialog and return to the main UI.");
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
        QaplaWindows::ImGuiControls::hooverTooltip("Stop and exit the tutorial. Progress may be lost; saved settings remain.");
    }

    return "";
}

// ============================================================================
// ChatbotStepTutorialSnackbarInfo implementation
// ============================================================================

ChatbotStepTutorialSnackbarInfo::ChatbotStepTutorialSnackbarInfo(Tutorial::TutorialName tutorialName)
    : tutorialName_(tutorialName)
{
}

std::string ChatbotStepTutorialSnackbarInfo::draw() {
    if (finished_) {
        ImGuiControls::textWrapped("Tutorial started. Check the board view for messages.");
        return "";
    }

    const auto& entry = Tutorial::instance().getEntry(tutorialName_);

    // Display header
    ImGui::TextColored(StepColors::SUCCESS_COLOR, "Tutorial: %s", entry.displayName.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Explain that this tutorial runs via snackbar
    ImGuiControls::textWrapped(
        "This tutorial runs via notification messages while the board stays in the foreground.");
    ImGui::Spacing();
    ImGuiControls::textWrapped(
        "The tutorial messages will appear as notifications on screen. "
        "You can also view all messages in the chatbot under 'Messages'.");
    ImGui::Spacing();

    ImGui::Separator();
    ImGui::Spacing();

    // Switch to board & finish button
    if (ImGuiControls::textButton("Switch to Board & Start")) {
        // Start the tutorial 
        if (!tutorialStarted_) {
            Tutorial::instance().startTutorial(tutorialName_);
            tutorialStarted_ = true;
        }
        // Switch to first board (board 0)
        StaticCallbacks::message().invokeAll("switch_to_board_1");
        finished_ = true;
    }
    QaplaWindows::ImGuiControls::hooverTooltip(
        "Switch to the board view and start the tutorial. Messages will appear as notifications.");

    ImGui::SameLine();

    // Cancel button
    if (ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }
    QaplaWindows::ImGuiControls::hooverTooltip(
        "Cancel and return to the menu without starting the tutorial.");

    return "";
}

} // namespace QaplaWindows::ChatBot
