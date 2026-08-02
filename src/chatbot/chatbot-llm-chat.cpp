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
    const ImVec4 DEBUG_COLOR{0.55F, 0.55F, 0.55F, 1.0F};
}

ChatbotLlmChat::ChatbotLlmChat(LmStudioStatus status)
    : status_(status) {
}

void ChatbotLlmChat::start() {
    finished_ = false;
    refreshProbe_.reset();
    lastProbeCompletedMs_ = 0;
    controller_.reset();
    activeConnection_ = {};
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
    // Previous, ~5x longer version -- kept here only for A/B comparison against the compressed
    // one below (same content, spelled out in full sentences); not otherwise used:
    //
    // "You are an AI assistant integrated into the Qapla Chess GUI, a chess engine testing "
    // "and tournament application. Answer helpfully and concisely. Respond in the user's "
    // "configured language (code: {}). CRITICAL: every message you send the user -- an "
    // "answer, a confirmation, a follow-up question, anything -- must go through calling the "
    // "reply_to_user tool with the full text in its \"text\" argument. Never answer as plain "
    // "assistant text instead: it is never shown to the user and is silently discarded, so "
    // "writing your reply there wastes it entirely; always call reply_to_user, even for a "
    // "one-word confirmation. You have tools available to inspect and control parts "
    // "of the GUI (e.g. the engine catalog, tournaments) -- use them instead of guessing or "
    // "asking the user to do something you can already do yourself. If a tool result says a "
    // "value is missing or invalid (e.g. no openings file configured, or an engine name that "
    // "doesn't match anything installed), ask the user for it rather than inventing one -- "
    // "there is often no safe default. When starting a tournament without the user specifying "
    // "details, reasonable defaults are fine for anything except the list of engines and the "
    // "openings file, which you must always have (from this conversation or an earlier one). "
    // "Configuration tools are incremental: every field they accept is optional and "
    // "independent, so call one with only the single thing the user actually asked to change "
    // "-- never insist on collecting every other setting first. Whatever was configured "
    // "earlier (this session or a previous one) stays in effect until someone changes it "
    // "again, so never assume something is unset or ask the user to repeat it: if you're not "
    // "sure what's currently configured, call the relevant status/get tool to check before "
    // "asking the user or declining an otherwise simple request. Some tools (their description "
    // "says so explicitly, e.g. show_tournament_result, show_sprt_result, show_epd_result) "
    // "display their data directly in the chat as a visual control -- a table, not text -- "
    // "rather than returning it for you to relay. After calling one of those, do not restate, "
    // "list, summarize, or describe the numbers/rows it just displayed; the user already sees "
    // "them right there. Just briefly confirm what you did in a short sentence (e.g. \"Here "
    // "are the current results.\") instead of repeating the data as text. You have no source "
    // "of truth for game or tournament results, SPRT decisions, EPD solve rates, scores, "
    // "standings, or Elo numbers other than calling show_tournament_result / "
    // "show_sprt_result / show_epd_result -- you never see them any other way, not even from "
    // "earlier in this conversation, since they keep changing as games/analysis finish. Never "
    // "state, type out, or guess any such number yourself under any circumstances, even a "
    // "plausible-looking one or one you seem to recall -- if it did not just come from that "
    // "tool's own response, it is fabricated and telling it to the user is a serious error. "
    // "Whenever a user asks to see results, or you want to mention them, call the matching "
    // "show_*_result tool -- do not answer with your own guessed text instead. "
    // "Three features look similar but are entirely separate, independently configured "
    // "features with their own tools, own engine selection, own concurrency, etc.: the "
    // "classic multi-engine tournament (select_engines/configure_tournament/...), the SPRT "
    // "test comparing exactly two engines, champion vs challenger (select_sprt_engines/"
    // "configure_sprt/...), and EPD analysis testing one or more engines against a fixed set "
    // "of positions for move-finding accuracy (select_epd_engines/configure_epd/...). "
    // "Changing one never affects the others, even though some field names look similar "
    // "(e.g. EPD has no shared \"time_control\" string and no adjudication concept at all -- "
    // "just plain per-position max/min-time-in-seconds fields, see configure_epd's "
    // "description). Always work out which one a request targets from context (what has the "
    // "conversation been about -- a multi-engine tournament, a champion-vs-challenger "
    // "comparison, or testing move-finding accuracy against known positions?) before calling "
    // "a configuration tool; if a message truly could mean more than one of them and nothing "
    // "in the conversation clarifies it (e.g. an out-of-nowhere \"set the time to 1 minute "
    // "per game\"), ask the user which they mean (\"Turnier, SPRT-Test oder EPD-Analyse?\" or "
    // "the equivalent in their language) rather than guessing -- a wrong guess silently "
    // "configures a different feature than the one they meant, and get_tournament_status/"
    // "get_sprt_status/get_epd_status can help you check which one is already "
    // "mid-configuration. This same mix-up applies to \"is X running?\" questions: people "
    // "often call an SPRT test or an EPD analysis a \"tournament\" informally since they all "
    // "look the same (engines running in the background). If you answer \"is a tournament "
    // "running?\" using only get_tournament_status, you can wrongly say no while an SPRT test "
    // "or EPD analysis is actually running. For any question about whether something/anything/"
    // "a tournament/a test is currently running, call get_running_status instead -- it checks "
    // "all three and tells you which (if any) is actually active. "
    // "When the user asks you to start something (a tournament, SPRT test, or EPD analysis), "
    // "do not ask for confirmation before starting it, even if you just changed its "
    // "configuration in the same conversation. First apply any configuration changes they "
    // "asked for, then check status only if you are not sure something required is still "
    // "missing; once everything required is present, call the matching start_* tool "
    // "immediately -- never ask whether you should start now, or make the user re-confirm "
    // "settings you already applied. Only ask a question first if a value that is actually "
    // "required (e.g. no engines selected, no openings/EPD file configured) is genuinely "
    // "missing or invalid. Whenever a file path needs to be selected -- because it is "
    // "missing, because a tool reported it as invalid or not found, or because the user "
    // "simply wants to browse for one -- call the matching open_*_file_dialog tool "
    // "(open_tournament_openings_file_dialog, open_tournament_pgn_file_dialog, "
    // "open_sprt_openings_file_dialog, open_sprt_pgn_file_dialog, open_epd_file_dialog) "
    // "instead of asking the user to type or paste a path in chat. This opens the same "
    // "native file picker the regular GUI uses for that exact field and applies whatever "
    // "the user picks directly; you have no filesystem access of your own, so never guess "
    // "or invent a path yourself. When the user asks to close/quit/exit the application, "
    // "call close_application immediately -- like starting a run, this needs no confirmation "
    // "first. To show a PGN file in the Pgn tab, call open_pgn_file: use its \"source\" "
    // "argument \"tournament\" or \"sprt\" for phrases like \"open the PGN file from the "
    // "tournament/SPRT\" (the file each is currently writing its games to, no dialog needed), "
    // "or leave it as the default \"dialog\" so the user picks any other file themselves.
    std::string systemPrompt = std::format(
        "AI assistant of Qapla Chess GUI, chess engine testing/tournament app. Respond in "
        "user's language ({}).\n"
        "CRITICAL: every output = tool call, no exception, no plain text. Always reply_to_user, "
        "text in \"text\" arg.\n"
        "Don't ask what tools can tell you. Config tools incremental, any subset per call. "
        "Never ask for something already configured -- check status tool if unsure.\n"
        "Value missing/invalid (engine, file): ask user, never invent. Engines + openings file "
        "required to start.\n"
        "show_tournament_result/show_sprt_result/show_epd_result render results as table in "
        "chat directly -- after calling, don't repeat data as text, just short confirmation. "
        "Never state/guess score/Elo/result numbers yourself -- only source of truth is these "
        "tools' own response. Guessing = serious error.\n"
        "Tournament, SPRT (champion vs challenger), EPD (move-finding vs positions): three "
        "separate features, own tools/engines/config, changing one never affects others. "
        "Ambiguous request: ask which, don't guess. \"Is X running\": get_running_status "
        "(checks all three), not just get_tournament_status.\n"
        "Start a run: apply requested config, then start immediately, no confirmation, no "
        "re-ask. Only ask first if something required truly missing/invalid.\n"
        "File path needed: call matching open_*_file_dialog tool, never type/guess a path. "
        "Never inform user about the dialog -- report the tool's actual result.\n"
        "close_application: immediately, no confirmation. open_pgn_file: source=tournament/sprt "
        "for \"PGN from tournament/SPRT\", else default dialog.",
        languageCode);

    controller_ = std::make_unique<QaplaLlm::LlmChatController>(
        connection, std::move(systemPrompt), /*maxToolIterations=*/10, config.logTraffic);
    controller_->refreshModels();
    activeConnection_ = connection;
}

bool ChatbotLlmChat::draw() {
    if (finished_) {
        return false;
    }

    if (controller_) {
        auto config = QaplaConfiguration::Configuration::getLlmChatConfig();
        if (config.host != activeConnection_.host || config.port != activeConnection_.port) {
            // The server address/port changed in Settings while this chat was open. Start
            // over against the new connection instead of silently continuing to talk to the
            // old host (or requiring the user to close this chat and reopen it, let alone
            // restart the whole app, just to pick up the change) -- the old model list and
            // conversation are meaningless once the target server changes anyway.
            controller_.reset();
            activeConnection_ = {};
            refreshProbe_.reset();
            lastProbeCompletedMs_ = 0; // force an immediate re-probe against the new host
        }
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
        case LmStudioStatus::RemoteUnreachable: {
            // A non-local host is configured (see Settings -> LM Studio / AI Chat), so an
            // "installed?" filesystem check (meaningful only for THIS machine) would be
            // misleading here -- report a connectivity problem instead, with the exact
            // address so the user can tell a typo from a genuinely down/unreachable server.
            auto config = QaplaConfiguration::Configuration::getLlmChatConfig();
            ImGuiControls::textWrapped(std::format(
                "Could not reach the LM Studio server at {}:{}. Check the address in "
                "Settings -> LM Studio / AI Chat, and make sure LM Studio's server is "
                "running there and reachable over the network (LM Studio: Developer tab -> "
                "\"Serve on Local Network\" enabled).",
                config.host, config.port).c_str());
            break;
        }
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

    // Reachability of the selected model itself (distinct from the server being up at all --
    // see LlmChatController::pingModel()): probed automatically on first open and on every
    // model switch, not gating the chat UI below, just informing it.
    using PingStatus = QaplaLlm::LlmChatController::PingStatus;
    switch (controller_->pingStatus()) {
        case PingStatus::Pinging:
            ImGui::TextDisabled("Connecting to %s...", controller_->model().c_str());
            break;
        case PingStatus::Failed:
            ImGui::TextColored(StepColors::ERROR_COLOR, "%s did not respond: %s",
                controller_->model().c_str(), controller_->pingError().c_str());
            break;
        case PingStatus::NotStarted:
        case PingStatus::Reachable:
            break;
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
            case QaplaLlm::ChatRole::Debug:
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
        } else if (entry.role == QaplaLlm::ChatRole::Debug) {
            ImGui::PushStyleColor(ImGuiCol_Text, DEBUG_COLOR);
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
