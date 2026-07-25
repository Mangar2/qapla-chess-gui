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

#include "llm-chat-controller.h"

#include <thread>
#include <utility>

namespace QaplaLlm {

LlmChatController::LlmChatController(LmStudioConnection connection, std::string systemPrompt)
    : client_(std::move(connection)), systemPrompt_(std::move(systemPrompt)) {
}

void LlmChatController::refreshModels() {
    if (refreshingModels_) {
        return;
    }
    refreshingModels_ = true;

    auto state = std::make_shared<PendingModels>();
    pendingModels_ = state;

    auto client = client_;
    std::thread([state, client]() {
        state->result = client.listModels();
        state->done.store(true, std::memory_order_release);
    }).detach();
}

void LlmChatController::sendMessage(const std::string& userText) {
    if (busy_ || userText.empty()) {
        return;
    }

    history_.push_back({ChatRole::User, userText});

    ChatCompletionRequest request;
    request.model = model_;
    request.messages.push_back({"system", systemPrompt_});
    for (const auto& entry : history_) {
        if (entry.role == ChatRole::User) {
            request.messages.push_back({"user", entry.text});
        } else if (entry.role == ChatRole::Assistant) {
            request.messages.push_back({"assistant", entry.text});
        }
        // Error entries are shown to the user locally but never replayed to the model.
    }

    auto state = std::make_shared<PendingChat>();
    pendingChat_ = state;
    busy_ = true;

    auto client = client_;
    std::thread([state, client, request = std::move(request)]() {
        state->result = client.chatCompletion(request);
        state->done.store(true, std::memory_order_release);
    }).detach();
}

void LlmChatController::stop() {
    if (!busy_) {
        return;
    }
    // Orphan the in-flight worker: dropping our shared_ptr means update()
    // can never observe its result, even once the thread finishes.
    pendingChat_.reset();
    busy_ = false;
    history_.push_back({ChatRole::Error, "Request cancelled."});
}

void LlmChatController::update() {
    if (busy_ && pendingChat_ && pendingChat_->done.load(std::memory_order_acquire)) {
        auto result = pendingChat_->result;
        pendingChat_.reset();
        busy_ = false;
        if (result.success) {
            history_.push_back({ChatRole::Assistant, result.content});
        } else {
            history_.push_back({ChatRole::Error, result.errorMessage});
        }
    }

    if (refreshingModels_ && pendingModels_ && pendingModels_->done.load(std::memory_order_acquire)) {
        auto result = pendingModels_->result;
        pendingModels_.reset();
        refreshingModels_ = false;
        if (result.success) {
            availableModels_ = result.modelIds;
            modelsError_.clear();
            if (model_.empty() && !availableModels_.empty()) {
                model_ = availableModels_.front();
            }
        } else {
            modelsError_ = result.errorMessage;
        }
    }
}

} // namespace QaplaLlm
