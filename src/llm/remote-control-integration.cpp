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

#include "remote-control-integration.h"

#include "../callback-manager.h"
#include "../chatbot/chatbot-remote-control.h"
#include "../chatbot/chatbot-window.h"
#include "../snackbar.h"

#include <format>
#include <memory>

namespace QaplaLlm {

bool startRemoteControl(const RemoteControlOptions& options, bool withPanel) {
    auto& server = RemoteControlServer::instance();
    if (!server.start(options)) {
        QaplaWindows::SnackbarManager::instance().showError(
            std::format("Could not start the remote control on port {} -- the port is in use. "
                        "The application works normally, it just cannot be driven from outside.",
                options.port),
            false, "remote-control");
        return false;
    }

    // The panel's "End remote control" button says so and nothing more; closing the channel is
    // this side's business. Kept for the lifetime of the process, like the other integration
    // subscriptions -- there is one remote control and it is started once.
    static auto endHandle = QaplaWindows::StaticCallbacks::message().registerCallback(
        [](const std::string& message) {
            if (message == QaplaWindows::ChatBot::ChatbotRemoteControl::END_MESSAGE) {
                RemoteControlServer::instance().stop();
            }
        });

    if (withPanel) {
        // The port as text, handed over once. The panel has nothing to ask afterwards.
        QaplaWindows::ChatBot::ChatbotWindow::instance()->setExclusiveThread(
            std::make_unique<QaplaWindows::ChatBot::ChatbotRemoteControl>(server.port()));
    }
    return true;
}

} // namespace QaplaLlm
