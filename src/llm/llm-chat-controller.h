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
#include <mutex>
#include <string>
#include <vector>

namespace QaplaLlm {

/**
 * @brief Role of one entry in the locally displayed chat history.
 */
enum class ChatRole {
    User,
    Assistant,
    Tool,  ///< A tool's own friendly result text, shown inline; never replayed to the model.
    Error  ///< Local/network/protocol error shown inline; never replayed to the model.
};

struct ChatEntry {
    ChatRole role;
    std::string text;
};

/** @brief One tool call executed within an agent-loop turn, for local display. */
struct ToolCallEvent {
    std::string toolName;
    bool success = true;
    std::string resultSummary;
};

/** @brief The model's final answer for one agent-loop turn (tool events are streamed separately, see PendingChat). */
struct AgentTurnResult {
    bool success = false;
    std::string finalContent;
    std::string errorMessage;
};

/**
 * @brief Drives a tool-calling conversation with a local LM Studio model.
 *
 * Each user message starts one detached worker thread that runs the full
 * agent loop (see docs/llm-chatbot-plan.md Step 3): call the model, and as
 * long as it responds with tool_calls instead of a final answer, execute
 * each one through GuiToolRegistry (which itself hops back to the UI thread
 * and blocks the worker until done), feed the results back, and ask again --
 * up to maxToolIterations times. update() must be called once per frame
 * from the UI thread to pick up results.
 *
 * Tool results are visible to the user as soon as they happen -- e.g. once
 * an engine is added and detected, that already shows both in the chat and
 * in the GUI's engine list, independently of how long the model then takes
 * to compose its own reply -- rather than being held back and only shown
 * together with the model's (often much slower) final answer.
 *
 * Thread-safety: all public methods (including update()) are meant to be
 * called from a single thread (the UI thread). The worker thread only ever
 * touches its own PendingChat/PendingModels state, exchanged with the UI
 * thread through a std::shared_ptr; PendingChat::events is additionally
 * guarded by its own mutex since, unlike the rest of the exchanged state,
 * it is written incrementally over the turn's whole lifetime rather than
 * once at the end.
 */
class LlmChatController {
public:
    LlmChatController(LmStudioConnection connection, std::string systemPrompt, int maxToolIterations = 10);

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

    /** @brief Appends the user's message and starts the agent loop. No-op if isBusy() or text is empty. */
    void sendMessage(const std::string& userText);

    /** @brief True while a turn (model calls and any tool calls within it) is in flight. */
    [[nodiscard]] bool isBusy() const {
        return busy_;
    }

    /**
     * @brief Abandons the in-flight turn.
     *
     * Best-effort: the worker thread keeps running to completion in the
     * background (bounded by the connection's request timeout and the tool
     * iteration limit), but its final answer is orphaned and never applied.
     * Any tool results already streamed into history() before stop() was
     * called remain -- their GUI-visible effects (e.g. an added engine)
     * already happened and can't be undone anyway. Note this does not
     * abort an individual tool call already queued for the UI thread --
     * e.g. a file dialog the model already asked to open will still open
     * even after stop() is called.
     */
    void stop();

    [[nodiscard]] const std::vector<ChatEntry>& history() const {
        return history_;
    }

    /** @brief Must be called once per frame from the UI thread to pick up worker results. */
    void update();

private:
    void appendToolEventToHistory(const ToolCallEvent& event);
    void applyTurnResult(const AgentTurnResult& result);
    void applyModelsResult(const ListModelsResult& result);

    struct PendingChat {
        std::mutex eventsMutex;
        std::vector<ToolCallEvent> events; ///< Appended incrementally by the worker as each tool call finishes.
        std::size_t consumedEvents = 0;    ///< UI-thread-only cursor into events, guarded by eventsMutex too.
        std::atomic<bool> done{false};
        AgentTurnResult result;
    };
    struct PendingModels {
        std::atomic<bool> done{false};
        ListModelsResult result;
    };

    /** @brief Drains and applies any tool events the worker has published so far, without waiting for done. */
    void drainToolEvents(PendingChat& pending);

    LmStudioClient client_;
    std::string systemPrompt_;
    std::string model_;
    int maxToolIterations_;
    std::vector<ChatEntry> history_;

    bool busy_ = false;
    std::shared_ptr<PendingChat> pendingChat_;

    bool refreshingModels_ = false;
    std::shared_ptr<PendingModels> pendingModels_;
    std::vector<std::string> availableModels_;
    std::string modelsError_;
};

} // namespace QaplaLlm
