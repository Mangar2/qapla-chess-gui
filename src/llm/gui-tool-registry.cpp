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

std::vector<ToolSpec> GuiToolRegistry::exportToolSpecs() const {
    std::scoped_lock lock(toolsMutex_);
    std::vector<ToolSpec> specs;
    specs.reserve(tools_.size());
    for (const auto& tool : tools_) {
        specs.push_back(ToolSpec{
            .name = tool.name,
            .description = tool.description,
            .parametersSchemaJson = tool.parametersSchema.stringify()
        });
    }
    return specs;
}

GuiToolResult GuiToolRegistry::callTool(const std::string& name, const std::string& argumentsJson) {
    std::chrono::milliseconds timeout;
    {
        std::scoped_lock lock(toolsMutex_);
        auto it = std::ranges::find_if(tools_, [&](const auto& tool) { return tool.name == name; });
        if (it == tools_.end()) {
            return GuiToolResult{.success = false, .content = "Unknown tool: " + name};
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
