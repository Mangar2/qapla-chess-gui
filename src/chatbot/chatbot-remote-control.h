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

#include "chatbot-step.h"
#include "chatbot-thread.h"

#include "../callback-manager.h"
#include "../remote-call-entry.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace QaplaWindows::ChatBot {

/**
 * @brief One call from outside, shown as a step that holds its own content.
 *
 * Exactly what ChatbotStepSnackbarMessage is for a captured snackbar: the text belongs to the
 * step, in the window, and nowhere else. Nothing keeps a log to be read back -- the entry arrived
 * once and lives here from then on.
 */
class ChatbotStepRemoteCall : public ChatbotStep {
public:
    explicit ChatbotStepRemoteCall(RemoteCallEntry entry);

    [[nodiscard]] std::string draw() override;

private:
    RemoteCallEntry entry_;
};

/**
 * @brief Collects calls announced on StaticCallbacks::remoteCall() and turns them into steps.
 *
 * The same shape as SnackbarCapture, with one difference that matters: those arrive on the UI
 * thread, these do not -- the channel fires on whichever thread answered the call. So what comes
 * in is parked under a lock and picked up in draw(), which is on the UI thread by definition.
 */
class RemoteCallCapture {
public:
    ~RemoteCallCapture() = default;

    RemoteCallCapture() = default;
    RemoteCallCapture(const RemoteCallCapture&) = delete;
    RemoteCallCapture& operator=(const RemoteCallCapture&) = delete;

    /** @brief Starts listening. Calls announced from now on become steps. */
    void install();

    /** @brief Stops listening. Anything already collected stays where it is. */
    void uninstall();

    /** @brief Appends a step for everything collected since the last call. */
    void appendCapturedSteps(std::vector<std::unique_ptr<ChatbotStep>>& steps);

private:
    mutable std::mutex mutex_;
    std::vector<RemoteCallEntry> pending_;
    std::unique_ptr<Callback::UnregisterHandle> handle_;
};

/**
 * @brief The log of everything the remote control has done, shown while it is active.
 *
 * Looks like the AI chat and is not it: there is no input field, because the other side of this
 * conversation is not in the room. What it does have is the same rendering, tables included -- a
 * result the caller received as text is drawn here as the real ImGuiTable (see
 * RemoteCallEntry::renderWidget), so one call answers the caller and shows the user at the same
 * time.
 *
 * It knows nothing about how those calls arrive. There is no HTTP here, no server, no port to ask
 * -- the port is text it was handed when it was made, and the calls are announcements it
 * registered for. Ending the remote control is a message it sends, not a function it calls.
 *
 * While it runs it is the only thread the ChatbotWindow offers (see
 * ChatbotWindow::setExclusiveThread): one steering wheel at a time, and it is the remote one.
 * Everything else about the GUI stays usable -- boards, tabs, the tournament and SPRT views with
 * their live numbers -- and the user may drive by hand at the same time. That can collide with
 * what the remote side is doing; that is allowed and not prevented. Locking it would break the
 * one thing the GUI has over a headless run, which is that the human stays in charge of their
 * own application.
 */
class ChatbotRemoteControl : public ChatbotThread {
public:
    /** @brief The message that ends the remote control, sent when the button is pressed. */
    static constexpr const char* END_MESSAGE = "end_remote_control";

    explicit ChatbotRemoteControl(int port = 0);
    ~ChatbotRemoteControl() override = default;

    [[nodiscard]] std::string getTitle() const override {
        return "Remote Control";
    }

    void start() override;
    bool draw() override;
    [[nodiscard]] bool isFinished() const override;
    [[nodiscard]] std::unique_ptr<ChatbotThread> clone() const override;

private:
    int port_;
    bool ended_ = false;
    bool finished_ = false;

    RemoteCallCapture capture_;
    std::vector<std::unique_ptr<ChatbotStep>> steps_;
};

} // namespace QaplaWindows::ChatBot
