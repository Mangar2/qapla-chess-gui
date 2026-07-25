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

#include <base-elements/qapla-json.h>

#include <chrono>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <vector>

namespace QaplaLlm {

/**
 * @brief Result of executing one GUI tool call.
 *
 * `content` is always sent back to the model as the tool's result message,
 * whether or not the action actually succeeded; `success` only affects how
 * the call is displayed locally in the chat (e.g. an error color).
 */
struct GuiToolResult {
    bool success = true;
    std::string content;
};

/**
 * @brief JSON Schema for a tool that takes no arguments: {"type":"object","properties":{}}.
 *
 * A bare "{}" (e.g. from QaplaTester::Json::JsonValue::object() alone) is
 * rejected by LM Studio's tool-schema validation ("Invalid discriminator
 * value. Expected 'object'") -- confirmed against a real server -- so this
 * is the correct default for GuiToolDefinition::parametersSchema, not just
 * a convenience.
 */
[[nodiscard]] inline QaplaTester::Json::JsonValue noArgsToolSchema() {
    auto schema = QaplaTester::Json::JsonValue::object();
    schema["type"] = "object";
    schema["properties"] = QaplaTester::Json::JsonValue::object();
    return schema;
}

/**
 * @brief One GUI-controllable action exposed to the LLM as a function tool.
 *
 * Handlers run exclusively on the UI thread (see GuiToolRegistry::processQueue()),
 * so they are free to touch GUI singletons exactly like the classic chatbot
 * steps do -- this is what guarantees an LLM action is immediately and
 * correctly visible in the GUI (see docs/llm-chatbot-plan.md).
 *
 * Template for a new tool group (see gui-tool-engine-management.* for a
 * worked example): split it across two files --
 * 1. gui-tool-<group>.h/.cpp: pure logic only (no os-dialogs.h, no
 *    configuration.h, no other ImGui/GLFW-adjacent include) so the
 *    unit-tests target can link and test it directly. Expose the handlers'
 *    actual work as free functions so tests can call them without going
 *    through a tool call at all.
 * 2. gui-tool-<group>-register.cpp: one registerXxxTools(GuiToolRegistry&)
 *    that builds each GuiToolDefinition and wires its handler to the pure
 *    functions from (1), plus whatever GUI singletons it actually needs to
 *    touch (Configuration, the window/window-data classes, ...). Only the
 *    qapla executable links this file. Call registerXxxTools() from
 *    registerGuiTools() in llm-chat-integration.cpp.
 * A tool's result content is dual-purpose -- sent to the model *and* shown
 * to the user (see ChatbotLlmChat) -- so write it as a short, friendly
 * sentence, never raw data dumps or internal identifiers.
 */
struct GuiToolDefinition {
    std::string name;
    std::string description;
    QaplaTester::Json::JsonValue parametersSchema = noArgsToolSchema(); ///< JSON Schema for "arguments".
    std::function<GuiToolResult(const QaplaTester::Json::JsonValue& arguments)> handler;

    /**
     * @brief How long callTool() waits for this tool's handler to run.
     *
     * Most tools finish within a frame or two, so the default is short.
     * Tools that block the UI thread on human interaction (e.g. a native
     * file dialog) must set a much longer timeout -- there is no way to
     * know in advance how long a person will take, and a short timeout
     * would just abandon the call while the handler is still legitimately
     * running, orphaning its (eventually correct) result.
     */
    std::chrono::milliseconds timeout = std::chrono::seconds(30);
};

/**
 * @brief Registry of GUI-controlling tools exposed to the LLM.
 *
 * Tool calls are requested from a worker thread (see LlmChatController) and
 * executed on the UI thread: callTool() enqueues the request and blocks the
 * calling thread only until processQueue() -- called once per frame from
 * the UI thread -- has run the handler, or until a timeout elapses.
 * Argument JSON parsing failures and handler exceptions are caught and
 * turned into a failed GuiToolResult rather than propagating.
 */
class GuiToolRegistry {
public:
    [[nodiscard]] static GuiToolRegistry& instance();

    /** @brief Registers a tool. Intended to be called once at startup, on the UI thread. */
    void registerTool(GuiToolDefinition tool);

    [[nodiscard]] bool hasTool(const std::string& name) const;

    /** @brief Tool specs in the wire-agnostic shape LmStudioClient turns into the OpenAI "tools" format. */
    [[nodiscard]] std::vector<ToolSpec> exportToolSpecs() const;

    /**
     * @brief Enqueues a tool call and blocks the calling thread until the UI
     * thread has executed it, or the tool's own timeout elapses.
     * @param name Registered tool name.
     * @param argumentsJson Raw JSON object text as received from the model (may be empty).
     */
    [[nodiscard]] GuiToolResult callTool(const std::string& name, const std::string& argumentsJson);

    /** @brief Must be called once per frame from the UI thread. */
    void processQueue();

private:
    struct QueuedCall {
        std::string name;
        std::string argumentsJson;
        std::promise<GuiToolResult> resultPromise;
    };

    mutable std::mutex toolsMutex_;
    std::vector<GuiToolDefinition> tools_;

    std::mutex queueMutex_;
    std::deque<QueuedCall> queue_;
};

} // namespace QaplaLlm
