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
#include "../configuration.h"

#include <base-elements/timer.h>

#include <imgui.h>

namespace QaplaWindows::ChatBot {

using QaplaLlm::LmStudioStatus;

namespace {
    // How often the status may be re-probed while the step is open. Chosen to
    // notice a manually started/stopped LM Studio server within a few
    // seconds without hammering it with requests every frame.
    constexpr uint64_t REFRESH_INTERVAL_MS = 3000;
}

ChatbotLlmChat::ChatbotLlmChat(LmStudioStatus status)
    : status_(status) {
}

void ChatbotLlmChat::start() {
    finished_ = false;
    refreshProbe_.reset();
    lastProbeCompletedMs_ = 0;
}

void ChatbotLlmChat::refreshStatus() {
    if (refreshProbe_.has_value()) {
        if (!refreshProbe_->isReady()) {
            return; // previous probe still in flight; keep showing status_ as-is
        }
        status_ = refreshProbe_->status();
        lastProbeCompletedMs_ = QaplaHelpers::Timer::getCurrentTimeMs();
        refreshProbe_.reset();
        return;
    }

    if (lastProbeCompletedMs_ != 0 &&
        QaplaHelpers::Timer::getCurrentTimeMs() - lastProbeCompletedMs_ < REFRESH_INTERVAL_MS) {
        return;
    }

    auto config = QaplaConfiguration::Configuration::getLlmChatConfig();
    QaplaLlm::LmStudioProbeConfig probeConfig;
    probeConfig.host = config.host;
    probeConfig.port = config.port;

    refreshProbe_.emplace(probeConfig);
    refreshProbe_->start();
}

bool ChatbotLlmChat::draw() {
    if (finished_) {
        return false;
    }

    refreshStatus();

    switch (status_) {
        case LmStudioStatus::ServerRunning:
            ImGuiControls::textWrapped("LM Studio detected, server is running.");
            break;
        case LmStudioStatus::InstalledServerDown:
            ImGuiControls::textWrapped("LM Studio installed, server is not started.");
            break;
        case LmStudioStatus::NotInstalled:
            // Only reachable if a live re-probe stops finding LM Studio after
            // the thread was registered (e.g. uninstalled while GUI is open).
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
