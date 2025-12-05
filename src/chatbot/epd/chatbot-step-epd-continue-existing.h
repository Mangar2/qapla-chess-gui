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

#include "../chatbot-step.h"
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to ask the user if they want to continue an existing EPD analysis.
 * 
 * This step is only active if there are incomplete results from a previous analysis.
 * If no incomplete analysis exists, this step finishes automatically and proceeds to the menu.
 */
class ChatbotStepEpdContinueExisting : public ChatbotStep {
public:
    ChatbotStepEpdContinueExisting() = default;

    [[nodiscard]] std::string draw() override;

private:
    std::string finishedMessage_;

    /**
     * @brief Checks if there is an existing analysis that can be continued.
     * @return true if there are incomplete results, false otherwise.
     */
    [[nodiscard]] bool hasIncompleteAnalysis() const;
};

} // namespace QaplaWindows::ChatBot
