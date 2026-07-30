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
    std::function<void()> renderWidget;
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
 * @brief JSON Schema property for a chess clock time control string.
 *
 * Shared across every tool group that lets the model configure a time
 * control (tournaments today, SPRT/EPD flows later) so the format and its
 * worked examples only need to be written -- and get updated -- once.
 * QaplaTester::TimeControl::parse() (the thing that actually consumes this
 * string) is deliberately lenient and never throws, so nothing here is
 * enforced beyond what the model is told; the description IS the contract.
 */
[[nodiscard]] inline QaplaTester::Json::JsonValue timeControlSchemaProperty() {
    auto prop = QaplaTester::Json::JsonValue::object();
    prop["type"] = "string";
    prop["description"] =
        "Chess clock time control. Format: \"<base>+<increment>\", both in "
        "seconds; increment is optional and defaults to 0. <base> may also "
        "be \"M:S\" or \"H:M:S\". Prefix with \"<moves>/\" for a moves-per-session "
        "phase (e.g. \"40/300+2\" = 40 moves in 300s, then +2s per move "
        "thereafter). Use \"inf\" for no time limit. "
        "Always convert the user's own words into this format yourself -- "
        "never ask the user to supply an already-formatted time control "
        "string. Examples: \"1 minute per game\" -> \"60+0\" (or \"1:00\"); "
        "\"5 minutes plus 3 second increment\" / \"5+3\" -> \"300+3\" (or "
        "\"5:00+3\"); \"40 moves in 5 minutes, then 2 seconds per move\" -> "
        "\"40/300+2\"; \"blitz\" (no further detail given) -> \"300+0\"; "
        "\"unlimited\"/\"no clock\" -> \"inf\".";
    return prop;
}

/**
 * @brief JSON Schema property for the off/test/active tri-state adjudication uses (see
 * ImGuiControls::triStateInput() -- "off" (never adjudicate), "test" (evaluate and log what it
 * would have decided, without ending games), "active" (actually adjudicate). Shared between the
 * tournament and SPRT adjudication tool groups so the vocabulary and its mapping to the
 * underlying active/testOnly bool pair only need to be written once.
 */
[[nodiscard]] inline QaplaTester::Json::JsonValue adjudicationModeSchemaProperty(const std::string& description) {
    auto prop = QaplaTester::Json::JsonValue::object();
    prop["type"] = "string";
    auto enumValues = QaplaTester::Json::JsonValue::array();
    enumValues.push_back("off");
    enumValues.push_back("test");
    enumValues.push_back("active");
    prop["enum"] = enumValues;
    prop["description"] = description;
    return prop;
}

/**
 * @brief Maps an adjudicationModeSchemaProperty() string to the underlying active/testOnly pair.
 * @return true if `mode` was recognized (active/testOnly were updated); false if not (a
 * human-readable problem was appended to `problems`, and active/testOnly are left untouched).
 */
[[nodiscard]] inline bool applyAdjudicationMode(
    const std::string& mode, bool& active, bool& testOnly, std::vector<std::string>& problems) {
    if (mode == "off") {
        active = false;
        testOnly = false;
    } else if (mode == "test") {
        active = true;
        testOnly = true;
    } else if (mode == "active") {
        active = true;
        testOnly = false;
    } else {
        problems.push_back("mode must be \"off\", \"test\", or \"active\" (got \"" + mode + "\")");
        return false;
    }
    return true;
}

/**
 * @brief Formats an off/test/active mode back to the same vocabulary applyAdjudicationMode() accepts.
 */
[[nodiscard]] inline std::string adjudicationModeText(bool active, bool testOnly) {
    if (!active) {
        return "off";
    }
    return testOnly ? "test" : "active";
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
