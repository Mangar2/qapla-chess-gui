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
 * @author GitHub Copilot
 * @copyright Copyright (c) 2025 GitHub Copilot
 */

#include "chatbot-step-finish.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepFinish::ChatbotStepFinish(std::string message)
    : message_(std::move(message)) {
}

std::string ChatbotStepFinish::draw() {
    ImGuiControls::textWrapped(message_);
    ImGui::Spacing();

    if (QaplaWindows::ImGuiControls::textButton("Finish")) {
        finished_ = true;
    }
    return "";
}

} // namespace QaplaWindows::ChatBot
