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

#include "llm/llm-chat-controller.h"
#include "llm/gui-tool-registry.h"

#include <httplib.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace QaplaLlm;

namespace {

// Mocks the two chat/completions round trips one tool call takes: the first
// response requests the tool, the second (after the tool result is fed
// back) is delayed to simulate the model visibly taking longer to compose
// its final reply than the tool call itself takes to execute -- this is
// exactly the ordering that regressed in practice (see commit history).
struct MockToolCallingServer {
    httplib::Server server;
    std::thread thread;
    int port = 0;
    std::atomic<int> completionCallCount{0};

    void start() {
        server.Post("/v1/chat/completions", [this](const httplib::Request&, httplib::Response& res) {
            if (completionCallCount.fetch_add(1) == 0) {
                res.set_content(
                    R"({"choices":[{"message":{"role":"assistant","content":null,"tool_calls":)"
                    R"([{"id":"call_1","type":"function",)"
                    R"("function":{"name":"test_llm_chat_controller_probe","arguments":"{}"}}]}}]})",
                    "application/json");
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"Done."}}]})",
                    "application/json");
            }
        });

        port = server.bind_to_any_port("127.0.0.1");
        REQUIRE(port > 0);
        thread = std::thread([this]() { server.listen_after_bind(); });
        while (!server.is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    ~MockToolCallingServer() {
        server.stop();
        if (thread.joinable()) {
            thread.join();
        }
    }

    [[nodiscard]] LmStudioConnection connection() const {
        LmStudioConnection connection;
        connection.host = "127.0.0.1";
        connection.port = port;
        connection.timeoutMs = 5000;
        return connection;
    }
};

} // namespace

TEST_CASE("LlmChatController shows a tool result before the model's slower final reply arrives",
    "[llm][llm-chat-controller]") {
    GuiToolRegistry::instance().registerTool(GuiToolDefinition{
        .name = "test_llm_chat_controller_probe",
        .description = "Test-only probe tool.",
        .handler = [](const QaplaTester::Json::JsonValue&) -> GuiToolResult {
            return GuiToolResult{.success = true, .content = "PROBE_OK"};
        }
    });

    MockToolCallingServer mock;
    mock.start();

    LlmChatController controller(mock.connection(), "system prompt");
    controller.setModel("test-model");
    controller.sendMessage("please call the probe tool");

    bool sawToolEntryWhileBusy = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!sawToolEntryWhileBusy && std::chrono::steady_clock::now() < deadline) {
        GuiToolRegistry::instance().processQueue(); // normally driven by the app's poll loop
        controller.update();

        if (controller.isBusy()) {
            for (const auto& entry : controller.history()) {
                if (entry.role == ChatRole::Tool && entry.text == "PROBE_OK") {
                    sawToolEntryWhileBusy = true;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    REQUIRE(sawToolEntryWhileBusy);

    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (controller.isBusy() && std::chrono::steady_clock::now() < deadline) {
        GuiToolRegistry::instance().processQueue();
        controller.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    REQUIRE_FALSE(controller.isBusy());

    bool sawFinalAssistantText = false;
    for (const auto& entry : controller.history()) {
        if (entry.role == ChatRole::Assistant && entry.text == "Done.") {
            sawFinalAssistantText = true;
        }
    }
    REQUIRE(sawFinalAssistantText);
}

TEST_CASE("LlmChatController carries a tool's renderWidget through to its ChatEntry",
    "[llm][llm-chat-controller]") {
    // A tool whose whole point is to display something (e.g. show_tournament_result) sets
    // GuiToolResult::renderWidget instead of (or alongside) plain text -- see its doc comment.
    // This must survive the worker-thread hop (ToolCallEvent) and land, callable, in the
    // ChatEntry the chat UI actually draws from history().
    int widgetCallCount = 0;
    GuiToolRegistry::instance().registerTool(GuiToolDefinition{
        .name = "test_llm_chat_controller_widget_probe",
        .description = "Test-only widget probe tool.",
        .handler = [&widgetCallCount](const QaplaTester::Json::JsonValue&) -> GuiToolResult {
            return GuiToolResult{
                .success = true,
                .content = "Showing something.",
                .renderWidget = [&widgetCallCount]() { ++widgetCallCount; }
            };
        }
    });

    // A dedicated inline server rather than MockToolCallingServer: that helper's Post handler
    // is fixed to a different tool name and registering a second handler for the same path
    // would just add a second, order-dependent route -- not worth the fragility here.
    httplib::Server server;
    std::atomic<int> completionCallCount{0};
    server.Post("/v1/chat/completions", [&completionCallCount](const httplib::Request&, httplib::Response& res) {
        if (completionCallCount.fetch_add(1) == 0) {
            res.set_content(
                R"({"choices":[{"message":{"role":"assistant","content":null,"tool_calls":)"
                R"([{"id":"call_1","type":"function",)"
                R"("function":{"name":"test_llm_chat_controller_widget_probe","arguments":"{}"}}]}}]})",
                "application/json");
        } else {
            res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"Done."}}]})",
                "application/json");
        }
    });
    int port = server.bind_to_any_port("127.0.0.1");
    REQUIRE(port > 0);
    std::thread serverThread([&server]() { server.listen_after_bind(); });
    while (!server.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    LmStudioConnection connection;
    connection.host = "127.0.0.1";
    connection.port = port;
    connection.timeoutMs = 5000;

    LlmChatController controller(connection, "system prompt");
    controller.setModel("test-model");
    controller.sendMessage("please call the widget probe tool");

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (controller.isBusy() && std::chrono::steady_clock::now() < deadline) {
        GuiToolRegistry::instance().processQueue();
        controller.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    REQUIRE_FALSE(controller.isBusy());

    const QaplaLlm::ChatEntry* toolEntry = nullptr;
    for (const auto& entry : controller.history()) {
        if (entry.role == ChatRole::Tool) {
            toolEntry = &entry;
        }
    }
    REQUIRE(toolEntry != nullptr);
    REQUIRE(static_cast<bool>(toolEntry->renderWidget));

    toolEntry->renderWidget();
    REQUIRE(widgetCallCount == 1);

    server.stop();
    serverThread.join();
}
