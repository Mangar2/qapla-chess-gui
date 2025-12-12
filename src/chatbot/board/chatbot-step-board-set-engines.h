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
#include <functional>
#include <cstdint>

namespace QaplaWindows {
    class InteractiveBoardWindow;
}

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to activate the selected engines on an interactive board.
 * This step has no UI, it just calls setActiveEngines() and finishes immediately.
 * This is only needed for interactive boards, not for tournaments/SPRT/EPD.
 */
class ChatbotStepBoardSetEngines : public ChatbotStep {
public:
    /**
     * @brief Callback function type that returns a pointer to the board.
     * The callback may return nullptr if the board no longer exists.
     */
    using BoardProvider = std::function<InteractiveBoardWindow*()>;

    /**
     * @brief Constructs with a board provider callback.
     */
    explicit ChatbotStepBoardSetEngines(BoardProvider provider);
    ~ChatbotStepBoardSetEngines() override = default;

    [[nodiscard]] std::string draw() override;

private:
    BoardProvider provider_;  ///< Callback to retrieve the board
};

} // namespace QaplaWindows::ChatBot
