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
 * @brief One message in an OpenAI-style chat completion request.
 */
struct ChatMessage {
    std::string role; // "system" | "user" | "assistant"
    std::string content;
};

/**
 * @brief A (non-streaming, tool-free) chat completion request.
 */
struct ChatCompletionRequest {
    std::string model;
    std::vector<ChatMessage> messages;
};

/**
 * @brief Result of a chat completion call.
 */
struct ChatCompletionResult {
    bool success = false;
    std::string content;      ///< Assistant reply text; valid only if success.
    std::string errorMessage; ///< Human-readable error; valid only if !success.
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
 * No tool/function-calling support yet (see docs/llm-chatbot-plan.md Step 3)
 * and no streaming (see Step 7). Calls block for the duration of the HTTP
 * request; callers that need the UI thread to stay responsive must run them
 * on a worker thread themselves (see LlmChatController).
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
