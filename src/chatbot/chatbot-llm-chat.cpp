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

#include "chatbot-llm-chat.h"
#include "imgui-controls.h"

#include <imgui.h>

namespace QaplaWindows::ChatBot {

using QaplaLlm::LmStudioStatus;

ChatbotLlmChat::ChatbotLlmChat(LmStudioStatus status)
    : status_(status) {
}

void ChatbotLlmChat::start() {
    finished_ = false;
}

bool ChatbotLlmChat::draw() {
    if (finished_) {
        return false;
    }

    switch (status_) {
        case LmStudioStatus::ServerRunning:
            ImGuiControls::textWrapped("LM Studio detected, server is running.");
            break;
        case LmStudioStatus::InstalledServerDown:
            ImGuiControls::textWrapped("LM Studio installed, server is not started.");
            break;
        case LmStudioStatus::NotInstalled:
            // Not reachable: the thread is only registered when LM Studio was found.
            ImGuiControls::textWrapped("LM Studio not detected.");
            break;
    }

    ImGui::Spacing();

    ImGui::BeginDisabled(true);
    static char buffer[1] = "";
    ImGui::InputTextMultiline("##LlmChatInput", buffer, sizeof(buffer), ImVec2(-1, 80));
    ImGui::EndDisabled();
    ImGuiControls::hooverTooltip("Chatting with the local model is not available yet.");

    ImGui::Spacing();

    if (ImGuiControls::textButton("Close")) {
        finished_ = true;
    }
    ImGuiControls::hooverTooltip("Close the AI chat and return to the previous view.");

    return false;
}

bool ChatbotLlmChat::isFinished() const {
    return finished_;
}

std::unique_ptr<ChatbotThread> ChatbotLlmChat::clone() const {
    return std::make_unique<ChatbotLlmChat>(status_);
}

} // namespace QaplaWindows::ChatBot
