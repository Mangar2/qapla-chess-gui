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
#include "activity-watch.h"
#include "lm-studio-locator.h"
#include "gui-tool-registry.h"
#include "actions/gui-action-activity.h"
#include "tools/gui-tools.h"
#include "../configuration.h"
#include "../callback-manager.h"
#include "../chatbot/chatbot-window.h"
#include "../chatbot/chatbot-llm-chat.h"

#include <base-elements/timer.h>

#include <memory>
#include <utility>

namespace QaplaLlm {

namespace {

// How often to retry detection while it hasn't succeeded yet (e.g. LM Studio genuinely
// isn't reachable at the currently configured host/port). Deliberately more relaxed than
// ChatbotLlmChat's own in-chat 3s refresh: this runs for the whole app lifetime in the
// background, including for users who never touch the AI Chat feature at all.
constexpr uint64_t RETRY_INTERVAL_MS = 10000;

struct DetectionState {
    explicit DetectionState(LmStudioProbeConfig config) : locator(std::move(config)) {
    }

    AsyncLmStudioLocator locator;
};

// Registers every available GUI tool group (see src/llm/tools/gui-tools.h). Registration is cheap
// and unconditional -- tools only ever run when a chat actually calls them, so there is no reason
// to gate this on LM Studio being found.
void registerAllGuiTools() {
    registerGuiTools(GuiToolRegistry::instance());
}

// Retries LM Studio detection periodically -- not just once at startup -- reading
// host/port/enabled from Configuration fresh on every attempt. Without this, fixing a
// wrong or unreachable LM Studio address in Settings (e.g. after pointing it at a remote
// server) would silently do nothing: the "AI Chat" menu entry would already have failed to
// appear at startup (detect() returning NotInstalled for the old settings) and nothing
// would ever retry, leaving a restart as the only way to pick up the corrected settings.
// Stops polling for good the first time detection succeeds and the menu entry is added --
// ChatbotWindow::registerThread() has no dedupe, so calling it twice would add the entry
// twice.
class LlmChatMenuRegistrar {
public:
    void poll() {
        if (detection_) {
            pollPendingDetection();
            return;
        }
        if (done_) {
            return;
        }
        maybeStartAttempt();
    }

private:
    void pollPendingDetection() {
        if (!detection_->locator.isReady()) {
            return;
        }

        auto status = detection_->locator.status();
        detection_.reset();
        lastAttemptCompletedMs_ = QaplaHelpers::Timer::getCurrentTimeMs();

        if (status != LmStudioStatus::NotInstalled) {
            QaplaWindows::ChatBot::ChatbotWindow::instance()->registerThread(
                std::make_unique<QaplaWindows::ChatBot::ChatbotLlmChat>(status));
            done_ = true;
        }
        // else: NotInstalled -- maybeStartAttempt() will try again once the retry
        // interval elapses, re-reading Configuration in case it changed meanwhile.
    }

    void maybeStartAttempt() {
        if (!QaplaConfiguration::Configuration::getLlmChatConfig().enabled) {
            return; // re-checked every attempt: stays responsive if enabled later
        }
        if (lastAttemptCompletedMs_ != 0 &&
            QaplaHelpers::Timer::getCurrentTimeMs() - lastAttemptCompletedMs_ < RETRY_INTERVAL_MS) {
            return;
        }

        auto config = QaplaConfiguration::Configuration::getLlmChatConfig();
        LmStudioProbeConfig probeConfig;
        probeConfig.host = config.host;
        probeConfig.port = config.port;

        detection_ = std::make_unique<DetectionState>(probeConfig);
        detection_->locator.start();
    }

    std::unique_ptr<DetectionState> detection_;
    uint64_t lastAttemptCompletedMs_ = 0;
    bool done_ = false;
};

} // namespace

void initializeLlmChat() {
    registerAllGuiTools();

    // Both intentionally kept alive for the whole process (static, never reset): the
    // tool-call job queue must be drained for the lifetime of the application, and the menu
    // registrar keeps retrying detection for just as long (see LlmChatMenuRegistrar). Letting
    // either returned UnregisterHandle go out of scope would immediately unregister it.
    static auto toolQueuePollHandle = QaplaWindows::StaticCallbacks::poll().registerCallback([]() {
        GuiToolRegistry::instance().processQueue();
    });

    // Samples the three activities every frame so a caller can wait on one of them (see
    // ActivityWatch). Hooked here rather than inside each activity's own code because it must
    // notice a run ending by any route at all -- finished on its own, stopped through a tool,
    // stopped by the user clicking the button. Watching from outside catches all three; asking
    // each stop path to report itself would have missed whichever one was added next.
    static auto activityWatchPollHandle = QaplaWindows::StaticCallbacks::poll().registerCallback(
        []() {
            for (auto activity : {Actions::Activity::Tournament, Actions::Activity::Sprt,
                     Actions::Activity::Epd, Actions::Activity::Clop}) {
                ActivityWatch::instance().update(activity, Actions::activityProgress(activity));
            }
        });

    static LlmChatMenuRegistrar menuRegistrar;
    static auto menuRegistrarPollHandle = QaplaWindows::StaticCallbacks::poll().registerCallback([]() {
        menuRegistrar.poll();
    });
}

} // namespace QaplaLlm
