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
#include "gui-tool-registry.h"

#include <thread>
#include <utility>

namespace QaplaLlm {

namespace {

    // Runs entirely on the worker thread: calls the model, and for as long as
    // it keeps responding with tool_calls, executes them via GuiToolRegistry
    // (which itself blocks this thread until the UI thread has processed
    // them) and asks again, until a final text answer or the iteration limit.
    AgentTurnResult runAgentLoop(
        const LmStudioClient& client,
        const std::string& model,
        const std::vector<ToolSpec>& tools,
        std::vector<ChatMessage> messages,
        int maxToolIterations) {

        AgentTurnResult turnResult;

        for (int iteration = 0; iteration < maxToolIterations; ++iteration) {
            ChatCompletionRequest request;
            request.model = model;
            request.messages = messages;
            request.tools = tools;

            auto response = client.chatCompletion(request);
            if (!response.success) {
                turnResult.errorMessage = response.errorMessage;
                return turnResult;
            }

            if (response.toolCalls.empty()) {
                turnResult.success = true;
                turnResult.finalContent = response.content;
                return turnResult;
            }

            // Echo the assistant's tool-call request back, then answer each
            // one -- both are required by the API to keep the message
            // sequence valid for the next round.
            ChatMessage assistantMessage;
            assistantMessage.role = "assistant";
            assistantMessage.content = response.content;
            assistantMessage.toolCalls = response.toolCalls;
            messages.push_back(std::move(assistantMessage));

            for (const auto& call : response.toolCalls) {
                GuiToolResult toolResult = GuiToolRegistry::instance().callTool(call.name, call.argumentsJson);

                ChatMessage toolMessage;
                toolMessage.role = "tool";
                toolMessage.toolCallId = call.id;
                toolMessage.content = toolResult.content;
                messages.push_back(std::move(toolMessage));

                turnResult.toolEvents.push_back(ToolCallEvent{
                    .toolName = call.name,
                    .success = toolResult.success,
                    .resultSummary = toolResult.content
                });
            }
        }

        turnResult.errorMessage = "Stopped after reaching the tool-call iteration limit.";
        return turnResult;
    }

} // namespace

LlmChatController::LlmChatController(LmStudioConnection connection, std::string systemPrompt, int maxToolIterations)
    : client_(std::move(connection)), systemPrompt_(std::move(systemPrompt)), maxToolIterations_(maxToolIterations) {
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

    std::vector<ChatMessage> messages;
    messages.push_back(ChatMessage{.role = "system", .content = systemPrompt_});
    for (const auto& entry : history_) {
        if (entry.role == ChatRole::User) {
            messages.push_back(ChatMessage{.role = "user", .content = entry.text});
        } else if (entry.role == ChatRole::Assistant) {
            messages.push_back(ChatMessage{.role = "assistant", .content = entry.text});
        }
        // Tool/Error entries are shown to the user locally but never replayed to the model.
    }

    auto state = std::make_shared<PendingChat>();
    pendingChat_ = state;
    busy_ = true;

    auto client = client_;
    auto model = model_;
    auto tools = GuiToolRegistry::instance().exportToolSpecs();
    auto maxToolIterations = maxToolIterations_;

    std::thread([state, client, model, tools, messages = std::move(messages), maxToolIterations]() mutable {
        state->result = runAgentLoop(client, model, tools, std::move(messages), maxToolIterations);
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

void LlmChatController::applyTurnResult(const AgentTurnResult& result) {
    for (const auto& event : result.toolEvents) {
        // Successful tool results are already phrased as a friendly,
        // user-facing sentence by the tool itself (see
        // gui-tool-*-register.cpp) -- no technical tool name needed.
        // Registry-level failures (unknown tool, timeout, bad arguments)
        // are developer-facing edge cases, so keep the name there for
        // anyone trying to diagnose what went wrong.
        std::string text = event.success
            ? event.resultSummary
            : (event.toolName + ": " + event.resultSummary);
        if (text.empty()) {
            text = event.toolName;
        }
        history_.push_back({event.success ? ChatRole::Tool : ChatRole::Error, text});
    }

    if (!result.success) {
        history_.push_back({ChatRole::Error, result.errorMessage});
    } else if (!result.finalContent.empty()) {
        history_.push_back({ChatRole::Assistant, result.finalContent});
    }
}

void LlmChatController::applyModelsResult(const ListModelsResult& result) {
    if (!result.success) {
        modelsError_ = result.errorMessage;
        return;
    }

    availableModels_ = result.modelIds;
    modelsError_.clear();
    if (model_.empty() && !availableModels_.empty()) {
        model_ = availableModels_.front();
    }
}

void LlmChatController::update() {
    if (busy_ && pendingChat_ && pendingChat_->done.load(std::memory_order_acquire)) {
        auto result = pendingChat_->result;
        pendingChat_.reset();
        busy_ = false;
        applyTurnResult(result);
    }

    if (refreshingModels_ && pendingModels_ && pendingModels_->done.load(std::memory_order_acquire)) {
        auto result = pendingModels_->result;
        pendingModels_.reset();
        refreshingModels_ = false;
        applyModelsResult(result);
    }
}

} // namespace QaplaLlm
