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
#include <json/json_pretty_print.h>

#include <format>
#include <functional>
#include <utility>

namespace QaplaLlm {

namespace {

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
                "Reply to user. ONLY way to say anything to them -- never plain text, always "
                "this tool, even short confirmation/question/acknowledgement. Full answer in "
                "\"text\" arg only -- anything outside \"text\" discarded, never shown. Never "
                "split answer or use placeholder here+real answer elsewhere. If a tool already "
                "displays its result in chat (e.g. show_result), don't repeat that data -- just "
                "a short confirmation sentence.",
            .parametersSchemaJson =
                R"({"type":"object","properties":{"text":{"type":"string",)"
                R"("description":"Complete message to user, in their language -- only part )"
                R"(they see."}},)"
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

    // [system, ...this turn's own exchange] -- see turnStartIndex's doc comment in
    // runAgentLoop for why the rest of the (unrelated, earlier-turn) messages are skipped.
    std::vector<ChatMessage> sliceFineTuningMessages(
        const std::vector<ChatMessage>& messages, std::size_t turnStartIndex) {
        std::vector<ChatMessage> result;
        result.push_back(messages.front()); // system prompt
        result.insert(result.end(), messages.begin() + static_cast<std::ptrdiff_t>(turnStartIndex), messages.end());
        return result;
    }

    // Builds the reply_to_user tool call the model *should* have made instead of answering
    // with plain text -- used to correct a fine-tuning record's final message (never the live
    // chat/log, which stay faithful to what actually happened) so the dataset only ever
    // demonstrates the structured form we're trying to train the model into using.
    ChatMessage makeSyntheticReplyMessage(const std::string& text) {
        ChatMessage message;
        message.role = "assistant";
        ToolCall call;
        call.id = "synthetic_reply"; // recognizable marker; never sent to a real API
        call.name = REPLY_TOOL_NAME;
        auto argumentsJson = QaplaTester::Json::JsonValue::object();
        argumentsJson["text"] = text;
        call.argumentsJson = argumentsJson.stringify();
        message.toolCalls.push_back(std::move(call));
        return message;
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
        int messageTimeoutMs,
        bool enableThinking,
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

        // Tracks the single most recently *executed* real tool call (name + raw arguments, as
        // sent by the model) across the whole turn, iterations included, so a call that's an
        // exact repeat of the one immediately before it -- e.g. the model calling show_result
        // for the same type twice in a row -- can be answered from the cached result instead of
        // re-running it (side-effect-free to repeat, like re-showing a result table, or actively
        // wrong to repeat, like re-starting a tournament). Any other call in between (including
        // reply_to_user, which ends the turn) breaks the "directly consecutive" chain naturally,
        // since the turn returns before another iteration can compare against it.
        std::string lastToolCallName;
        std::string lastToolCallArgs;
        GuiToolResult lastToolResult;
        bool hasLastToolResult = false;
        // How many times in a row (including the original, real execution) the same
        // name+arguments pair has now been seen; reset to 1 whenever a call differs from the
        // one right before it. Once this goes past kMaxIdenticalCallRepeats, the model is
        // treated as stuck in a loop rather than legitimately repeating a side-effect-free call
        // -- see the abort below.
        int identicalCallStreak = 0;
        constexpr int kMaxIdenticalCallRepeats = 3;

        for (int iteration = 0; iteration < maxToolIterations; ++iteration) {
            ChatCompletionRequest request;
            request.model = model;
            request.messages = messages;
            request.tools = tools;
            request.enableThinking = enableThinking;

            auto response = client.chatCompletion(request, cancelHandle, messageTimeoutMs);
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

                if (turnClean) {
                    // Every real tool call earlier in this turn (if any) still succeeded --
                    // worth keeping as a fine-tuning example anyway, just corrected to show the
                    // reply_to_user call the model *should* have made instead of literally
                    // replaying the plain-text fallback we're actively trying to eliminate (see
                    // the system prompt and this tool's own description). The live chat/log
                    // above are untouched and keep showing what actually happened.
                    auto fineTuningMessages = sliceFineTuningMessages(messages, turnStartIndex);
                    fineTuningMessages.push_back(makeSyntheticReplyMessage(response.content));
                    fineTuningWriter->appendTurn(fineTuningMessages);
                }
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

                const bool isDuplicate = hasLastToolResult && call.name == lastToolCallName
                    && call.argumentsJson == lastToolCallArgs;
                identicalCallStreak = isDuplicate ? identicalCallStreak + 1 : 1;

                if (isDuplicate && identicalCallStreak > kMaxIdenticalCallRepeats) {
                    // The model is calling the exact same tool with the exact same arguments
                    // over and over -- beyond a couple of legitimate repeats (handled below by
                    // the plain dedupe) this stops looking like a deliberate repeat and starts
                    // looking like a stuck loop, so give up on the turn instead of feeding the
                    // model its own repeated result forever.
                    logger->logLine("TOOL_CALL_ABORT",
                        call.name + " args=" + call.argumentsJson + " repeated "
                            + std::to_string(identicalCallStreak) + " times in a row");
                    onToolEvent(ToolCallEvent{
                        .resultSummary = "Debug: \"" + call.name + "\" was called with identical "
                            "arguments more than " + std::to_string(kMaxIdenticalCallRepeats)
                            + " times in a row; aborting this turn.",
                        .debug = true
                    });
                    turnResult.success = false;
                    turnResult.errorMessage = "Aborted: \"" + call.name
                        + "\" was called identically more than " + std::to_string(kMaxIdenticalCallRepeats)
                        + " times in a row.";
                    return turnResult;
                }

                GuiToolResult toolResult;
                if (isDuplicate) {
                    // Same tool, same raw arguments, directly following the call that produced
                    // lastToolResult -- reuse it instead of running the action again (see the
                    // lastToolCallName/-Args doc comment above the iteration loop).
                    toolResult = lastToolResult;
                    logger->logLine("TOOL_CALL_DEDUPED", call.name + " args=" + call.argumentsJson);
                    onToolEvent(ToolCallEvent{
                        .resultSummary = "Debug: duplicate call to \"" + call.name
                            + "\" with identical arguments suppressed; reusing the previous result.",
                        .debug = true
                    });
                } else {
                    logger->logLine("TOOL_CALL", call.name + " args=" + call.argumentsJson);
                    toolResult = GuiToolRegistry::instance().callTool(call.name, call.argumentsJson);
                    logger->logLine(toolResult.success ? "TOOL_RESULT" : "TOOL_ERROR", toolResult.content);
                }
                if (!toolResult.success) {
                    turnClean = false;
                }
                lastToolCallName = call.name;
                lastToolCallArgs = call.argumentsJson;
                lastToolResult = toolResult;
                hasLastToolResult = true;

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

                if (toolResult.terminal) {
                    // Ends the turn without asking the model again -- see GuiToolResult::terminal.
                    // finalContent stays empty on purpose: the tool result above already reached
                    // the user via onToolEvent, applyTurnResult only adds a further chat entry
                    // when finalContent is non-empty.
                    turnResult.success = true;
                }
            }

            if (turnResult.success) {
                if (turnClean) {
                    fineTuningWriter->appendTurn(sliceFineTuningMessages(messages, turnStartIndex));
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
    std::string logDirectory, int messageTimeoutMs)
    : client_(std::move(connection)), systemPrompt_(std::move(systemPrompt)), maxToolIterations_(maxToolIterations),
      messageTimeoutMs_(messageTimeoutMs),
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

    // A real message supersedes the background reachability ping (see pingModel()): cancel it
    // on the wire immediately, and discard its result locally so a ping cut off mid-flight
    // can't later land as a misleading "model did not respond" once the real request is what
    // matters -- the real request is about to prove reachability far better than the ping ever
    // could.
    if (pingCancelHandle_) {
        pingCancelHandle_->cancel();
        pingCancelHandle_.reset();
        pingTask_.reset();
        pingStatus_ = PingStatus::Reachable;
        pingError_.clear();
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

    // Rough prompt-size estimate for diagnosing "context too long" server errors: the full
    // tool list and flattened history are resent on every turn (see runAgentLoop's
    // request.tools = tools inside its retry loop), so a model loaded with a small context
    // window can hit its limit even on an early message. There's no local tokenizer for
    // whatever model happens to be loaded, so this uses the widely-used ~4-characters-per-token
    // heuristic instead of an exact count -- not precise, but enough to tell "close to the
    // limit" from "nowhere near it", and to compare against LM Studio's configured context size.
    {
        std::size_t messagesChars = 0;
        for (const auto& message : messages) {
            messagesChars += chatMessageToJson(message).size();
        }
        std::string toolsJsonCompact = toolSpecsToJson(tools);
        std::size_t totalChars = messagesChars + toolsJsonCompact.size();
        history_.push_back({
            ChatRole::Debug,
            std::format(
                "Debug: request size ~{} chars ≈ {} tokens (~4 chars/token estimate) -- "
                "messages {} chars, tools {} chars across {} tools.",
                totalChars, totalChars / 4, messagesChars, toolsJsonCompact.size(), tools.size()),
            nullptr
        });

        // Logged once (see logSystemPromptAndToolsOnce's doc comment) rather than on every
        // turn: system prompt and tools are resent unchanged on every request, so repeating
        // this ~30KB block every turn would just bloat the log file with identical text.
        logger_->logSystemPromptAndToolsOnce(
            systemPrompt_,
            QaplaTester::Json::stringify_pretty(QaplaTester::Json::JsonValue::parse(toolsJsonCompact)));
    }

    auto maxToolIterations = maxToolIterations_;
    auto messageTimeoutMs = messageTimeoutMs_;
    auto enableThinking = enableThinking_;
    auto cancelHandle = chatCancelHandle_;
    auto logger = logger_;
    auto fineTuningWriter = fineTuningWriter_;

    chatTask_.start([client, model, tools, messages = std::move(messages), maxToolIterations, messageTimeoutMs,
                      enableThinking, events, cancelHandle, logger, fineTuningWriter]() mutable {
        auto onToolEvent = [events](ToolCallEvent event) {
            std::scoped_lock lock(events->mutex);
            events->events.push_back(std::move(event));
        };
        return runAgentLoop(
            client, model, tools, std::move(messages), maxToolIterations, messageTimeoutMs, enableThinking,
            cancelHandle.get(), logger.get(), fineTuningWriter.get(), onToolEvent);
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
    if (event.debug) {
        history_.push_back({ChatRole::Debug, event.resultSummary, nullptr});
        return;
    }

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

    // Deliberately just "Hi", no system message explaining what to answer -- a "reply briefly"
    // instruction sent some reasoning models into thousands of tokens of thinking about what
    // "briefly" even means before ever answering, defeating the point of a cheap reachability
    // probe. This only needs *any* reply at all, not a specific one.
    ChatCompletionRequest request;
    request.model = model_;
    request.messages = {
        ChatMessage{.role = "user", .content = "Hi"}
    };
    request.enableThinking = enableThinking_;

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
