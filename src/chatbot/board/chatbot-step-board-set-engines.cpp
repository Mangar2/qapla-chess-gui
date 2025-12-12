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

#include "chatbot-step-board-set-engines.h"
#include "../../interactive-board-window.h"

namespace QaplaWindows::ChatBot {

ChatbotStepBoardSetEngines::ChatbotStepBoardSetEngines(BoardProvider provider)
    : provider_(std::move(provider)) {}

std::string ChatbotStepBoardSetEngines::draw() {
    // This step has no UI - it just activates the engines and finishes
    
    auto* board = provider_();
    if (board == nullptr) {
        // Board no longer exists, stop the thread
        finished_ = true;
        return "stop";
    }
    
    // Activate the selected engines
    board->setActiveEngines();
    
    // Mark as finished immediately
    finished_ = true;
    return "";
}

} // namespace QaplaWindows::ChatBot
