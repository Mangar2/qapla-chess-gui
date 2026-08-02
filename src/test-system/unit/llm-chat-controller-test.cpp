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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

using namespace QaplaLlm;

namespace {
    // A real (system temp, never the user's actual config directory) directory dedicated to
    // tests that reach a clean success -- LlmChatController's logDirectory is injectable
    // exactly so tests never write into the real ~/.qapla-chess-gui/finetuning.json.
    std::filesystem::path testLogDirectory() {
        return std::filesystem::temp_directory_path() / "qapla-llm-chat-controller-test";
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream in(path);
        std::ostringstream out;
        out << in.rdbuf();
        return out.str();
    }
}

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
        server.Post("/v1/chat/completions", [this](const httplib::Request& req, httplib::Response& res) {
            if (req.body.find("\"Hi\"") != std::string::npos) {
                // LlmChatController::setModel() triggers an automatic reachability ping (see
                // its pingModel()) -- answer it trivially so it doesn't steal a slot from the
                // call-count-based sequencing below, which is only about the real conversation.
                res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"pong"}}]})",
                    "application/json");
                return;
            }
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

    // This turn's successful tool call + "Done." final reply is exactly the "clean but
    // unstructured final reply" shape LlmChatController now corrects into finetuning.json
    // (see the dedicated test for that) -- redirect away from the real config directory.
    std::filesystem::remove_all(testLogDirectory());
    std::filesystem::create_directories(testLogDirectory());

    LlmChatController controller(
        mock.connection(), "system prompt", /*maxToolIterations=*/10, /*logTraffic=*/false,
        testLogDirectory().string());
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

    std::filesystem::remove_all(testLogDirectory());
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
    server.Post("/v1/chat/completions", [&completionCallCount](const httplib::Request& req, httplib::Response& res) {
        if (req.body.find("\"Hi\"") != std::string::npos) {
            // See MockToolCallingServer above: the automatic reachability ping triggered by
            // setModel() must not steal a slot from the call-count-based sequencing below.
            res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"pong"}}]})",
                "application/json");
            return;
        }
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

    // Same reasoning as the probe test above: this turn's successful tool call + "Done." final
    // reply now gets corrected into finetuning.json -- redirect away from the real directory.
    std::filesystem::remove_all(testLogDirectory());
    std::filesystem::create_directories(testLogDirectory());

    LlmChatController controller(
        connection, "system prompt", /*maxToolIterations=*/10, /*logTraffic=*/false, testLogDirectory().string());
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

    std::filesystem::remove_all(testLogDirectory());

    server.stop();
    serverThread.join();
}

TEST_CASE("LlmChatController automatically pings a model when it is selected", "[llm][llm-chat-controller]") {
    httplib::Server server;
    std::atomic<int> pingCallCount{0};
    server.Post("/v1/chat/completions", [&pingCallCount](const httplib::Request& req, httplib::Response& res) {
        if (req.body.find("\"Hi\"") != std::string::npos) {
            pingCallCount.fetch_add(1);
            res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"pong"}}]})",
                "application/json");
            return;
        }
        res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"Done."}}]})", "application/json");
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
    REQUIRE(controller.pingStatus() == LlmChatController::PingStatus::NotStarted);

    controller.setModel("model-a");
    REQUIRE(controller.pingStatus() == LlmChatController::PingStatus::Pinging);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (controller.pingStatus() == LlmChatController::PingStatus::Pinging
        && std::chrono::steady_clock::now() < deadline) {
        controller.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    REQUIRE(controller.pingStatus() == LlmChatController::PingStatus::Reachable);
    REQUIRE(pingCallCount.load() == 1);

    // Selecting a different model re-probes it, not the one already confirmed reachable.
    controller.setModel("model-b");
    REQUIRE(controller.pingStatus() == LlmChatController::PingStatus::Pinging);

    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (controller.pingStatus() == LlmChatController::PingStatus::Pinging
        && std::chrono::steady_clock::now() < deadline) {
        controller.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    REQUIRE(controller.pingStatus() == LlmChatController::PingStatus::Reachable);
    REQUIRE(pingCallCount.load() == 2);

    // Re-selecting the same model must not re-ping (setModel() no-ops when unchanged).
    controller.setModel("model-b");
    REQUIRE(pingCallCount.load() == 2);

    server.stop();
    serverThread.join();
}

TEST_CASE("LlmChatController cancels an in-flight ping when a real message is sent",
    "[llm][llm-chat-controller]") {
    // A real sendMessage() must not have to wait for -- or later be clobbered by -- a
    // reachability ping still in flight from setModel(). Delay the ping response so there's a
    // window to call sendMessage() while pingStatus() is still Pinging.
    httplib::Server server;
    server.Post("/v1/chat/completions", [](const httplib::Request& req, httplib::Response& res) {
        if (req.body.find("\"Hi\"") != std::string::npos) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"pong"}}]})",
                "application/json");
            return;
        }
        res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"Done."}}]})", "application/json");
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

    std::filesystem::remove_all(testLogDirectory());
    std::filesystem::create_directories(testLogDirectory());

    LlmChatController controller(
        connection, "system prompt", /*maxToolIterations=*/10, /*logTraffic=*/false, testLogDirectory().string());
    controller.setModel("test-model");
    REQUIRE(controller.pingStatus() == LlmChatController::PingStatus::Pinging);

    // sendMessage() itself is synchronous (just starts the worker thread), so pingStatus() must
    // already reflect the cancellation immediately after this call -- no update() needed.
    controller.sendMessage("hello");
    REQUIRE(controller.pingStatus() == LlmChatController::PingStatus::Reachable);
    REQUIRE(controller.pingError().empty());

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (controller.isBusy() && std::chrono::steady_clock::now() < deadline) {
        controller.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    REQUIRE_FALSE(controller.isBusy());

    // The cancelled ping's eventual (delayed) response must never retroactively flip
    // pingStatus() back to anything else once update() drains it.
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    controller.update();
    REQUIRE(controller.pingStatus() == LlmChatController::PingStatus::Reachable);

    std::filesystem::remove_all(testLogDirectory());

    server.stop();
    serverThread.join();
}

TEST_CASE("LlmChatController routes the model's reply through the reply_to_user tool",
    "[llm][llm-chat-controller]") {
    // Every real conversation turn must offer "tool_choice":"required" and a reply_to_user
    // tool, and a reply_to_user call must become the turn's final answer directly -- not a
    // Tool-role history entry -- without ever going through GuiToolRegistry.
    std::string lastRequestBody;
    httplib::Server server;
    server.Post("/v1/chat/completions", [&lastRequestBody](const httplib::Request& req, httplib::Response& res) {
        if (req.body.find("\"Hi\"") != std::string::npos) {
            res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"pong"}}]})",
                "application/json");
            return;
        }
        lastRequestBody = req.body;
        res.set_content(
            R"({"choices":[{"message":{"role":"assistant","content":null,"tool_calls":)"
            R"([{"id":"call_1","type":"function",)"
            R"("function":{"name":"reply_to_user","arguments":"{\"text\":\"Hello from model\"}"}}]}}]})",
            "application/json");
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

    std::filesystem::remove_all(testLogDirectory());
    std::filesystem::create_directories(testLogDirectory());

    LlmChatController controller(connection, "system prompt", /*maxToolIterations=*/10, /*logTraffic=*/false,
        testLogDirectory().string());
    controller.setModel("test-model");
    controller.sendMessage("hello");

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (controller.isBusy() && std::chrono::steady_clock::now() < deadline) {
        GuiToolRegistry::instance().processQueue();
        controller.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    REQUIRE_FALSE(controller.isBusy());

    REQUIRE(lastRequestBody.find(R"("tool_choice":"required")") != std::string::npos);
    REQUIRE(lastRequestBody.find(R"("name":"reply_to_user")") != std::string::npos);

    int assistantCount = 0;
    int toolCount = 0;
    std::string lastAssistantText;
    for (const auto& entry : controller.history()) {
        if (entry.role == ChatRole::Assistant) {
            ++assistantCount;
            lastAssistantText = entry.text;
        } else if (entry.role == ChatRole::Tool) {
            ++toolCount;
        }
    }
    REQUIRE(assistantCount == 1);
    REQUIRE(lastAssistantText == "Hello from model");
    REQUIRE(toolCount == 0);

    // A fully clean turn (no tool/connection errors) must also be recorded in finetuning.json,
    // as one JSONL line: {"messages":[system, user, assistant(reply_to_user tool_call)]}.
    auto fineTuningContent = readFile(testLogDirectory() / "finetuning.json");
    REQUIRE(fineTuningContent.find(R"({"messages":[{"content":"system prompt","role":"system"})") != std::string::npos);
    REQUIRE(fineTuningContent.find(R"({"content":"hello","role":"user"})") != std::string::npos);
    REQUIRE(fineTuningContent.find(R"("name":"reply_to_user")") != std::string::npos);
    REQUIRE(std::ranges::count(fineTuningContent, '\n') == 1); // exactly one line, one turn

    std::filesystem::remove_all(testLogDirectory());

    server.stop();
    serverThread.join();
}

TEST_CASE("LlmChatController does not record a turn in finetuning.json if any tool call failed",
    "[llm][llm-chat-controller]") {
    // A fine-tuning example must only ever demonstrate correct behavior -- a turn where a tool
    // rejected its arguments (still a well-formed, structured call -- see LlmChatLogger's class
    // docs) must NOT end up in the dataset, even though the turn itself still ends in a normal
    // reply_to_user reply from the model's perspective.
    GuiToolRegistry::instance().registerTool(GuiToolDefinition{
        .name = "test_llm_chat_controller_failing_probe",
        .description = "Test-only probe tool that always reports failure.",
        .handler = [](const QaplaTester::Json::JsonValue&) -> GuiToolResult {
            return GuiToolResult{.success = false, .content = "Invalid arguments."};
        }
    });

    std::atomic<int> completionCallCount{0};
    httplib::Server server;
    server.Post("/v1/chat/completions", [&completionCallCount](const httplib::Request& req, httplib::Response& res) {
        if (req.body.find("\"Hi\"") != std::string::npos) {
            res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"pong"}}]})",
                "application/json");
            return;
        }
        if (completionCallCount.fetch_add(1) == 0) {
            res.set_content(
                R"({"choices":[{"message":{"role":"assistant","content":null,"tool_calls":)"
                R"([{"id":"call_1","type":"function",)"
                R"("function":{"name":"test_llm_chat_controller_failing_probe","arguments":"{}"}}]}}]})",
                "application/json");
        } else {
            res.set_content(
                R"({"choices":[{"message":{"role":"assistant","content":null,"tool_calls":)"
                R"([{"id":"call_2","type":"function",)"
                R"("function":{"name":"reply_to_user","arguments":"{\"text\":\"Done anyway\"}"}}]}}]})",
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

    std::filesystem::remove_all(testLogDirectory());
    std::filesystem::create_directories(testLogDirectory());

    LlmChatController controller(connection, "system prompt", /*maxToolIterations=*/10, /*logTraffic=*/false,
        testLogDirectory().string());
    controller.setModel("test-model");
    controller.sendMessage("hello");

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (controller.isBusy() && std::chrono::steady_clock::now() < deadline) {
        GuiToolRegistry::instance().processQueue();
        controller.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    REQUIRE_FALSE(controller.isBusy());

    // The turn still completed (the model recovered and replied) -- just not cleanly.
    bool sawAssistantReply = false;
    for (const auto& entry : controller.history()) {
        if (entry.role == ChatRole::Assistant && entry.text == "Done anyway") {
            sawAssistantReply = true;
        }
    }
    REQUIRE(sawAssistantReply);

    // No finetuning.json (or an empty one) since the file is only created on the first
    // actually-recorded turn, and this turn was never clean.
    REQUIRE_FALSE(std::filesystem::exists(testLogDirectory() / "finetuning.json"));

    std::filesystem::remove_all(testLogDirectory());

    server.stop();
    serverThread.join();
}

TEST_CASE("LlmChatController corrects a clean turn's unstructured final reply for finetuning.json",
    "[llm][llm-chat-controller]") {
    // Reproduces the real-world case reported against this feature: a turn where every real
    // tool call succeeded, but the model's *final* reply ignored "tool_choice":"required" and
    // came back as plain text instead of a reply_to_user call. The live chat/log must show
    // that faithfully, but finetuning.json should still record the turn -- corrected to show
    // the reply_to_user call the model should have made -- so the successful tool use isn't
    // lost from the dataset, and the dataset never reinforces the plain-text fallback.
    GuiToolRegistry::instance().registerTool(GuiToolDefinition{
        .name = "test_llm_chat_controller_start_probe",
        .description = "Test-only probe tool that always succeeds.",
        .handler = [](const QaplaTester::Json::JsonValue&) -> GuiToolResult {
            return GuiToolResult{.success = true, .content = "Started."};
        }
    });

    std::atomic<int> completionCallCount{0};
    httplib::Server server;
    server.Post("/v1/chat/completions", [&completionCallCount](const httplib::Request& req, httplib::Response& res) {
        if (req.body.find("\"Hi\"") != std::string::npos) {
            res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"pong"}}]})",
                "application/json");
            return;
        }
        if (completionCallCount.fetch_add(1) == 0) {
            res.set_content(
                R"({"choices":[{"message":{"role":"assistant","content":null,"tool_calls":)"
                R"([{"id":"call_1","type":"function",)"
                R"("function":{"name":"test_llm_chat_controller_start_probe","arguments":"{}"}}]}}]})",
                "application/json");
        } else {
            // No tool_calls at all -- the model ignored "tool_choice":"required" for its final
            // reply, exactly the shape observed in practice.
            res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"Tournament started!"}}]})",
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

    std::filesystem::remove_all(testLogDirectory());
    std::filesystem::create_directories(testLogDirectory());

    LlmChatController controller(connection, "system prompt", /*maxToolIterations=*/10, /*logTraffic=*/false,
        testLogDirectory().string());
    controller.setModel("test-model");
    controller.sendMessage("start it");

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (controller.isBusy() && std::chrono::steady_clock::now() < deadline) {
        GuiToolRegistry::instance().processQueue();
        controller.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    REQUIRE_FALSE(controller.isBusy());

    // The live chat shows exactly what happened: the plain-text reply, untouched.
    bool sawPlainReply = false;
    for (const auto& entry : controller.history()) {
        if (entry.role == ChatRole::Assistant && entry.text == "Tournament started!") {
            sawPlainReply = true;
        }
    }
    REQUIRE(sawPlainReply);

    // finetuning.json records the turn anyway, corrected: the successful tool call stays, and
    // the final reply is now a reply_to_user call instead of bare content.
    auto fineTuningContent = readFile(testLogDirectory() / "finetuning.json");
    REQUIRE(fineTuningContent.find(R"("name":"test_llm_chat_controller_start_probe")") != std::string::npos);
    REQUIRE(fineTuningContent.find(R"("name":"reply_to_user")") != std::string::npos);
    REQUIRE(fineTuningContent.find(R"("arguments":"{\"text\":\"Tournament started!\"}")") != std::string::npos);
    REQUIRE(std::ranges::count(fineTuningContent, '\n') == 1);

    std::filesystem::remove_all(testLogDirectory());

    server.stop();
    serverThread.join();
}
