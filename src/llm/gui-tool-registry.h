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

    /**
     * @brief Optional: renders a live GUI control in the chat history in place of `content`'s
     * plain text (e.g. the same ImGuiTable a classic, non-AI chatbot step would draw).
     *
     * Set this from a tool handler whose whole point is to *show* something -- as opposed to a
     * tool that merely reports facts as text for the model to relay -- so the result appears in
     * the chat as a real control, not a text dump. Built on the UI thread inside the handler (see
     * GuiToolRegistry::processQueue()), it is then carried across the worker thread that drives
     * the agent loop as an inert std::function (never invoked there) and finally invoked back on
     * the UI thread each frame while its ChatEntry stays visible in the chat history -- so it's
     * safe to capture and call other UI-thread-only state (singletons, ImGui calls) exactly as a
     * normal draw() method would. Keep `content` short in this case (e.g. "Showing the current
     * tournament results.") since the control itself carries the actual data.
     */
    std::function<void()> renderWidget{};

    /**
     * @brief Ends the agent turn right after this call, skipping any further model round-trip.
     *
     * `content` still reaches the user exactly as usual, via the normal tool-result chat entry
     * (see LlmChatController::appendToolEventToHistory) -- but the model is never asked again,
     * so it never gets a chance to add a reply_to_user narrating/paraphrasing what just
     * happened. Use for actions whose outcome must never be commented on (e.g. a file dialog
     * result): telling the model not to comment, even worded as a hard prohibition in both the
     * system prompt and the tool result itself, was not reliable -- removing it from the loop
     * is.
     */
    bool terminal = false;
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
 * @brief Who is calling, for the few tools and parameters that are not for everyone.
 *
 * The distinction is not about trust -- the remote control listens on loopback only -- but about
 * what makes sense at each end. Anything that opens a modal file dialog waits for a person who is
 * sitting in front of the window; from outside it would hang the caller on something it cannot
 * see and cannot answer. Closing the application ends the very thing an outside caller asked to
 * watch, and takes its own channel down with it.
 */
enum class CallOrigin {
    /** @brief The chat inside the window, where a user is present to answer a dialog. */
    Local,

    /** @brief Over the remote control, where nobody is watching the caller's end. */
    Remote
};

/**
 * @brief One GUI-controllable action exposed to the LLM as a function tool.
 *
 * Handlers run exclusively on the UI thread (see GuiToolRegistry::processQueue()),
 * so they are free to touch GUI singletons exactly like the classic chatbot
 * steps do -- this is what guarantees an LLM action is immediately and
 * correctly visible in the GUI (see docs/llm-chatbot-plan.md).
 *
 * This is the wire-level shape a tool ends up in, not the shape tools are *written* in: they are
 * declared as data with Api::defineTool() in src/llm/tools/, which builds the schema and the
 * argument reader from one parameter list and calls an action in src/llm/actions/. Register a new
 * one there rather than constructing a GuiToolDefinition by hand -- see src/llm/tools/gui-tools.h
 * for why the two sides are kept apart.
 *
 * A tool's result content is dual-purpose -- sent to the model *and* shown
 * to the user (see ChatbotLlmChat) -- so write it as a short, friendly
 * sentence, never raw data dumps or internal identifiers.
 */
// Every member below carries a default, including the ones a default already gives. Callers
// build these with designated initializers and name only the fields they care about, which is
// what -Wmissing-field-initializers complains about unless the omitted member has an initializer
// of its own. Spelling out the empty ones keeps that idiom warning-free at the source, rather
// than making every call site list fields it has nothing to say about.
struct GuiToolDefinition {
    std::string name{};
    std::string description{};
    QaplaTester::Json::JsonValue parametersSchema = noArgsToolSchema(); ///< JSON Schema for "arguments".
    std::function<GuiToolResult(const QaplaTester::Json::JsonValue& arguments)> handler{};

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

    /** @brief Not offered to, and not callable by, a CallOrigin::Remote caller. */
    bool localOnly = false;

    /**
     * @brief Parameter names withheld from a remote caller: absent from the published schema,
     * and rejected if passed anyway.
     *
     * Rejected rather than quietly dropped. A caller that asked for a file dialog wants a file
     * chosen; carrying on without one would apply the rest of its patch and report success for a
     * request that was not carried out.
     */
    std::vector<std::string> localOnlyParameters{};
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

    /**
     * @brief Tool specs in the wire-agnostic shape LmStudioClient turns into the OpenAI "tools"
     * format.
     * @param origin Drops the tools and parameters that origin may not use (see CallOrigin).
     */
    [[nodiscard]] std::vector<ToolSpec> exportToolSpecs(
        CallOrigin origin = CallOrigin::Local) const;

    /**
     * @brief Enqueues a tool call and blocks the calling thread until the UI
     * thread has executed it, or the tool's own timeout elapses.
     * @param name Registered tool name.
     * @param argumentsJson Raw JSON object text as received from the model (may be empty).
     * @param origin Refused before reaching the UI thread if this origin may not make the call.
     */
    [[nodiscard]] GuiToolResult callTool(const std::string& name, const std::string& argumentsJson,
        CallOrigin origin = CallOrigin::Local);

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
