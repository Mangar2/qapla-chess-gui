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

#include <string>
#include <memory>

namespace QaplaWindows::ChatBot {

/**
 * @brief Interface for a chatbot conversation thread.
 * 
 * A thread consists of a sequence of steps that guide the user through a specific task.
 */
class ChatbotThread {
public:
    virtual ~ChatbotThread() = default;

    /**
     * @brief Gets the title of the thread.
     * @return The title string.
     */
    [[nodiscard]] virtual std::string getTitle() const = 0;

    /**
     * @brief Starts the thread.
     * 
     * This should initialize the first step(s).
     */
    virtual void start() = 0;

    /**
     * @brief Renders the current state of the thread.
     */
    virtual void draw() = 0;

    /**
     * @brief Checks if the thread is finished.
     * @return true if the thread is completed, false otherwise.
     */
    [[nodiscard]] virtual bool isFinished() const = 0;

    /**
     * @brief Creates a clone of the thread.
     * 
     * Needed because the window holds prototypes.
     * @return A unique pointer to a new instance of the thread.
     */
    [[nodiscard]] virtual std::unique_ptr<ChatbotThread> clone() const = 0;
};

} // namespace QaplaWindows::ChatBot
