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

#include "async-worker-result.h"
#include "llm-chat-logger.h"
#include "llm-finetuning-writer.h"
#include "lm-studio-client.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace QaplaLlm {

/**
 * @brief Role of one entry in the locally displayed chat history.
 */
enum class ChatRole : std::uint8_t {
    User,
    Assistant,
    Tool,  ///< A tool's own friendly result text, shown inline; never replayed to the model.
    Error  ///< Local/network/protocol error shown inline; never replayed to the model.
};

struct ChatEntry {
    ChatRole role;
    std::string text;

    /** @brief See GuiToolResult::renderWidget. Set only on some ChatRole::Tool entries. */
    std::function<void()> renderWidget;
};

/** @brief One tool call executed within an agent-loop turn, for local display. */
struct ToolCallEvent {
    std::string toolName;
    bool success = true;
    std::string resultSummary;

    /** @brief See GuiToolResult::renderWidget; carried through unchanged. */
    std::function<void()> renderWidget;
};

/** @brief The model's final answer for one agent-loop turn (tool events are streamed separately, see ToolEventChannel). */
struct AgentTurnResult {
    bool success = false;
    std::string finalContent;
    std::string errorMessage;
};

/**
 * @brief Drives a tool-calling conversation with a local LM Studio model.
 *
 * Each user message starts one detached worker thread (via chatTask_, an
 * AsyncWorkerResult) that runs the full agent loop (see
 * docs/llm-chatbot-plan.md Step 3): call the model, and as long as it
 * responds with tool_calls instead of a final answer, execute each one
 * through GuiToolRegistry (which itself hops back to the UI thread and
 * blocks the worker until done), feed the results back, and ask again --
 * up to maxToolIterations times. update() must be called once per frame
 * from the UI thread to pick up results.
 *
 * Tool results are visible to the user as soon as they happen -- e.g. once
 * an engine is added and detected, that already shows both in the chat and
 * in the GUI's engine list, independently of how long the model then takes
 * to compose its own reply -- rather than being held back and only shown
 * together with the model's (often much slower) final answer. This is why
 * a turn's tool events travel over their own toolEvents_ channel instead of
 * riding along inside AgentTurnResult: chatTask_ only becomes ready once
 * the whole turn (all roundtrips) has finished, but tool events must reach
 * history() as each one happens, mid-turn.
 *
 * Thread-safety: all public methods (including update()) are meant to be
 * called from a single thread (the UI thread). The worker thread only ever
 * touches AsyncWorkerResult's internal state (see async-worker-result.h)
 * and ToolEventChannel, both exchanged with the UI thread through a
 * std::shared_ptr.
 */
class LlmChatController {
public:
    /** @brief Reachability of the currently selected model, probed by pingModel(). */
    enum class PingStatus : std::uint8_t {
        NotStarted, ///< No model selected yet, or pingModel() hasn't run for it.
        Pinging,
        Reachable,
        Failed
    };

    /**
     * @param logTraffic If true, the full conversation (user messages, structured model
     *        replies incl. tool errors, connection errors, and capped unstructured output) is
     *        appended to a timestamped file in logDirectory -- see LlmChatLogger. Defaults to
     *        false so constructing a controller in a test never writes into the real user
     *        config directory; the GUI integration passes the Settings checkbox's value
     *        explicitly (see ChatbotLlmChat::ensureController()).
     * @param logDirectory Directory both LlmChatLogger's (if logTraffic) and
     *        LlmFineTuningWriter's files are written to. Empty (the default) means the app's
     *        own config directory; overridable so tests never touch the real one. The
     *        fine-tuning file is always written regardless of logTraffic -- see
     *        LlmFineTuningWriter's class docs for why it has no such toggle.
     */
    LlmChatController(
        LmStudioConnection connection, std::string systemPrompt, int maxToolIterations = 10, bool logTraffic = false,
        std::string logDirectory = "");

    /** @brief Starts a non-blocking model list refresh. No-op if already in flight. */
    void refreshModels();
    [[nodiscard]] bool isRefreshingModels() const {
        return modelsTask_.isRunning();
    }
    [[nodiscard]] const std::vector<std::string>& availableModels() const {
        return availableModels_;
    }
    [[nodiscard]] const std::string& modelsError() const {
        return modelsError_;
    }

    /** @brief Switches models and, if the model actually changed, re-probes reachability (see pingModel()). */
    void setModel(std::string modelId);
    [[nodiscard]] const std::string& model() const {
        return model_;
    }

    /** @brief Reachability state of model(), as last probed by pingModel(). */
    [[nodiscard]] PingStatus pingStatus() const {
        return pingStatus_;
    }
    /** @brief Set once pingStatus() == Failed; a human-readable reason (e.g. a timeout explanation). */
    [[nodiscard]] const std::string& pingError() const {
        return pingError_;
    }

    /** @brief Appends the user's message and starts the agent loop. No-op if isBusy() or text is empty. */
    void sendMessage(const std::string& userText);

    /** @brief True while a turn (model calls and any tool calls within it) is in flight. */
    [[nodiscard]] bool isBusy() const {
        return chatTask_.isRunning();
    }

    /**
     * @brief Abandons the in-flight turn and tells LM Studio to stop generating it.
     *
     * Actively cancels the in-flight HTTP request (see LmStudioCancelHandle) rather than
     * just discarding the result locally, so the server also notices and stops burning
     * compute on an answer nobody will read. The worker thread itself unblocks once the
     * cancelled call returns, but its result is orphaned and never applied. Any tool results
     * already streamed into history() before stop() was called remain -- their GUI-visible
     * effects (e.g. an added engine) already happened and can't be undone anyway. Note this
     * does not abort an individual tool call already queued for the UI thread -- e.g. a file
     * dialog the model already asked to open will still open even after stop() is called.
     */
    void stop();

    [[nodiscard]] const std::vector<ChatEntry>& history() const {
        return history_;
    }

    /** @brief Must be called once per frame from the UI thread to pick up worker results. */
    void update();

private:
    /** @brief Multi-value counterpart to AsyncWorkerResult: the worker appends as it goes, the UI thread drains. */
    struct ToolEventChannel {
        std::mutex mutex;
        std::vector<ToolCallEvent> events;
        std::size_t consumed = 0;
    };

    void appendToolEventToHistory(const ToolCallEvent& event);
    void applyTurnResult(const AgentTurnResult& result);
    void applyModelsResult(const ListModelsResult& result);
    void applyPingResult(const ChatCompletionResult& result);
    void drainToolEvents();

    /**
     * @brief Probes whether model() actually answers, not just whether the server is up.
     *
     * Triggered automatically by setModel() and by applyModelsResult() picking a default
     * model, i.e. whenever a new model is selected or the chat is opened for the first time
     * (see class docs) -- never needs to be called directly. Uses a longer timeout than a
     * regular turn (PING_TIMEOUT_MS, see .cpp) since first contact with a model LM Studio
     * hasn't loaded into memory yet can take a while. Supersedes (and cancels on the wire,
     * see LmStudioCancelHandle) any still-in-flight ping for a previous model.
     */
    void pingModel();

    LmStudioClient client_;
    std::string systemPrompt_;
    std::string model_;
    int maxToolIterations_;
    std::vector<ChatEntry> history_;
    std::shared_ptr<LlmChatLogger> logger_;
    std::shared_ptr<LlmFineTuningWriter> fineTuningWriter_;

    AsyncWorkerResult<AgentTurnResult> chatTask_;
    std::shared_ptr<ToolEventChannel> toolEvents_; ///< Non-null exactly while chatTask_.isRunning().
    std::shared_ptr<LmStudioCancelHandle> chatCancelHandle_; ///< Lets stop() cancel the in-flight request on the wire.

    AsyncWorkerResult<ListModelsResult> modelsTask_;
    std::vector<std::string> availableModels_;
    std::string modelsError_;

    AsyncWorkerResult<ChatCompletionResult> pingTask_;
    std::shared_ptr<LmStudioCancelHandle> pingCancelHandle_;
    std::string pingedModel_;      ///< Which model pingStatus_/pingTask_ refer to.
    PingStatus pingStatus_ = PingStatus::NotStarted;
    std::string pingError_;
};

} // namespace QaplaLlm
