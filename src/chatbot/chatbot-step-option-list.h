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
#include <vector>
#include <string>
#include <functional>

namespace QaplaWindows::ChatBot {

/**
 * @brief A chatbot step that presents a list of options to the user.
 */
class ChatbotStepOptionList : public ChatbotStep {
public:
    struct Option {
        std::string text;
        std::function<void()> onSelected;
    };

    /**
     * @brief Constructs a new ChatbotStepOptionList.
     * @param prompt The text to display before the options.
     * @param options The list of options to display.
     */
    ChatbotStepOptionList(std::string prompt, std::vector<Option> options);

    [[nodiscard]] std::string draw() override;
    [[nodiscard]] bool isFinished() const override;

private:
    std::string prompt_;
    std::vector<Option> options_;
    bool finished_ = false;
};

} // namespace QaplaWindows::ChatBot
