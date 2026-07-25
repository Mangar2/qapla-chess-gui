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
#include "../llm/lm-studio-locator.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief Chatbot thread offering a free-text chat with a local LM Studio model.
 *
 * Registered in the ChatbotWindow only when an LM Studio installation was
 * detected (see LmStudioLocator). This first version only shows the
 * detected status and an inactive input field; the actual conversation is
 * wired up in a later development step.
 *
 * The status shown is re-probed periodically (throttled) while this step is
 * open, rather than frozen at construction time: LM Studio's server can be
 * started/stopped by the user at any time, independently of the GUI.
 */
class ChatbotLlmChat : public ChatbotThread {
public:
    explicit ChatbotLlmChat(QaplaLlm::LmStudioStatus status);

    [[nodiscard]] std::string getTitle() const override {
        return "AI Chat";
    }
    void start() override;
    bool draw() override;
    [[nodiscard]] bool isFinished() const override;
    [[nodiscard]] std::unique_ptr<ChatbotThread> clone() const override;

private:
    /**
     * @brief Refreshes status_ from a throttled, non-blocking re-probe.
     *
     * Starts a new async probe at most once per refresh interval, and only
     * once any previous probe has completed; never blocks the UI thread.
     */
    void refreshStatus();

    QaplaLlm::LmStudioStatus status_;
    bool finished_ = false;
    std::optional<QaplaLlm::AsyncLmStudioLocator> refreshProbe_;
    uint64_t lastProbeCompletedMs_ = 0;
};

} // namespace QaplaWindows::ChatBot
