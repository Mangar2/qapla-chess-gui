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
#include "opening-parser.h"

#include <optional>

namespace QaplaWindows {
    class ImGuiTournamentOpening;
}

namespace QaplaWindows::ChatBot {

/**
 * @brief Chatbot step for configuring tournament opening settings.
 * Supports both standard tournaments and SPRT tournaments.
 */
class ChatbotStepTournamentOpening : public ChatbotStep {
public:
    explicit ChatbotStepTournamentOpening(EngineSelectContext type = EngineSelectContext::Standard);
    ~ChatbotStepTournamentOpening() override = default;

    /**
     * @brief Draws the opening configuration UI.
     * @return "stop" if cancelled, empty string otherwise.
     */
    [[nodiscard]] std::string draw() override;

private:
    EngineSelectContext type_;

    /**
     * @brief Gets the tournament opening configuration.
     * @return Reference to the tournament opening.
     */
    [[nodiscard]] ImGuiTournamentOpening& getTournamentOpening();
    [[nodiscard]] const ImGuiTournamentOpening& getTournamentOpening() const;

    /**
     * @brief Draws status messages based on opening file validation.
     */
    void drawStatusMessage();

    /**
     * @brief Draws the validation result and trace.
     */
    void drawValidationResult();

    /**
     * @brief Draws the action buttons.
     * @return "stop" if cancelled, empty string otherwise.
     */
    std::string drawButtons();

    /**
     * @brief Checks if the opening file path exists.
     * @return True if file exists, false otherwise.
     */
    [[nodiscard]] bool doesOpeningFileExist() const;

    /**
     * @brief Validates the opening file by parsing it.
     */
    void validateOpeningFile();

    /**
     * @brief Formats the trace for display.
     * @return Formatted trace string.
     */
    [[nodiscard]] std::string formatTrace() const;

    bool showMoreOptions_ = false;                          ///< Show advanced options
    bool showTrace_ = false;                                ///< Show parser trace
    bool isValidated_ = false;                              ///< True if file was successfully validated
    std::optional<QaplaTester::OpeningParserResult> parseResult_;
    std::string lastFilename_;                              ///< Last checked file path (valid or not)
};

} // namespace QaplaWindows::ChatBot
