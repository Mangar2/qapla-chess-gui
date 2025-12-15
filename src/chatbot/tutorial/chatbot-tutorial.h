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

#pragma once

#include "../chatbot-thread.h"
#include "../chatbot-step.h"
#include "../chatbot-step-option-list.h"
#include "snackbar.h"
#include "tutorial.h"

#include <vector>
#include <memory>
#include <optional>
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief A chatbot thread for running tutorials interactively.
 * 
 * This thread allows the user to select a tutorial and then displays
 * tutorial messages in the chatbot instead of as snackbars.
 */
class ChatbotTutorial : public ChatbotThread {
public:
    [[nodiscard]] std::string getTitle() const override { 
        return "Tutorial"; 
    }
    void start() override;
    bool draw() override;
    [[nodiscard]] bool isFinished() const override;
    [[nodiscard]] std::unique_ptr<ChatbotThread> clone() const override;

private:
    std::vector<std::unique_ptr<ChatbotStep>> steps_;
    size_t currentStepIndex_ = 0;
    bool stopped_ = false;
};

/**
 * @brief Configuration for a tutorial including where it should run.
 */
struct TutorialConfig {
    Tutorial::TutorialName name;   ///< Tutorial identifier
    bool runsInChatbot;            ///< true: runs in chatbot, false: runs via snackbar
};

/**
 * @brief A chatbot step for selecting which tutorial to run.
 * 
 * Uses ChatbotStepOptionList for the actual selection UI.
 */
class ChatbotStepTutorialSelect : public ChatbotStep {
public:
    ChatbotStepTutorialSelect();
    [[nodiscard]] std::string draw() override;
    
    /**
     * @brief Gets the selected tutorial configuration.
     * @return The selected tutorial config, or nullopt if none/cancelled.
     */
    [[nodiscard]] std::optional<TutorialConfig> getSelectedTutorial() const {
        return selectedTutorial_;
    }

private:
    /// List of available tutorials with their configuration
    static const std::vector<TutorialConfig> availableTutorials_;
    
    std::unique_ptr<ChatbotStepOptionList> optionSelector_;
    std::optional<TutorialConfig> selectedTutorial_;
};

/**
 * @brief A chatbot step that displays tutorial messages captured from the SnackbarManager.
 * 
 * This step sets up a filter callback on the SnackbarManager to intercept tutorial
 * messages and display them in the chatbot instead.
 */
class ChatbotStepTutorialRunner : public ChatbotStep {
public:
    /**
     * @brief Constructs a tutorial runner for the specified tutorial.
     * @param tutorialName The tutorial to run.
     */
    explicit ChatbotStepTutorialRunner(Tutorial::TutorialName tutorialName);
    ~ChatbotStepTutorialRunner() override;

    [[nodiscard]] std::string draw() override;

private:
    Tutorial::TutorialName tutorialName_;
    std::vector<SnackbarManager::SnackbarEntry> capturedMessages_;
    std::unique_ptr<Callback::UnregisterHandle> filterHandle_;  ///< RAII handle for filter callback
    bool tutorialStarted_ = false;

    /**
     * @brief Installs the filter callback on the SnackbarManager.
     */
    void installFilter();

    /**
     * @brief Removes the filter callback from the SnackbarManager.
     */
    void removeFilter();
};

/**
 * @brief A chatbot step that informs the user about a snackbar-based tutorial.
 * 
 * For tutorials that don't run in the chatbot, this step explains that
 * messages will appear as snackbars while the board stays in the foreground,
 * and provides buttons to switch to the board or cancel.
 */
class ChatbotStepTutorialSnackbarInfo : public ChatbotStep {
public:
    /**
     * @brief Constructs the info step for a snackbar-based tutorial.
     * @param tutorialName The tutorial to run via snackbar.
     */
    explicit ChatbotStepTutorialSnackbarInfo(Tutorial::TutorialName tutorialName);
    ~ChatbotStepTutorialSnackbarInfo() override = default;

    [[nodiscard]] std::string draw() override;

private:
    Tutorial::TutorialName tutorialName_;
    bool tutorialStarted_ = false;
};

} // namespace QaplaWindows::ChatBot
