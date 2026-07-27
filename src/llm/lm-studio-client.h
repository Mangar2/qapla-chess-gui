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

    /** @brief POST /v1/chat/completions (blocking, stream: false). */
    [[nodiscard]] ChatCompletionResult chatCompletion(const ChatCompletionRequest& request) const;

private:
    LmStudioConnection connection_;
};

} // namespace QaplaLlm
