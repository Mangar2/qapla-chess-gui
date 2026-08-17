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

#include "chatbot-thread.h"

#include "../llm/remote-control-server.h"

#include <memory>
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief The log of everything the remote control has done, shown while it is active.
 *
 * Looks like the AI chat and is not it: there is no input field, because the other side of this
 * conversation is not in the room. What it does have is the same rendering, tables included -- a
 * result the caller received as text is drawn here as the real ImGuiTable (see
 * QaplaLlm::RemoteCallEntry::renderWidget), so one call answers the caller and shows the user at
 * the same time.
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
    [[nodiscard]] std::string getTitle() const override {
        return "Remote Control";
    }

    void start() override;
    bool draw() override;
    [[nodiscard]] bool isFinished() const override;
    [[nodiscard]] std::unique_ptr<ChatbotThread> clone() const override;

private:
    /** @brief Draws one logged call: its heading line, then its result or its live table. */
    static void drawEntry(const QaplaLlm::RemoteCallEntry& entry);

    bool finished_ = false;
};

/**
 * @brief Starts the remote control and makes its log the only thing the chatbot panel offers.
 *
 * Reports failure to the user through the snackbar (an occupied port is the realistic case) and
 * leaves the GUI in its ordinary state, since a GUI nobody can reach from outside is still a
 * perfectly good GUI.
 *
 * @return Whether the server is now listening.
 */
bool startRemoteControl(const QaplaLlm::RemoteControlOptions& options);

} // namespace QaplaWindows::ChatBot
