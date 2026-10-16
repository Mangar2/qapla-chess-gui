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

#include "../chatbot-thread.h"
#include "../chatbot-step.h"
#include "../chatbot-snackbar-capture.h"

#include <memory>
#include <vector>

namespace QaplaWindows::ChatBot {

/**
 * @brief A chatbot thread for evaluating every position of a 9 bit book with an engine.
 *
 * The books first, then the engine, the limit of every search, and the start. A run that is
 * already going leaves nothing to set up: the thread opens on it, with the button that stops it.
 */
class ChatbotBookEval : public ChatbotThread {
public:
    [[nodiscard]] std::string getTitle() const override {
        return "Book Evaluation";
    }
    void start() override;
    bool draw() override;
    [[nodiscard]] bool isFinished() const override;
    [[nodiscard]] std::unique_ptr<ChatbotThread> clone() const override;

private:
    std::vector<std::unique_ptr<ChatbotStep>> steps_;
    size_t currentStepIndex_ = 0;
    bool stopped_ = false;
    SnackbarCapture snackbarCapture_{"bookeval"};
};

} // namespace QaplaWindows::ChatBot
