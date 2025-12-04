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

#include "chatbot-step.h"
#include "snackbar.h"
#include "callback-manager.h"

#include <vector>
#include <memory>
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief A step that displays a captured snackbar message in the chatbot.
 * 
 * This step is automatically created by SnackbarCapture when a message
 * with a matching topic is received. It displays the message and is
 * immediately finished to not block the conversation flow.
 */
class ChatbotStepSnackbarMessage : public ChatbotStep {
public:
    /**
     * @brief Constructs a snackbar message step.
     * @param entry The snackbar entry to display.
     */
    explicit ChatbotStepSnackbarMessage(SnackbarManager::SnackbarEntry entry);

    [[nodiscard]] std::string draw() override;

private:
    SnackbarManager::SnackbarEntry entry_;
};

/**
 * @brief Helper class for capturing snackbar messages in chatbot threads.
 * 
 * Captures messages with specific topics and inserts them as steps into
 * the chatbot's step list at the appropriate position.
 * 
 * Usage:
 * @code
 * class MyChatbotThread : public ChatbotThread {
 *     SnackbarCapture snackbarCapture_{"my-topic"};
 *     std::vector<std::unique_ptr<ChatbotStep>> steps_;
 *     size_t currentStepIndex_ = 0;
 *     
 *     void start() override {
 *         snackbarCapture_.install();
 *     }
 *     
 *     bool draw() override {
 *         // Insert captured messages as steps after currentStepIndex_
 *         snackbarCapture_.insertCapturedSteps(steps_, currentStepIndex_);
 *         // ... rest of draw logic
 *     }
 * };
 * @endcode
 */
class SnackbarCapture {
public:
    /**
     * @brief Constructs a snackbar capture for a single topic.
     * @param topic The topic to capture.
     */
    explicit SnackbarCapture(std::string topic);

    /**
     * @brief Constructs a snackbar capture for multiple topics.
     * @param topics The topics to capture.
     */
    explicit SnackbarCapture(std::vector<std::string> topics);

    ~SnackbarCapture() = default;

    // Move only
    SnackbarCapture(SnackbarCapture&&) = default;
    SnackbarCapture& operator=(SnackbarCapture&&) = default;
    SnackbarCapture(const SnackbarCapture&) = delete;
    SnackbarCapture& operator=(const SnackbarCapture&) = delete;

    /**
     * @brief Installs the filter callback on the SnackbarManager.
     * 
     * After calling this, messages with matching topics will be captured
     * instead of displayed in the snackbar.
     */
    void install();

    /**
     * @brief Uninstalls the filter callback.
     * 
     * Messages will no longer be captured and will be displayed normally.
     */
    void uninstall();

    /**
     * @brief Checks if the capture is currently installed.
     * @return true if installed, false otherwise.
     */
    [[nodiscard]] bool isInstalled() const {
        return filterHandle_ != nullptr;
    }

    /**
     * @brief Inserts captured messages as steps into the step list.
     * 
     * New message steps are inserted right after currentStepIndex.
     * The steps are immediately marked as finished so they don't block
     * the conversation flow.
     * 
     * @param steps The step list to insert into.
     * @param currentStepIndex The current step index (messages are inserted after this).
     */
    void insertCapturedSteps(
        std::vector<std::unique_ptr<ChatbotStep>>& steps,
        size_t currentStepIndex);

    /**
     * @brief Checks if there are pending captured messages.
     * @return true if there are messages waiting to be inserted.
     */
    [[nodiscard]] bool hasPendingMessages() const {
        return !pendingMessages_.empty();
    }

private:
    std::vector<std::string> topics_;
    std::vector<SnackbarManager::SnackbarEntry> pendingMessages_;
    std::unique_ptr<Callback::UnregisterHandle> filterHandle_;

    /**
     * @brief Checks if a topic matches any of the configured topics.
     * @param topic The topic to check.
     * @return true if the topic matches, false otherwise.
     */
    [[nodiscard]] bool matchesTopic(const std::string& topic) const;
};

} // namespace QaplaWindows::ChatBot
