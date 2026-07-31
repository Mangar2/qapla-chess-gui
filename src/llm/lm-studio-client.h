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

#include <mutex>
#include <string>
#include <vector>

namespace QaplaLlm {

/**
 * @brief Connection parameters for talking to the local LM Studio server.
 */
struct LmStudioConnection {
    std::string host = "localhost";
    int port = 1234;
    int timeoutMs = 60000; // chat completions can legitimately take a while to generate
};

/**
 * @brief Cross-thread handle to abort an in-flight chatCompletion() call.
 *
 * The caller (see LlmChatController) keeps one of these alive for the duration of a request
 * and can call cancel() from a different thread -- e.g. the UI thread reacting to a Stop
 * click, or giving up on a stale request before starting a new one -- to make the blocked
 * HTTP call return early. This actually closes the TCP connection to LM Studio, rather than
 * just discarding the result locally once it eventually arrives: a server that notices a
 * disconnected client (as llama.cpp-based backends typically do) stops generating too,
 * instead of continuing to burn compute for an answer nobody will read.
 *
 * Deliberately holds an opaque `void*` rather than an `httplib::Client*` so this header (like
 * the rest of this file) stays free of the httplib dependency; the cast back happens only
 * inside lm-studio-client.cpp.
 */
class LmStudioCancelHandle {
public:
    LmStudioCancelHandle() = default;
    LmStudioCancelHandle(const LmStudioCancelHandle&) = delete;
    LmStudioCancelHandle& operator=(const LmStudioCancelHandle&) = delete;

    /** @brief Aborts the request currently using this handle, if any. Safe from any thread, any time. */
    void cancel();

private:
    friend class LmStudioClient;
    friend class CancelRegistration; ///< RAII attach/detach helper, defined in lm-studio-client.cpp
    std::mutex mutex_;
    void* activeClient_ = nullptr; ///< non-null (an httplib::Client*) only while a request is in flight
};

/**
 * @brief One function-tool call requested by the model, or echoed back to it.
 *
 * The wire format (JSON) is assembled/parsed entirely inside
 * lm-studio-client.cpp; argumentsJson is passed through verbatim so this
 * header stays free of any JSON library dependency.
 */
struct ToolCall {
    std::string id;
    std::string name;
    std::string argumentsJson; ///< Raw JSON object text, e.g. "{}" or "{\"path\":\"x\"}".
};

/**
 * @brief One message in an OpenAI-style chat completion request.
 *
 * - role "system" / "user": content only.
 * - role "assistant": content (may be empty) and, if the model requested
 *   tool calls, toolCalls -- both must be echoed back verbatim in a later
 *   request so the API can match up the following "tool" messages.
 * - role "tool": content is the tool's result text; toolCallId identifies
 *   which of the assistant's toolCalls this result answers.
 */
struct ChatMessage {
    std::string role;
    std::string content;
    std::vector<ToolCall> toolCalls{}; ///< Only meaningful for role == "assistant".
    std::string toolCallId{};          ///< Only meaningful for role == "tool".
};

/**
 * @brief Serializes one ChatMessage to its OpenAI wire-format JSON object text -- the exact
 * same representation chatCompletion() sends inside the request's "messages" array.
 *
 * Exposed (returning a plain string, not a JSON library type, to keep this header free of that
 * dependency -- see ToolCall's doc comment) so LlmFineTuningWriter can build dataset records
 * using the identical message shape rather than duplicating this serialization.
 */
[[nodiscard]] std::string chatMessageToJson(const ChatMessage& message);

/**
 * @brief A function tool offered to the model (OpenAI "tools" entry).
 *
 * parametersSchemaJson is the raw JSON Schema text for the tool's
 * "arguments" object (e.g. "{\"type\":\"object\",\"properties\":{}}");
 * building/validating that schema is GuiToolRegistry's job, not the
 * client's -- this struct just carries it over the wire.
 */
struct ToolSpec {
    std::string name;
    std::string description;
    std::string parametersSchemaJson;
};

/**
 * @brief A (non-streaming) chat completion request, optionally offering tools.
 */
struct ChatCompletionRequest {
    std::string model;
    std::vector<ChatMessage> messages;
    std::vector<ToolSpec> tools; ///< Empty => no "tools" field is sent at all.
};

/**
 * @brief Result of a chat completion call.
 */
struct ChatCompletionResult {
    bool success = false;
    std::string content;             ///< Assistant reply text; may be empty if toolCalls is non-empty.
    std::vector<ToolCall> toolCalls; ///< Non-empty if the model wants to call tools instead of replying.
    std::string errorMessage;        ///< Human-readable error; valid only if !success.
};

/**
 * @brief Result of a listModels() call.
 */
struct ListModelsResult {
    bool success = false;
    std::vector<std::string> modelIds;
    std::string errorMessage;
};

/**
 * @brief Blocking OpenAI-compatible HTTP client for the local LM Studio server.
 *
 * No streaming yet (see docs/llm-chatbot-plan.md Step 7). Calls block for
 * the duration of the HTTP request; callers that need the UI thread to stay
 * responsive must run them on a worker thread themselves (see
 * LlmChatController).
 */
class LmStudioClient {
public:
    explicit LmStudioClient(LmStudioConnection connection = {});

    /** @brief GET /v1/models. */
    [[nodiscard]] ListModelsResult listModels() const;

    /**
     * @brief POST /v1/chat/completions (blocking, stream: false).
     * @param cancelHandle If non-null, registered for the duration of this call so
     *        cancelHandle->cancel() (from another thread) can abort it early. See
     *        LmStudioCancelHandle.
     * @param timeoutMsOverride If > 0, used instead of the connection's configured
     *        timeoutMs for this one call -- e.g. a longer timeout for a model's first,
     *        possibly cold-load-triggering contact than for routine follow-up turns.
     */
    [[nodiscard]] ChatCompletionResult chatCompletion(
        const ChatCompletionRequest& request,
        LmStudioCancelHandle* cancelHandle = nullptr,
        int timeoutMsOverride = 0) const;

private:
    LmStudioConnection connection_;
};

} // namespace QaplaLlm
