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

#include "chatbot-window.h"
#include "imgui-controls.h"
#include "snackbar.h"

#include <imgui.h>

#include <format>
#include <utility>

namespace QaplaWindows::ChatBot {

namespace {
    const ImVec4 CALL_COLOR{0.6F, 0.8F, 1.0F, 1.0F};
    const ImVec4 RESULT_COLOR{0.85F, 0.75F, 0.4F, 1.0F};
} // namespace

ChatbotStepRemoteCall::ChatbotStepRemoteCall(RemoteCallEntry entry) : entry_(std::move(entry)) {
    // Nothing to wait for: the call already happened, this only shows it.
    finished_ = true;
}

std::string ChatbotStepRemoteCall::draw() {
    // The heading names the tool and its arguments verbatim -- unlike the AI chat, which hides
    // both because an end user does not know what "configure_sprt" is. Here that is precisely
    // what the user is watching for: this window exists to make an outside caller's actions
    // followable, and a paraphrase would defeat it.
    ImGui::TextColored(CALL_COLOR, "%s  %s", entry_.time.c_str(), entry_.toolName.c_str());
    if (!entry_.arguments.empty() && entry_.arguments != "{}") {
        ImGui::PushStyleColor(ImGuiCol_Text, CALL_COLOR);
        ImGui::TextWrapped("%s", entry_.arguments.c_str());
        ImGui::PopStyleColor();
    }

    if (entry_.renderWidget) {
        // The live control the tool built, redrawn every frame from the GUI's current state --
        // so a results table shown here keeps counting up while the run continues.
        entry_.renderWidget();
        return {};
    }

    ImGui::PushStyleColor(
        ImGuiCol_Text, entry_.success ? RESULT_COLOR : StepColors::ERROR_COLOR);
    ImGui::TextWrapped("%s", entry_.content.c_str());
    ImGui::PopStyleColor();
    return {};
}

void RemoteCallCapture::install() {
    if (handle_) {
        return;
    }
    handle_ = StaticCallbacks::remoteCall().registerCallback([this](const RemoteCallEntry& entry) {
        std::scoped_lock lock(mutex_);
        pending_.push_back(entry);
    });
}

void RemoteCallCapture::uninstall() {
    handle_.reset();
}

void RemoteCallCapture::appendCapturedSteps(std::vector<std::unique_ptr<ChatbotStep>>& steps) {
    std::vector<RemoteCallEntry> collected;
    {
        std::scoped_lock lock(mutex_);
        collected.swap(pending_);
    }
    for (auto& entry : collected) {
        steps.push_back(std::make_unique<ChatbotStepRemoteCall>(std::move(entry)));
    }
}

ChatbotRemoteControl::ChatbotRemoteControl(int port) : port_(port) {
}

void ChatbotRemoteControl::start() {
    finished_ = false;
    ended_ = false;
    capture_.install();
}

bool ChatbotRemoteControl::draw() {
    capture_.appendCapturedSteps(steps_);

    if (ended_) {
        ImGuiControls::textDisabled("Remote control has ended. The application is yours again.");
    } else {
        ImGui::TextColored(StepColors::SUCCESS_COLOR, "%s",
            std::format("Remote control is listening on 127.0.0.1:{}.", port_).c_str());
    }
    ImGui::TextWrapped("%s",
        "Everything an outside caller does appears below, and in the rest of the application as "
        "it happens -- games on the boards, results in the tournament and SPRT views. You can "
        "keep using all of it yourself while this runs.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (steps_.empty()) {
        ImGuiControls::textDisabled("Nothing has been called yet.");
    }
    for (const auto& step : steps_) {
        // A step may ask to stop the thread; these never do -- they are a record of something
        // that already happened. Said out loud so the discarded value is a decision, not a slip.
        static_cast<void>(step->draw());
        ImGui::Spacing();
    }

    // The one activity with nothing of its own to look at: CLOP has no tab, so its live numbers
    // belong here. Below the call log rather than above it: this window scrolls to the bottom as
    // entries arrive, so anything placed at the top is pushed out of sight by the very calls that
    // ask about it. The log is what happened, this is what is happening, and the view already
    // sits where "now" is. Redrawn every frame -- what "the CLI prints this every n samples"
    // turns into when the reader is a person watching a window.
    auto& clop = QaplaWindows::ClopData::instance();
    if (clop.hasTables()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(StepColors::SUCCESS_COLOR, "%s", "CLOP tuning");
        clop.drawTables();
    }

    if (ended_) {
        return true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGuiControls::textButton("End remote control")) {
        // Announced, not called: this window has no idea what is listening out there, and does
        // not need one. Whoever opened the channel closes it -- and closes nothing else, a run
        // that is playing keeps playing. Clearing the exclusive thread first means that when
        // this one reports itself finished a moment later, the ChatbotWindow has its menu back.
        ended_ = true;
        capture_.uninstall();
        StaticCallbacks::message().invokeAll(END_MESSAGE);
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
    return std::make_unique<ChatbotRemoteControl>(port_);
}

} // namespace QaplaWindows::ChatBot
