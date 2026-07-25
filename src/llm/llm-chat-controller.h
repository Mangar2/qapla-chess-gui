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

/** @brief Outcome of one full agent-loop turn (see runAgentLoop() in the .cpp). */
struct AgentTurnResult {
    bool success = false;
    std::string finalContent;
    std::string errorMessage;
    std::vector<ToolCallEvent> toolEvents;
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
 * from the UI thread to pick up the turn's result.
 *
 * Thread-safety: all public methods (including update()) are meant to be
 * called from a single thread (the UI thread). The worker thread only ever
 * touches its own PendingChat/PendingModels state, exchanged with the UI
 * thread through a std::shared_ptr and a release/acquire "done" flag.
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
     * iteration limit), but its result is orphaned and never applied. Note
     * this does not abort an individual tool call already queued for the UI
     * thread -- e.g. a file dialog the model already asked to open will
     * still open even after stop() is called.
     */
    void stop();

    [[nodiscard]] const std::vector<ChatEntry>& history() const {
        return history_;
    }

    /**
     * @brief Appends a locally-generated status note to the history (e.g.
     * "Engine detection completed."), shown the same way as a Tool result.
     *
     * For events the UI layer observes outside the agent loop (e.g. a GUI
     * singleton's async state finishing after a tool call already
     * returned) -- see ChatbotLlmChat's engine-detection watcher. Like
     * other Tool entries, never replayed to the model.
     */
    void notify(const std::string& text) {
        history_.push_back({ChatRole::Tool, text});
    }

    /** @brief Must be called once per frame from the UI thread to pick up worker results. */
    void update();

private:
    void applyTurnResult(const AgentTurnResult& result);
    void applyModelsResult(const ListModelsResult& result);

    struct PendingChat {
        std::atomic<bool> done{false};
        AgentTurnResult result;
    };
    struct PendingModels {
        std::atomic<bool> done{false};
        ListModelsResult result;
    };

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
