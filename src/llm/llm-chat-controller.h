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

#include "lm-studio-client.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace QaplaLlm {

/**
 * @brief Role of one entry in the locally displayed chat history.
 */
enum class ChatRole {
    User,
    Assistant,
    Error ///< Local/network/protocol error shown inline; never replayed to the model.
};

struct ChatEntry {
    ChatRole role;
    std::string text;
};

/**
 * @brief Drives a request/response conversation with a local LM Studio model.
 *
 * Each request runs on a detached worker thread; update() must be called
 * once per frame from the UI thread to pick up results. No tool-calling
 * loop yet (see docs/llm-chatbot-plan.md Step 3) and no streaming (Step 7).
 *
 * Thread-safety: all public methods (including update()) are meant to be
 * called from a single thread (the UI thread). Worker threads only ever
 * touch their own PendingChat/PendingModels state, exchanged with the UI
 * thread through a std::shared_ptr and a release/acquire "done" flag.
 */
class LlmChatController {
public:
    LlmChatController(LmStudioConnection connection, std::string systemPrompt);

    /** @brief Starts a non-blocking model list refresh. No-op if already in flight. */
    void refreshModels();
    [[nodiscard]] bool isRefreshingModels() const {
        return refreshingModels_;
    }
    [[nodiscard]] const std::vector<std::string>& availableModels() const {
        return availableModels_;
    }
    [[nodiscard]] const std::string& modelsError() const {
        return modelsError_;
    }

    void setModel(std::string modelId) {
        model_ = std::move(modelId);
    }
    [[nodiscard]] const std::string& model() const {
        return model_;
    }

    /** @brief Appends the user's message and starts the model request. No-op if isBusy() or text is empty. */
    void sendMessage(const std::string& userText);

    /** @brief True while a chat request is in flight. */
    [[nodiscard]] bool isBusy() const {
        return busy_;
    }

    /**
     * @brief Abandons the in-flight request.
     *
     * Best-effort: the worker thread keeps running to completion in the
     * background (bounded by the connection's request timeout), but its
     * result is orphaned and never applied.
     */
    void stop();

    [[nodiscard]] const std::vector<ChatEntry>& history() const {
        return history_;
    }

    /** @brief Must be called once per frame from the UI thread to pick up worker results. */
    void update();

private:
    struct PendingChat {
        std::atomic<bool> done{false};
        ChatCompletionResult result;
    };
    struct PendingModels {
        std::atomic<bool> done{false};
        ListModelsResult result;
    };

    LmStudioClient client_;
    std::string systemPrompt_;
    std::string model_;
    std::vector<ChatEntry> history_;

    bool busy_ = false;
    std::shared_ptr<PendingChat> pendingChat_;

    bool refreshingModels_ = false;
    std::shared_ptr<PendingModels> pendingModels_;
    std::vector<std::string> availableModels_;
    std::string modelsError_;
};

} // namespace QaplaLlm
