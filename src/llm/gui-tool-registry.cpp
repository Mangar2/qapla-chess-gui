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

#include "gui-tool-registry.h"

#include <algorithm>

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;
}

GuiToolRegistry& GuiToolRegistry::instance() {
    static GuiToolRegistry registry;
    return registry;
}

void GuiToolRegistry::registerTool(GuiToolDefinition tool) {
    std::scoped_lock lock(toolsMutex_);
    tools_.push_back(std::move(tool));
}

bool GuiToolRegistry::hasTool(const std::string& name) const {
    std::scoped_lock lock(toolsMutex_);
    return std::ranges::any_of(tools_, [&](const auto& tool) { return tool.name == name; });
}

namespace {
    /** @brief The tool's schema with the parameters this origin may not use removed. */
    [[nodiscard]] Json::JsonValue schemaFor(const GuiToolDefinition& tool, CallOrigin origin) {
        if (origin == CallOrigin::Local || tool.localOnlyParameters.empty()
            || !tool.parametersSchema.contains("properties")) {
            return tool.parametersSchema;
        }
        auto schema = tool.parametersSchema;
        auto properties = Json::JsonValue::object();
        for (const auto& [name, value] : schema["properties"].as_object()) {
            if (std::ranges::find(tool.localOnlyParameters, name) == tool.localOnlyParameters.end()) {
                properties[name] = value;
            }
        }
        schema["properties"] = properties;
        return schema;
    }

    /** @brief The withheld parameter names a caller passed anyway, if any. */
    [[nodiscard]] std::vector<std::string> withheldParametersUsed(
        const GuiToolDefinition& tool, const std::string& argumentsJson) {
        std::vector<std::string> used;
        if (tool.localOnlyParameters.empty() || argumentsJson.empty()) {
            return used;
        }
        auto parsed = Json::JsonValue::try_parse(argumentsJson);
        if (!parsed || !parsed->is_object()) {
            return used;
        }
        for (const auto& name : tool.localOnlyParameters) {
            if (parsed->contains(name) && !parsed->at(name).is_null()) {
                used.push_back(name);
            }
        }
        return used;
    }
} // namespace

std::vector<ToolSpec> GuiToolRegistry::exportToolSpecs(CallOrigin origin) const {
    std::scoped_lock lock(toolsMutex_);
    std::vector<ToolSpec> specs;
    specs.reserve(tools_.size());
    for (const auto& tool : tools_) {
        if (tool.localOnly && origin == CallOrigin::Remote) {
            continue;
        }
        specs.push_back(ToolSpec{
            .name = tool.name,
            .description = tool.description,
            .parametersSchemaJson = schemaFor(tool, origin).stringify()
        });
    }
    return specs;
}

GuiToolResult GuiToolRegistry::callTool(const std::string& name, const std::string& argumentsJson,
    CallOrigin origin) {
    std::chrono::milliseconds timeout;
    {
        std::scoped_lock lock(toolsMutex_);
        auto it = std::ranges::find_if(tools_, [&](const auto& tool) { return tool.name == name; });
        if (it == tools_.end()) {
            return GuiToolResult{.success = false, .content = "Unknown tool: " + name};
        }
        // Refused here rather than in the handler: the handler runs on the UI thread and would
        // have to know who called it, which is exactly the sort of thing the actions layer is
        // kept free of.
        if (origin == CallOrigin::Remote) {
            if (it->localOnly) {
                return GuiToolResult{.success = false,
                    .content = "'" + name
                        + "' can only be used from inside the application window, not through the "
                          "remote control."};
            }
            if (auto withheld = withheldParametersUsed(*it, argumentsJson); !withheld.empty()) {
                std::string names;
                for (const auto& parameter : withheld) {
                    names += (names.empty() ? "" : ", ") + parameter;
                }
                return GuiToolResult{.success = false,
                    .content = "Nothing was changed. " + names
                        + " opens a file dialog for the person at the window, which the remote "
                          "control cannot answer -- pass the path itself instead."};
            }
        }
        timeout = it->timeout;
    }

    QueuedCall call;
    call.name = name;
    call.argumentsJson = argumentsJson;
    auto future = call.resultPromise.get_future();

    {
        std::scoped_lock lock(queueMutex_);
        queue_.push_back(std::move(call));
    }

    if (future.wait_for(timeout) != std::future_status::ready) {
        return GuiToolResult{.success = false, .content = "Tool call to '" + name + "' timed out."};
    }
    return future.get();
}

void GuiToolRegistry::processQueue() {
    std::deque<QueuedCall> pending;
    {
        std::scoped_lock lock(queueMutex_);
        pending.swap(queue_);
    }

    for (auto& call : pending) {
        std::function<GuiToolResult(const Json::JsonValue&)> handler;
        {
            std::scoped_lock lock(toolsMutex_);
            auto it = std::ranges::find_if(tools_, [&](const auto& tool) { return tool.name == call.name; });
            if (it != tools_.end()) {
                handler = it->handler;
            }
        }

        if (!handler) {
            call.resultPromise.set_value(GuiToolResult{.success = false, .content = "Unknown tool: " + call.name});
            continue;
        }

        GuiToolResult result;
        try {
            auto arguments = call.argumentsJson.empty()
                ? Json::JsonValue::object()
                : Json::JsonValue::parse(call.argumentsJson);
            result = handler(arguments);
        } catch (const std::exception& ex) {
            result = GuiToolResult{
                .success = false,
                .content = "Invalid arguments for tool '" + call.name + "': " + std::string(ex.what())
            };
        }
        call.resultPromise.set_value(std::move(result));
    }
}

} // namespace QaplaLlm
