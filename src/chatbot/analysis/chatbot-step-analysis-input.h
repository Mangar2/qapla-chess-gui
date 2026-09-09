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

#include <cstdint>
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief Step that picks the PGN file to analyse and loads it into the Pgn view.
 *
 * Loaded into the view rather than read privately: the games that are about to be analysed are
 * then in front of the user, in the table they already know, and the next step filters exactly
 * what is shown there.
 */
class ChatbotStepAnalysisInput : public ChatbotStep {
public:
    ChatbotStepAnalysisInput() = default;
    ~ChatbotStepAnalysisInput() override = default;

    [[nodiscard]] std::string draw() override;

private:
    /** @brief What the step is doing: choosing a file, or waiting for one to be read. */
    enum class State : std::uint8_t {
        Choosing,
        Loading
    };

    /** @brief Draws the file selection and starts the loading when it is confirmed. */
    [[nodiscard]] std::string drawChoosing();

    /** @brief Waits for the Pgn view to finish reading, and reports what it read. */
    void drawLoading();

    State state_ = State::Choosing;
    std::string summary_;
};

} // namespace QaplaWindows::ChatBot
