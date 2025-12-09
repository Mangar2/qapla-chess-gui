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
 * @brief Chatbot step to display standard tournament results.
 * 
 * Shows tournament ELO table directly in the chatbot window and provides
 * a button to view detailed HTML results in the default browser.
 */
class ChatbotStepStandardTournamentResult : public ChatbotStep {
public:
    /**
     * @brief Constructor for standard tournament result display step.
     * @param title Title for the HTML report (default: "Tournament Results").
     */
    explicit ChatbotStepStandardTournamentResult(std::string title = "Tournament Results");

    [[nodiscard]] std::string draw() override;

private:
    std::string title_;
    std::string htmlPath_;

    /**
     * @brief Generates HTML report and saves it to config directory.
     * @return Path to the generated HTML file.
     */
    std::string generateHtmlReport();
};

} // namespace QaplaWindows::ChatBot
