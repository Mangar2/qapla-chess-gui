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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#pragma once

#include "../chatbot-step.h"
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief Step that names the book to evaluate and the evaluated book to write.
 *
 * An evaluated book that exists already is the state of an earlier run, and the run continues it.
 */
class ChatbotStepBookEvalFiles : public ChatbotStep {
public:
    ChatbotStepBookEvalFiles() = default;
    ~ChatbotStepBookEvalFiles() override = default;

    [[nodiscard]] std::string draw() override;

private:
    std::string summary_;
};

} // namespace QaplaWindows::ChatBot
