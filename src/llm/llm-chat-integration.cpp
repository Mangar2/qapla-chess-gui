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

#include "llm-chat-integration.h"
#include "lm-studio-locator.h"
#include "../configuration.h"
#include "../callback-manager.h"
#include "../chatbot/chatbot-window.h"
#include "../chatbot/chatbot-llm-chat.h"

#include <memory>
#include <utility>

namespace QaplaLlm {

namespace {

// Holds the async locator plus its own poll-callback handle so it can
// unregister itself from QaplaWindows::StaticCallbacks::poll() once the
// detection result has been consumed.
struct DetectionState {
    explicit DetectionState(LmStudioProbeConfig config) : locator(std::move(config)) {
    }

    AsyncLmStudioLocator locator;
    std::unique_ptr<QaplaWindows::Callback::UnregisterHandle> pollHandle;
};

} // namespace

void startLlmChatDetection() {
    auto config = QaplaConfiguration::Configuration::getLlmChatConfig();
    if (!config.enabled) {
        return;
    }

    LmStudioProbeConfig probeConfig;
    probeConfig.host = config.host;
    probeConfig.port = config.port;

    auto state = std::make_shared<DetectionState>(probeConfig);
    state->locator.start();

    state->pollHandle = QaplaWindows::StaticCallbacks::poll().registerCallback([state]() {
        if (!state->locator.isReady()) {
            return;
        }

        auto status = state->locator.status();
        if (status != LmStudioStatus::NotInstalled) {
            QaplaWindows::ChatBot::ChatbotWindow::instance()->registerThread(
                std::make_unique<QaplaWindows::ChatBot::ChatbotLlmChat>(status));
        }

        // Detection only ever runs once at startup; unregister to stop polling.
        state->pollHandle.reset();
    });
}

} // namespace QaplaLlm
