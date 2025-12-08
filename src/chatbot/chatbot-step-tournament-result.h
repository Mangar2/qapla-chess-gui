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
#include <string>


namespace QaplaTester { class TournamentResult; }

namespace QaplaWindows::ChatBot {

/**
 * @brief Reusable chatbot step to display tournament results.
 * 
 * Shows a text summary of tournament results in the chatbot and provides
 * a button to view detailed HTML results in the default browser.
 */
class ChatbotStepTournamentResult : public ChatbotStep {
public:
    /**
     * @brief Constructor for tournament result display step.
     * @param context The engine select context (Standard or SPRT).
     * @param title Title for the HTML report (default: "Tournament Results").
     */
    explicit ChatbotStepTournamentResult(
        EngineSelectContext context = EngineSelectContext::Standard,
        std::string title = "Tournament Results");

    [[nodiscard]] std::string draw() override;

private:
    EngineSelectContext context_;
    std::string title_;
    bool htmlGenerated_ = false;
    std::string htmlPath_;

    /**
     * @brief Gets the tournament result based on context.
     * @return The current tournament result.
     */
    QaplaTester::TournamentResult getTournamentResult() const;

    /**
     * @brief Generates HTML report and saves it to config directory.
     * @return Path to the generated HTML file.
     */
    std::string generateHtmlReport();

    /**
     * @brief Formats a plain text summary of the results.
     * @return Plain text summary string.
     */
    std::string formatTextSummary() const;
};

} // namespace QaplaWindows::ChatBot
