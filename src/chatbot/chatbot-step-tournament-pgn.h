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
#include "chatbot-step-tournament-stop-running.h"

namespace QaplaWindows {
    class ImGuiTournamentPgn;
}

namespace QaplaWindows::ChatBot {

/**
 * @brief Chatbot step for selecting the PGN file for tournament results.
 * Supports both standard tournaments and SPRT tournaments.
 */
class ChatbotStepTournamentPgn : public ChatbotStep {
public:
    explicit ChatbotStepTournamentPgn(TournamentType type = TournamentType::Standard);
    ~ChatbotStepTournamentPgn() override = default;

    /**
     * @brief Draws the PGN file selection UI.
     * @return "stop" if cancelled, empty string otherwise.
     */
    [[nodiscard]] std::string draw() override;

private:
    TournamentType type_;

    /**
     * @brief Gets the tournament PGN configuration.
     * @return Reference to the tournament PGN.
     */
    [[nodiscard]] ImGuiTournamentPgn& getTournamentPgn();

    /**
     * @brief Result of file path validation.
     */
    struct ValidationResult {
        bool isValidPath = false;   ///< True if path is syntactically valid and parent exists
        bool fileExists = false;    ///< True if the file already exists
        bool willOverwrite = false; ///< True if file exists and append mode is off
    };

    /**
     * @brief Validates the PGN file path.
     * @param filePath The file path to validate.
     * @param appendMode Whether append mode is enabled.
     * @return Validation result with path status.
     */
    [[nodiscard]] static ValidationResult validateFilePath(const std::string& filePath, bool appendMode);

    /**
     * @brief Draws status messages based on validation result.
     * @param validation The validation result to display.
     */
    static void drawStatusMessage(const ValidationResult& validation);

    /**
     * @brief Draws the action buttons (Continue/Cancel).
     * @param validation The validation result for button state.
     * @return "stop" if cancelled, empty string otherwise.
     */
    std::string drawButtons(const ValidationResult& validation);
};

} // namespace QaplaWindows::ChatBot
