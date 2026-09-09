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
 * @brief Step that picks the PGN file the analysed games are written to.
 *
 * A run writes one copy of every game per engine, so the file is never the one being read: an
 * analysis appending into its own input would grow it while it walks through it.
 */
class ChatbotStepAnalysisOutput : public ChatbotStep {
public:
    ChatbotStepAnalysisOutput() = default;
    ~ChatbotStepAnalysisOutput() override = default;

    [[nodiscard]] std::string draw() override;

private:
    /**
     * @brief What is known about the chosen path.
     */
    struct ValidationResult {
        bool isValidPath = false;    ///< The path is usable and its directory exists
        bool fileExists = false;     ///< The file is already there
        bool willOverwrite = false;  ///< It is there and append mode is off
        bool isInputFile = false;    ///< It is the file the games are read from
    };

    /** @brief Looks at the chosen path and the append mode. */
    [[nodiscard]] static ValidationResult validate(const std::string& filePath, bool appendMode);

    /** @brief Says what the validation found, if there is anything to say. */
    static void drawStatusMessage(const ValidationResult& validation);

    bool showMoreOptions_ = false;
    std::string summary_;
};

} // namespace QaplaWindows::ChatBot
