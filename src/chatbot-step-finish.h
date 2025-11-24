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

#include "chatbot-step.h"
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief A chatbot step that displays a message and a finish button.
 */
class ChatbotStepFinish : public ChatbotStep {
public:
    /**
     * @brief Constructs a new ChatbotStepFinish.
     * @param message The message to display.
     */
    explicit ChatbotStepFinish(std::string message);

    void draw() override;
    [[nodiscard]] bool isFinished() const override;

private:
    std::string message_;
    bool finished_ = false;
};

} // namespace QaplaWindows::ChatBot
