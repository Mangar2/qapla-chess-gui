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

#pragma once

#include "embedded-window.h"
#include "chatbot-thread.h"
#include "chatbot-step.h"
#include <vector>
#include <memory>

namespace QaplaWindows::ChatBot {

/**
 * @brief Window that provides a chatbot-like interface for simplified user interaction.
 */
class ChatbotWindow : public EmbeddedWindow {
public:
    ChatbotWindow();
    ~ChatbotWindow() override = default;

    void draw() override;

    /**
     * @brief Registers a new thread type that the user can select.
     * @param thread The thread prototype to register.
     */
    void registerThread(std::unique_ptr<ChatbotThread> thread);

    /**
     * @brief Gets the singleton instance of the ChatbotWindow.
     * @return Pointer to the singleton instance.
     */
    static ChatbotWindow* instance() {
        static ChatbotWindow instance;
        return &instance;
    }

    /**
     * @brief Resets the chatbot to its initial state.
     * 
     * Clears all active and completed threads, returning to the main menu.
     * Use this in tests to ensure a clean starting state.
     */
    void reset();

private:
    /**
     * @brief Starts a new instance of the given thread prototype.
     * @param threadPrototype The prototype to clone and start.
     */
    void startThread(const ChatbotThread& threadPrototype);

    /**
     * @brief Resets the window to the main menu state.
     */
    void resetToMainMenu();

    /**
     * @brief Initializes the registered thread prototypes.
     * 
     * Creates fresh instances of all available chatbot threads.
     */
    void initializeThreads();

    std::vector<std::unique_ptr<ChatbotThread>> registeredThreads_;
    std::vector<std::unique_ptr<ChatbotThread>> completedThreads_;
    
    std::unique_ptr<ChatbotThread> activeThread_;
    std::unique_ptr<ChatbotStep> mainMenuStep_;
    
    float lastCursorY_ = 0.0F;  ///< Last cursor Y position to detect content growth
};

} // namespace QaplaWindows::ChatBot
