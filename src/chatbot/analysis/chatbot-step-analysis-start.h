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
 * @brief Step that sets the concurrency, starts the analysis, and watches it run.
 *
 * The games are taken from the Pgn view here and not before: from this moment the analysis has
 * its own copy of them and the view is free again, so the file the analysis is writing can be
 * loaded into it and looked at while the run goes on.
 */
class ChatbotStepAnalysisStart : public ChatbotStep {
public:
    ChatbotStepAnalysisStart() = default;
    ~ChatbotStepAnalysisStart() override = default;

    [[nodiscard]] std::string draw() override;

private:
    /** @brief Draws the concurrency and the start button. */
    [[nodiscard]] std::string drawBeforeStart();

    /** @brief Draws how far the run has come, and how to stop it. */
    void drawWhileRunning();

    bool started_ = false;
};

} // namespace QaplaWindows::ChatBot
