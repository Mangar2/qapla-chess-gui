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

#include "chatbot-remote-control.h"
#include "../clop-data.h"

#include "chatbot-step.h"
#include "chatbot-window.h"
#include "imgui-controls.h"
#include "snackbar.h"

#include <imgui.h>

#include <format>

namespace QaplaWindows::ChatBot {

namespace {
    const ImVec4 CALL_COLOR{0.6F, 0.8F, 1.0F, 1.0F};
    const ImVec4 RESULT_COLOR{0.85F, 0.75F, 0.4F, 1.0F};
} // namespace

void ChatbotRemoteControl::start() {
    finished_ = false;
}

void ChatbotRemoteControl::drawEntry(const QaplaLlm::RemoteCallEntry& entry) {
    // The heading names the tool and its arguments verbatim -- unlike the AI chat, which hides
    // both because an end user does not know what "configure_sprt" is. Here that is precisely
    // what the user is watching for: this window exists to make an outside caller's actions
    // followable, and a paraphrase would defeat it.
    ImGui::TextColored(CALL_COLOR, "%s  %s", entry.time.c_str(), entry.toolName.c_str());
    if (!entry.arguments.empty() && entry.arguments != "{}") {
        ImGui::PushStyleColor(ImGuiCol_Text, CALL_COLOR);
        ImGui::TextWrapped("%s", entry.arguments.c_str());
        ImGui::PopStyleColor();
    }

    if (entry.renderWidget) {
        // The live control the tool built, redrawn every frame from the GUI's current state --
        // so a results table shown here keeps counting up while the run continues. See
        // GuiToolResult::renderWidget for the threading contract this relies on.
        entry.renderWidget();
        return;
    }
    ImGui::PushStyleColor(
        ImGuiCol_Text, entry.success ? RESULT_COLOR : StepColors::ERROR_COLOR);
    ImGui::TextWrapped("%s", entry.content.c_str());
    ImGui::PopStyleColor();
}

bool ChatbotRemoteControl::draw() {
    auto& server = QaplaLlm::RemoteControlServer::instance();

    if (server.isRunning()) {
        ImGui::TextColored(StepColors::SUCCESS_COLOR, "%s",
            std::format("Remote control is listening on 127.0.0.1:{}.", server.port()).c_str());
    } else {
        ImGuiControls::textDisabled("Remote control has ended. The application is yours again.");
    }
    ImGui::TextWrapped("%s",
        "Everything an outside caller does appears below, and in the rest of the application as "
        "it happens -- games on the boards, results in the tournament and SPRT views. You can "
        "keep using all of it yourself while this runs.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // The one activity with nothing of its own to look at: CLOP has no tab, so its live numbers
    // belong here, above the call log, where they stay put instead of scrolling away with
    // whatever call happened to ask for them last. Redrawn every frame, which is what "the CLI
    // prints this every n samples" turns into when the reader is a person watching a window.
    auto& clop = QaplaWindows::ClopData::instance();
    if (clop.hasTables()) {
        ImGui::TextColored(StepColors::SUCCESS_COLOR, "%s", "CLOP tuning");
        clop.drawTables();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    auto entries = server.entries();
    if (entries.empty()) {
        ImGuiControls::textDisabled("Nothing has been called yet.");
    }
    for (const auto& entry : entries) {
        drawEntry(entry);
        ImGui::Spacing();
    }

    if (!server.isRunning()) {
        return true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGuiControls::textButton("End remote control")) {
        // Closes the channel and nothing else -- a run that is playing keeps playing, see
        // RemoteControlServer::stop(). Clearing the exclusive thread first means that when this
        // one reports itself finished a moment later, the ChatbotWindow has the full menu back.
        server.stop();
        ChatbotWindow::instance()->clearExclusiveThread();
        SnackbarManager::instance().showNote(
            "Remote control ended. Anything already running was left running.", false,
            "remote-control");
        finished_ = true;
    }
    ImGuiControls::hooverTooltip(
        "Stops accepting calls from outside. Does not stop a running tournament, SPRT test or "
        "EPD analysis.");
    return true;
}

bool ChatbotRemoteControl::isFinished() const {
    return finished_;
}

std::unique_ptr<ChatbotThread> ChatbotRemoteControl::clone() const {
    return std::make_unique<ChatbotRemoteControl>();
}

bool startRemoteControl(const QaplaLlm::RemoteControlOptions& options) {
    if (!QaplaLlm::RemoteControlServer::instance().start(options)) {
        SnackbarManager::instance().showError(
            std::format("Could not start the remote control on port {} -- the port is in use. "
                        "The application works normally, it just cannot be driven from outside.",
                options.port),
            false, "remote-control");
        return false;
    }
    ChatbotWindow::instance()->setExclusiveThread(std::make_unique<ChatbotRemoteControl>());
    return true;
}

} // namespace QaplaWindows::ChatBot
