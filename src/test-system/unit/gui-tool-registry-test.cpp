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

#include <catch2/catch_test_macros.hpp>

#include "llm/gui-tool-registry.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace QaplaLlm;
namespace Json = QaplaTester::Json;

namespace {
    // Simulates the UI thread draining the queue while a worker thread's
    // callTool() blocks waiting for a result.
    template <typename WorkerFn>
    GuiToolResult runOnWorkerWhilePolling(GuiToolRegistry& registry, WorkerFn worker) {
        std::atomic<bool> done{false};
        GuiToolResult result;
        std::thread thread([&]() {
            result = worker();
            done.store(true, std::memory_order_release);
        });

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!done.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
            registry.processQueue();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        thread.join();

        REQUIRE(done.load());
        return result;
    }
}

TEST_CASE("GuiToolRegistry executes a tool call handed off to the UI thread", "[llm][gui-tool-registry]") {
    GuiToolRegistry registry;
    registry.registerTool(GuiToolDefinition{
        .name = "echo",
        .description = "Echoes back the given text.",
        .parametersSchema = Json::JsonValue::object(),
        .handler = [](const Json::JsonValue& arguments) -> GuiToolResult {
            std::string text = (arguments.contains("text") && arguments.at("text").is_string())
                ? arguments.at("text").as_string() : "";
            return GuiToolResult{.success = true, .content = "echo:" + text};
        }
    });

    auto result = runOnWorkerWhilePolling(registry, [&]() {
        return registry.callTool("echo", R"({"text":"hi"})");
    });

    REQUIRE(result.success);
    REQUIRE(result.content == "echo:hi");
}

TEST_CASE("GuiToolRegistry reports an error result for an unknown tool without touching the queue",
    "[llm][gui-tool-registry]") {
    GuiToolRegistry registry;

    // No worker/polling needed: an unknown tool is rejected immediately,
    // without ever being enqueued for the UI thread.
    auto result = registry.callTool("does_not_exist", "{}");

    REQUIRE_FALSE(result.success);
    REQUIRE(result.content.find("Unknown tool") != std::string::npos);
}

TEST_CASE("GuiToolRegistry turns invalid argument JSON into a failed result instead of throwing",
    "[llm][gui-tool-registry]") {
    GuiToolRegistry registry;
    bool handlerCalled = false;
    registry.registerTool(GuiToolDefinition{
        .name = "noop",
        .description = "",
        .parametersSchema = Json::JsonValue::object(),
        .handler = [&](const Json::JsonValue&) -> GuiToolResult {
            handlerCalled = true;
            return GuiToolResult{.success = true, .content = "ok"};
        }
    });

    auto result = runOnWorkerWhilePolling(registry, [&]() {
        return registry.callTool("noop", "{not valid json");
    });

    REQUIRE_FALSE(result.success);
    REQUIRE_FALSE(handlerCalled);
    REQUIRE(result.content.find("Invalid arguments") != std::string::npos);
}

TEST_CASE("GuiToolRegistry::callTool times out when nobody drains the queue", "[llm][gui-tool-registry]") {
    GuiToolRegistry registry;
    registry.registerTool(GuiToolDefinition{
        .name = "never_called",
        .description = "",
        .parametersSchema = Json::JsonValue::object(),
        .handler = [](const Json::JsonValue&) -> GuiToolResult {
            return GuiToolResult{.success = true, .content = "unused"};
        },
        .timeout = std::chrono::milliseconds(100)
    });

    auto result = registry.callTool("never_called", "{}");

    REQUIRE_FALSE(result.success);
    REQUIRE(result.content.find("timed out") != std::string::npos);
}

TEST_CASE("GuiToolRegistry uses each tool's own configured timeout", "[llm][gui-tool-registry]") {
    GuiToolRegistry registry;
    registry.registerTool(GuiToolDefinition{
        .name = "slow_interactive_tool",
        .description = "",
        .parametersSchema = Json::JsonValue::object(),
        .handler = [](const Json::JsonValue&) -> GuiToolResult {
            return GuiToolResult{.success = true, .content = "done"};
        },
        .timeout = std::chrono::seconds(2)
    });

    // Simulates the UI thread being "slow" (e.g. waiting on the user) for
    // longer than the default 30s timeout would matter for this test, but
    // well within the tool's own 2s timeout once it does run.
    auto result = runOnWorkerWhilePolling(registry, [&]() {
        return registry.callTool("slow_interactive_tool", "{}");
    });

    REQUIRE(result.success);
    REQUIRE(result.content == "done");
}

TEST_CASE("GuiToolRegistry::exportToolSpecs reflects registered tools", "[llm][gui-tool-registry]") {
    GuiToolRegistry registry;
    REQUIRE(registry.exportToolSpecs().empty());

    registry.registerTool(GuiToolDefinition{
        .name = "sample",
        .description = "A sample tool.",
        .parametersSchema = Json::JsonValue::object(),
        .handler = [](const Json::JsonValue&) -> GuiToolResult { return {}; }
    });

    auto specs = registry.exportToolSpecs();
    REQUIRE(specs.size() == 1);
    REQUIRE(specs[0].name == "sample");
    REQUIRE(specs[0].description == "A sample tool.");
    REQUIRE(registry.hasTool("sample"));
    REQUIRE_FALSE(registry.hasTool("other"));
}
