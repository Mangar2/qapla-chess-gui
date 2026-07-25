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
#include "chatbot-step.h"
#include "imgui-controls.h"
#include "../configuration.h"
#include "../i18n.h"

#include <base-elements/timer.h>

#include <imgui.h>

#include <format>

namespace QaplaWindows::ChatBot {

using QaplaLlm::LmStudioStatus;

namespace {
    // How often the status may be re-probed while the step is open. Chosen to
    // notice a manually started/stopped LM Studio server within a few
    // seconds without hammering it with requests every frame.
    constexpr uint64_t REFRESH_INTERVAL_MS = 3000;

    const ImVec4 USER_COLOR{0.6F, 0.8F, 1.0F, 1.0F};
    const ImVec4 TOOL_COLOR{0.85F, 0.75F, 0.4F, 1.0F};
    const ImVec4 ASSISTANT_COLOR{0.7F, 1.0F, 0.7F, 1.0F};
}

ChatbotLlmChat::ChatbotLlmChat(LmStudioStatus status)
    : status_(status) {
}

void ChatbotLlmChat::start() {
    finished_ = false;
    refreshProbe_.reset();
    lastProbeCompletedMs_ = 0;
    controller_.reset();
    inputBuffer_.fill('\0');
    lastHistorySize_ = 0;
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

void ChatbotLlmChat::ensureController() {
    if (controller_) {
        return;
    }

    auto config = QaplaConfiguration::Configuration::getLlmChatConfig();
    QaplaLlm::LmStudioConnection connection;
    connection.host = config.host;
    connection.port = config.port;

    auto languageCode = Translator::instance().getLanguageCode();
    std::string systemPrompt = std::format(
        "You are an AI assistant integrated into the Qapla Chess GUI, a chess engine testing "
        "and tournament application. Answer helpfully and concisely. Respond in the user's "
        "configured language (code: {}). You currently have no tools available -- you can only "
        "chat; you cannot yet control the GUI.",
        languageCode);

    controller_ = std::make_unique<QaplaLlm::LlmChatController>(connection, std::move(systemPrompt));
    controller_->refreshModels();
}

bool ChatbotLlmChat::draw() {
    if (finished_) {
        return false;
    }

    refreshStatus();

    if (!controller_ && status_ == LmStudioStatus::ServerRunning) {
        ensureController();
    }

    if (controller_) {
        controller_->update();
        drawChatUi();
    } else {
        drawStatusOnly();
    }

    ImGui::Spacing();

    if (ImGuiControls::textButton("Close")) {
        finished_ = true;
    }
    ImGuiControls::hooverTooltip("Close the AI chat and return to the previous view.");

    return false;
}

void ChatbotLlmChat::drawStatusOnly() {
    switch (status_) {
        case LmStudioStatus::ServerRunning:
            // Not reachable in practice: draw() creates the controller as
            // soon as status_ becomes ServerRunning, before this is called.
            ImGuiControls::textWrapped("LM Studio detected, server is running.");
            break;
        case LmStudioStatus::InstalledServerDown:
            ImGuiControls::textWrapped("LM Studio installed, server is not started.");
            break;
        case LmStudioStatus::NotInstalled:
            ImGuiControls::textWrapped("LM Studio not detected.");
            break;
    }
}

void ChatbotLlmChat::drawChatUi() {
    // Model dropdown
    const auto& models = controller_->availableModels();
    std::string preview = controller_->model().empty() ? "(no model)" : controller_->model();
    ImGui::SetNextItemWidth(300.0F);
    if (ImGui::BeginCombo("##LlmModelSelect", preview.c_str())) {
        for (const auto& modelId : models) {
            bool isSelected = (modelId == controller_->model());
            if (ImGui::Selectable(modelId.c_str(), isSelected)) {
                controller_->setModel(modelId);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGuiControls::textButton("Refresh Models")) {
        controller_->refreshModels();
    }
    if (controller_->isRefreshingModels()) {
        ImGui::SameLine();
        ImGui::TextDisabled("(refreshing...)");
    } else if (!controller_->modelsError().empty()) {
        ImGui::SameLine();
        ImGui::TextColored(StepColors::ERROR_COLOR, "(%s)", controller_->modelsError().c_str());
    }

    ImGui::Spacing();

    // History
    const auto& history = controller_->history();
    ImGui::BeginChild("##LlmChatHistory", ImVec2(-1, 220), true);
    for (const auto& entry : history) {
        // Tool entries are already phrased as a plain status sentence (see
        // gui-tool-*-register.cpp) and shown without a header: end users
        // don't know what a "tool" or "open_add_engine_dialog" is, so no
        // label -- just the friendly text, in a distinguishing color.
        switch (entry.role) {
            case QaplaLlm::ChatRole::User:
                ImGui::TextColored(USER_COLOR, "You:");
                break;
            case QaplaLlm::ChatRole::Assistant:
                ImGui::TextColored(ASSISTANT_COLOR, "Assistant:");
                break;
            case QaplaLlm::ChatRole::Tool:
                break;
            case QaplaLlm::ChatRole::Error:
                ImGui::TextColored(StepColors::ERROR_COLOR, "Error:");
                break;
        }
        if (entry.role == QaplaLlm::ChatRole::Tool) {
            ImGui::PushStyleColor(ImGuiCol_Text, TOOL_COLOR);
            ImGui::TextWrapped("%s", entry.text.c_str());
            ImGui::PopStyleColor();
        } else {
            ImGui::TextWrapped("%s", entry.text.c_str());
        }
        ImGui::Spacing();
    }
    if (controller_->isBusy()) {
        ImGui::TextDisabled("Thinking...");
    }
    if (history.size() != lastHistorySize_) {
        ImGui::SetScrollHereY(1.0F);
        lastHistorySize_ = history.size();
    }
    ImGui::EndChild();

    ImGui::Spacing();

    // Input
    const bool busy = controller_->isBusy();
    ImGui::BeginDisabled(busy);
    constexpr ImGuiInputTextFlags inputFlags =
        ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_EnterReturnsTrue;
    bool submitted = ImGui::InputTextMultiline(
        "##LlmChatInput", inputBuffer_.data(), inputBuffer_.size(), ImVec2(-1, 80), inputFlags);
    ImGui::EndDisabled();
    ImGuiControls::hooverTooltip("Type your message. Enter sends, Ctrl+Enter inserts a newline.");

    if (submitted) {
        trySend();
    }

    if (busy) {
        if (ImGuiControls::textButton("Stop")) {
            controller_->stop();
        }
    } else {
        if (ImGuiControls::textButton("Send")) {
            trySend();
        }
    }
}

void ChatbotLlmChat::trySend() {
    if (!controller_ || controller_->isBusy()) {
        return;
    }

    std::string text(inputBuffer_.data());
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
        text.pop_back();
    }
    if (text.empty()) {
        return;
    }

    controller_->sendMessage(text);
    inputBuffer_.fill('\0');
}

bool ChatbotLlmChat::isFinished() const {
    return finished_;
}

std::unique_ptr<ChatbotThread> ChatbotLlmChat::clone() const {
    return std::make_unique<ChatbotLlmChat>(status_);
}

} // namespace QaplaWindows::ChatBot
