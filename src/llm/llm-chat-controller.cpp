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

#include <base-elements/qapla-json.h>

#include <functional>
#include <utility>

namespace QaplaLlm {

namespace {

    // Follow-up turns (model already contacted once, presumably already loaded into memory).
    constexpr int MESSAGE_TIMEOUT_MS = 60000;
    // First contact with a model: LM Studio may still need to load it into memory/VRAM, which
    // can legitimately take much longer than a routine follow-up turn.
    constexpr int PING_TIMEOUT_MS = 180000;

    // Not a real GUI action -- a pseudo-tool the model must call to say anything to the user at
    // all (see chatCompletion's "tool_choice":"required"). This turns every reply into
    // structured, schema-validated output instead of arbitrary free text, so there's no longer
    // a "the model wrote something unparseable/hallucinated-looking directly into the chat"
    // failure mode to filter after the fact -- either it's a valid call to this tool (or a real
    // GUI tool), or the response is rejected/retried by LM Studio's own grammar-constrained
    // decoding before it ever reaches us (backend/model support permitting; see the
    // "tool_choice" comment in lm-studio-client.cpp for the best-effort fallback).
    constexpr const char* REPLY_TOOL_NAME = "reply_to_user";

    ToolSpec replyToUserToolSpec() {
        return ToolSpec{
            .name = REPLY_TOOL_NAME,
            .description =
                "Send your reply to the user. This is the ONLY way to say anything to them -- "
                "never answer directly as plain text; always call this tool instead, even for a "
                "short confirmation, a clarifying question, or just acknowledging a result. Put "
                "your COMPLETE answer in this call's \"text\" argument: any separate message "
                "content you write alongside this tool call (outside \"text\") is never shown to "
                "the user and is silently discarded, so never split your answer between the two "
                "or write a short placeholder here while putting the real explanation elsewhere. "
                "If a tool's own description says it already displays its result directly in the "
                "chat (e.g. show_tournament_result), do not repeat that data here -- call this "
                "with just a short confirmation sentence.",
            .parametersSchemaJson =
                R"({"type":"object","properties":{"text":{"type":"string",)"
                R"("description":"Your complete message to the user, in their own language -- )"
                R"(this is the only part of your response the user will ever see."}},)"
                R"("required":["text"]})"
        };
    }

    // Best-effort: an empty/unparseable argument still ends the turn (with an empty reply)
    // rather than crashing or looping -- a malformed call here is a model/backend quirk, not
    // something worth surfacing as a hard error to the user.
    std::string extractReplyText(const std::string& argumentsJson) {
        auto parsed = QaplaTester::Json::JsonValue::try_parse(argumentsJson);
        if (parsed && parsed->is_object() && parsed->contains("text") && parsed->at("text").is_string()) {
            return parsed->at("text").as_string();
        }
        return "";
    }

    // Runs entirely on the worker thread: calls the model, and for as long as
    // it keeps responding with tool_calls, executes them via GuiToolRegistry
    // (which itself blocks this thread until the UI thread has processed
    // them) and asks again, until a final text answer or the iteration limit.
    // Each tool result is reported through onToolEvent as soon as it happens
    // -- not batched into the returned AgentTurnResult -- so the UI can show
    // it immediately instead of waiting for the model's (often much slower)
    // final reply.
    AgentTurnResult runAgentLoop(
        const LmStudioClient& client,
        const std::string& model,
        const std::vector<ToolSpec>& tools,
        std::vector<ChatMessage> messages,
        int maxToolIterations,
        LmStudioCancelHandle* cancelHandle,
        LlmChatLogger* logger,
        LlmFineTuningWriter* fineTuningWriter,
        const std::function<void(ToolCallEvent)>& onToolEvent) {

        AgentTurnResult turnResult;
        // Recorded to finetuning.json only if the turn ends via reply_to_user with this still
        // true, i.e. no tool ever reported failure -- see LlmFineTuningWriter's class docs.
        bool turnClean = true;
        // messages currently ends with [system, ...earlier turns (flattened text-only)...,
        // this turn's new user message] -- index of that user message, so the eventual
        // fine-tuning record can be just [system, user, ...this turn's own exchange] without
        // the unrelated earlier-turn messages in between.
        const std::size_t turnStartIndex = messages.size() - 1;

        for (int iteration = 0; iteration < maxToolIterations; ++iteration) {
            ChatCompletionRequest request;
            request.model = model;
            request.messages = messages;
            request.tools = tools;

            auto response = client.chatCompletion(request, cancelHandle, MESSAGE_TIMEOUT_MS);
            if (!response.success) {
                turnResult.errorMessage = response.errorMessage;
                logger->logLine("ERROR", response.errorMessage);
                return turnResult;
            }

            if (response.toolCalls.empty()) {
                // The model ignored "tool_choice":"required" (see chatCompletion) and answered
                // with free text instead of a tool call -- exactly the "unstructured" output the
                // log caps, since a model stuck looping this can't be trusted not to flood it.
                turnResult.success = true;
                turnResult.finalContent = response.content;
                logger->logUnstructured(response.content);
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

            if (!response.content.empty()) {
                logger->logLine("ASSISTANT_CONTENT", response.content);
            }

            for (const auto& call : response.toolCalls) {
                if (call.name == REPLY_TOOL_NAME) {
                    // Not a real GUI action -- this call's "content" IS the reply, so it ends
                    // the turn (after the rest of this iteration's calls, if any, still run)
                    // rather than going through GuiToolRegistry/onToolEvent like a real tool.
                    turnResult.success = true;
                    turnResult.finalContent = extractReplyText(call.argumentsJson);
                    logger->logLine("REPLY", turnResult.finalContent);
                    continue;
                }

                logger->logLine("TOOL_CALL", call.name + " args=" + call.argumentsJson);
                GuiToolResult toolResult = GuiToolRegistry::instance().callTool(call.name, call.argumentsJson);
                logger->logLine(toolResult.success ? "TOOL_RESULT" : "TOOL_ERROR", toolResult.content);
                if (!toolResult.success) {
                    turnClean = false;
                }

                ChatMessage toolMessage;
                toolMessage.role = "tool";
                toolMessage.toolCallId = call.id;
                toolMessage.content = toolResult.content;
                messages.push_back(std::move(toolMessage));

                onToolEvent(ToolCallEvent{
                    .toolName = call.name,
                    .success = toolResult.success,
                    .resultSummary = toolResult.content,
                    .renderWidget = toolResult.renderWidget
                });
            }

            if (turnResult.success) {
                if (turnClean) {
                    std::vector<ChatMessage> fineTuningMessages;
                    fineTuningMessages.push_back(messages.front()); // system prompt
                    fineTuningMessages.insert(fineTuningMessages.end(),
                        messages.begin() + static_cast<std::ptrdiff_t>(turnStartIndex), messages.end());
                    fineTuningWriter->appendTurn(fineTuningMessages);
                }
                return turnResult;
            }
        }

        turnResult.errorMessage = "Stopped after reaching the tool-call iteration limit.";
        logger->logLine("ERROR", turnResult.errorMessage);
        return turnResult;
    }

} // namespace

LlmChatController::LlmChatController(
    LmStudioConnection connection, std::string systemPrompt, int maxToolIterations, bool logTraffic,
    std::string logDirectory)
    : client_(std::move(connection)), systemPrompt_(std::move(systemPrompt)), maxToolIterations_(maxToolIterations),
      logger_(std::make_shared<LlmChatLogger>(logTraffic, logDirectory)),
      fineTuningWriter_(std::make_shared<LlmFineTuningWriter>(std::move(logDirectory))) {
}

void LlmChatController::refreshModels() {
    if (modelsTask_.isRunning()) {
        return;
    }

    auto client = client_;
    modelsTask_.start([client]() { return client.listModels(); });
}

void LlmChatController::sendMessage(const std::string& userText) {
    if (isBusy() || userText.empty()) {
        return;
    }

    history_.push_back({ChatRole::User, userText, nullptr});
    logger_->logLine("USER", userText);

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

    auto events = std::make_shared<ToolEventChannel>();
    toolEvents_ = events;

    chatCancelHandle_ = std::make_shared<LmStudioCancelHandle>();

    auto client = client_;
    auto model = model_;
    auto tools = GuiToolRegistry::instance().exportToolSpecs();
    tools.push_back(replyToUserToolSpec());
    auto maxToolIterations = maxToolIterations_;
    auto cancelHandle = chatCancelHandle_;
    auto logger = logger_;
    auto fineTuningWriter = fineTuningWriter_;

    chatTask_.start([client, model, tools, messages = std::move(messages), maxToolIterations, events,
                      cancelHandle, logger, fineTuningWriter]() mutable {
        auto onToolEvent = [events](ToolCallEvent event) {
            std::scoped_lock lock(events->mutex);
            events->events.push_back(std::move(event));
        };
        return runAgentLoop(
            client, model, tools, std::move(messages), maxToolIterations, cancelHandle.get(), logger.get(),
            fineTuningWriter.get(), onToolEvent);
    });
}

void LlmChatController::stop() {
    if (!isBusy()) {
        return;
    }
    if (chatCancelHandle_) {
        // Actually abort the in-flight HTTP request (closes the TCP connection to LM
        // Studio) instead of just discarding the result locally -- see LmStudioCancelHandle.
        chatCancelHandle_->cancel();
    }
    // Orphan the in-flight worker (see AsyncWorkerResult): update() can
    // never observe its result, even once the thread finishes. Any tool
    // events already drained into history_ before this point stay -- their
    // real-world effects already happened.
    chatTask_.reset();
    toolEvents_.reset();
    chatCancelHandle_.reset();
    history_.push_back({ChatRole::Error, "Request cancelled.", nullptr});
}

void LlmChatController::appendToolEventToHistory(const ToolCallEvent& event) {
    // Successful tool results are already phrased as a friendly,
    // user-facing sentence by the tool itself (see gui-tool-*-register.cpp)
    // -- no technical tool name needed. Registry-level failures (unknown
    // tool, timeout, bad arguments) are developer-facing edge cases, so
    // keep the name there for anyone trying to diagnose what went wrong.
    std::string text = event.success
        ? event.resultSummary
        : (event.toolName + ": " + event.resultSummary);
    if (text.empty()) {
        text = event.toolName;
    }
    history_.push_back({event.success ? ChatRole::Tool : ChatRole::Error, text, event.renderWidget});
}

void LlmChatController::applyTurnResult(const AgentTurnResult& result) {
    if (!result.success) {
        history_.push_back({ChatRole::Error, result.errorMessage, nullptr});
    } else if (!result.finalContent.empty()) {
        history_.push_back({ChatRole::Assistant, result.finalContent, nullptr});
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
    if (!model_.empty() && pingedModel_ != model_) {
        // Covers the chat's first-ever open: refreshModels() was kicked off right after
        // construction, and this is where its result first picks a default model.
        pingModel();
    }
}

void LlmChatController::applyPingResult(const ChatCompletionResult& result) {
    if (result.success) {
        pingStatus_ = PingStatus::Reachable;
        pingError_.clear();
    } else {
        pingStatus_ = PingStatus::Failed;
        pingError_ = result.errorMessage;
    }
}

void LlmChatController::setModel(std::string modelId) {
    if (modelId == model_) {
        return;
    }
    model_ = std::move(modelId);
    pingModel();
}

void LlmChatController::pingModel() {
    if (model_.empty()) {
        return;
    }
    if (pingCancelHandle_) {
        // Supersede any still-in-flight ping for a previous model -- both locally
        // (AsyncWorkerResult::start() below already orphans pingTask_) and on the wire, so
        // LM Studio doesn't keep generating an answer nobody will read (see stop()).
        pingCancelHandle_->cancel();
    }
    pingCancelHandle_ = std::make_shared<LmStudioCancelHandle>();
    pingedModel_ = model_;
    pingStatus_ = PingStatus::Pinging;
    pingError_.clear();

    ChatCompletionRequest request;
    request.model = model_;
    request.messages = {
        ChatMessage{.role = "system", .content = "Reply with one short word to confirm you are working."},
        ChatMessage{.role = "user", .content = "ping"}
    };

    auto client = client_;
    auto cancelHandle = pingCancelHandle_;
    pingTask_.start([client, request, cancelHandle]() {
        return client.chatCompletion(request, cancelHandle.get(), PING_TIMEOUT_MS);
    });
}

void LlmChatController::drainToolEvents() {
    if (!toolEvents_) {
        return;
    }

    std::vector<ToolCallEvent> newEvents;
    {
        std::scoped_lock lock(toolEvents_->mutex);
        if (toolEvents_->consumed < toolEvents_->events.size()) {
            newEvents.assign(
                toolEvents_->events.begin() + static_cast<std::ptrdiff_t>(toolEvents_->consumed),
                toolEvents_->events.end());
            toolEvents_->consumed = toolEvents_->events.size();
        }
    }
    for (const auto& event : newEvents) {
        appendToolEventToHistory(event);
    }
}

void LlmChatController::update() {
    drainToolEvents();

    if (chatTask_.isReady()) {
        applyTurnResult(chatTask_.consume());
        toolEvents_.reset();
        chatCancelHandle_.reset();
    }

    if (modelsTask_.isReady()) {
        applyModelsResult(modelsTask_.consume());
    }

    if (pingTask_.isReady()) {
        applyPingResult(pingTask_.consume());
        pingCancelHandle_.reset();
    }
}

} // namespace QaplaLlm
