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

#include "remote-control-server.h"

/**
 * @file
 * @brief The one place that knows both the remote control and the window that shows it.
 *
 * Sits on this side of the fence for the same reason llm-chat-integration.cpp does: the chatbot
 * is a display, and a display should not have to know what it is displaying the output of. So the
 * server knows nothing of any window, the window knows nothing of any server, and the two are
 * introduced here -- in a .cpp, once, at startup.
 */

namespace QaplaLlm {

/**
 * @brief Starts the remote control and gives the chatbot panel a view of it.
 *
 * Reports failure to the user through the snackbar (an occupied port is the realistic case) and
 * leaves the GUI in its ordinary state, since a GUI nobody can reach from outside is still a
 * perfectly good GUI.
 *
 * @param withPanel Whether the chatbot window is given over to the remote control's log. Off
 *        during an automated GUI test run: those tests drive the chat themselves, and the panel
 *        takes it over exclusively. The channel is wanted there all the same -- it is how one
 *        asks a failing test what the application actually thinks its state is.
 * @return Whether the server is now listening.
 */
bool startRemoteControl(const RemoteControlOptions& options, bool withPanel = true);

} // namespace QaplaLlm
