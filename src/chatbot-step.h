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

namespace QaplaWindows::ChatBot {

/**
 * @brief Interface for a single step in a chatbot conversation.
 * 
 * A step represents a single interaction or action within a chatbot thread.
 */
class ChatbotStep {
public:
    virtual ~ChatbotStep() = default;

    /**
     * @brief Renders the step's UI.
     * @return Empty string for normal continuation, "stop" to abort the thread.
     */
    [[nodiscard]] virtual std::string draw() = 0;

    /**
     * @brief Checks if the step is finished.
     * @return true if the step is completed, false otherwise.
     */
    [[nodiscard]] virtual bool isFinished() const = 0;
};

} // namespace QaplaWindows::ChatBot
