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
 * @brief Step that narrows the loaded games down to the ones worth analysing.
 *
 * The dialog it opens is the Pgn view's own -- the same one its Filter button opens, reached
 * from a second place. Whatever it lets through is what the table shows and what the analysis
 * will be given.
 */
class ChatbotStepAnalysisFilter : public ChatbotStep {
public:
    ChatbotStepAnalysisFilter() = default;
    ~ChatbotStepAnalysisFilter() override = default;

    [[nodiscard]] std::string draw() override;

private:
    std::string summary_;
};

} // namespace QaplaWindows::ChatBot
