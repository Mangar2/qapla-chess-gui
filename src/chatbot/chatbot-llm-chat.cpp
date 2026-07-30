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
        "configured language (code: {}). You have tools available to inspect and control parts "
        "of the GUI (e.g. the engine catalog, tournaments) -- use them instead of guessing or "
        "asking the user to do something you can already do yourself. If a tool result says a "
        "value is missing or invalid (e.g. no openings file configured, or an engine name that "
        "doesn't match anything installed), ask the user for it rather than inventing one -- "
        "there is often no safe default. When starting a tournament without the user specifying "
        "details, reasonable defaults are fine for anything except the list of engines and the "
        "openings file, which you must always have (from this conversation or an earlier one). "
        "Configuration tools are incremental: every field they accept is optional and "
        "independent, so call one with only the single thing the user actually asked to change "
        "-- never insist on collecting every other setting first. Whatever was configured "
        "earlier (this session or a previous one) stays in effect until someone changes it "
        "again, so never assume something is unset or ask the user to repeat it: if you're not "
        "sure what's currently configured, call the relevant status/get tool to check before "
        "asking the user or declining an otherwise simple request. Some tools (their description "
        "says so explicitly, e.g. show_tournament_result, show_sprt_result) display their data "
        "directly in the chat as a visual control -- a table, not text -- rather than returning "
        "it for you to relay. After calling one of those, do not restate, list, summarize, or "
        "describe the numbers/rows it just displayed; the user already sees them right there. "
        "Just briefly confirm what you did in a short sentence (e.g. \"Here are the current "
        "results.\") instead of repeating the data as text. You have no source of truth for game "
        "or tournament results, SPRT decisions, scores, standings, or Elo numbers other than "
        "calling show_tournament_result / show_sprt_result -- you never see them any other way, "
        "not even from earlier in this conversation, since they keep changing as games finish. "
        "Never state, type out, or guess any such number yourself under any circumstances, even "
        "a plausible-looking one or one you seem to recall -- if it did not just come from that "
        "tool's own response, it is fabricated and telling it to the user is a serious error. "
        "Whenever a user asks to see results, or you want to mention them, call the matching "
        "show_*_result tool -- do not answer with your own guessed text instead. "
        "Two features look similar but are entirely separate, independently configured "
        "features with their own tools, own engine selection, own time control, own "
        "concurrency, own openings file, own adjudication settings, etc.: the classic "
        "multi-engine tournament (select_engines/configure_tournament/...) and the SPRT test "
        "comparing exactly two engines, champion vs challenger (select_sprt_engines/"
        "configure_sprt/...). Changing one never affects the other, even though the field names "
        "look the same. Always work out which one a request targets from context (what has the "
        "conversation been about -- a multi-engine tournament, or a champion-vs-challenger "
        "comparison?) before calling a configuration tool; if a message truly could mean either "
        "and nothing in the conversation clarifies it (e.g. an out-of-nowhere \"set the time "
        "control to 1 minute per game\"), ask the user \"Turnier oder SPRT-Test?\" (or the "
        "equivalent in their language) rather than guessing -- a wrong guess silently configures "
        "the other feature instead of the one they meant, and get_tournament_status/"
        "get_sprt_status can help you check which one is already mid-configuration. This same "
        "mix-up applies to \"is X running?\" questions: people often call an SPRT test a "
        "\"tournament\" informally since it looks the same (engines playing games in the "
        "background). If you answer \"is a tournament running?\" using only "
        "get_tournament_status, you can wrongly say no while an SPRT test is actually running. "
        "For any question about whether something/anything/a tournament/a test is currently "
        "running, call get_running_status instead -- it checks both and tells you which (if "
        "either) is actually active.",
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
        ImGui::SameLine();
    } else {
        drawStatusOnly();
        ImGui::Spacing();
    }

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
        if (entry.role == QaplaLlm::ChatRole::Tool && entry.renderWidget) {
            // A real GUI control (e.g. the same ImGuiTable a classic chatbot step would draw)
            // rather than a text dump -- see GuiToolResult::renderWidget.
            entry.renderWidget();
        } else if (entry.role == QaplaLlm::ChatRole::Tool) {
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
