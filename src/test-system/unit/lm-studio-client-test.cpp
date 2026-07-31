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

#include "llm/lm-studio-client.h"

#include <httplib.h>

#include <chrono>
#include <thread>

using namespace QaplaLlm;

namespace {

struct MockServer {
    httplib::Server server;
    std::thread thread;
    int port = 0;

    void start() {
        port = server.bind_to_any_port("127.0.0.1");
        REQUIRE(port > 0);
        thread = std::thread([this]() { server.listen_after_bind(); });
        while (!server.is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    ~MockServer() {
        server.stop();
        if (thread.joinable()) {
            thread.join();
        }
    }

    [[nodiscard]] LmStudioConnection connection() const {
        LmStudioConnection connection;
        connection.host = "127.0.0.1";
        connection.port = port;
        connection.timeoutMs = 2000;
        return connection;
    }
};

} // namespace

TEST_CASE("LmStudioClient::listModels parses the model id list", "[llm][lm-studio-client]") {
    MockServer mock;
    mock.server.Get("/v1/models", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"data":[{"id":"qwen2.5-7b-instruct"},{"id":"llama-3-8b"}]})", "application/json");
    });
    mock.start();

    LmStudioClient client(mock.connection());
    auto result = client.listModels();

    REQUIRE(result.success);
    REQUIRE(result.modelIds.size() == 2);
    REQUIRE(result.modelIds[0] == "qwen2.5-7b-instruct");
    REQUIRE(result.modelIds[1] == "llama-3-8b");
}

TEST_CASE("LmStudioClient::listModels filters out embedding models using LM Studio's native REST API",
    "[llm][lm-studio-client]") {
    MockServer mock;
    mock.server.Get("/v1/models", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(
            R"({"data":[{"id":"qwen3.5-9b"},{"id":"google/gemma-3-12b"},)"
            R"({"id":"text-embedding-nomic-embed-text-v1.5"}]})",
            "application/json");
    });
    mock.server.Get("/api/v0/models", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(
            R"({"data":[)"
            R"({"id":"qwen3.5-9b","type":"llm","state":"loaded"},)"
            R"({"id":"google/gemma-3-12b","type":"llm","state":"not-loaded"},)"
            R"({"id":"text-embedding-nomic-embed-text-v1.5","type":"embeddings","state":"not-loaded"}])"
            R"(})",
            "application/json");
    });
    mock.start();

    LmStudioClient client(mock.connection());
    auto result = client.listModels();

    REQUIRE(result.success);
    REQUIRE(result.modelIds.size() == 2);
    REQUIRE(result.modelIds[0] == "qwen3.5-9b");
    REQUIRE(result.modelIds[1] == "google/gemma-3-12b");
}

TEST_CASE("LmStudioClient::listModels keeps the full list when the native REST API is unavailable",
    "[llm][lm-studio-client]") {
    // No /api/v0/models handler registered -- simulates an older LM Studio version that
    // doesn't have it. Filtering is best-effort: listModels() must still succeed with the
    // unfiltered list, not fail or hang.
    MockServer mock;
    mock.server.Get("/v1/models", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"data":[{"id":"qwen2.5-7b-instruct"},{"id":"llama-3-8b"}]})", "application/json");
    });
    mock.start();

    LmStudioClient client(mock.connection());
    auto result = client.listModels();

    REQUIRE(result.success);
    REQUIRE(result.modelIds.size() == 2);
}

TEST_CASE("LmStudioClient::listModels reports a readable error when nothing listens", "[llm][lm-studio-client]") {
    LmStudioConnection connection;
    connection.host = "127.0.0.1";
    connection.port = 1; // nothing listens here
    connection.timeoutMs = 500;

    LmStudioClient client(connection);
    auto result = client.listModels();

    REQUIRE_FALSE(result.success);
    REQUIRE(result.modelIds.empty());
    REQUIRE_FALSE(result.errorMessage.empty());
}

TEST_CASE("LmStudioClient::chatCompletion extracts the assistant reply", "[llm][lm-studio-client]") {
    MockServer mock;
    mock.server.Post("/v1/chat/completions", [](const httplib::Request& req, httplib::Response& res) {
        REQUIRE(req.body.find("\"model\"") != std::string::npos);
        REQUIRE(req.body.find("\"messages\"") != std::string::npos);
        res.set_content(
            R"({"choices":[{"message":{"role":"assistant","content":"Hello there!"}}]})",
            "application/json");
    });
    mock.start();

    LmStudioClient client(mock.connection());
    ChatCompletionRequest request;
    request.model = "qwen2.5-7b-instruct";
    request.messages.push_back({"system", "You are a helpful assistant."});
    request.messages.push_back({"user", "Hi"});

    auto result = client.chatCompletion(request);

    REQUIRE(result.success);
    REQUIRE(result.content == "Hello there!");
}

TEST_CASE("LmStudioClient::chatCompletion surfaces server-side error messages", "[llm][lm-studio-client]") {
    MockServer mock;
    mock.server.Post("/v1/chat/completions", [](const httplib::Request&, httplib::Response& res) {
        res.status = 400;
        res.set_content(R"({"error":{"message":"No model loaded."}})", "application/json");
    });
    mock.start();

    LmStudioClient client(mock.connection());
    ChatCompletionRequest request;
    request.model = "";
    request.messages.push_back({"user", "Hi"});

    auto result = client.chatCompletion(request);

    REQUIRE_FALSE(result.success);
    REQUIRE(result.errorMessage.find("No model loaded.") != std::string::npos);
}

TEST_CASE("LmStudioClient::chatCompletion sends tools and parses tool_calls", "[llm][lm-studio-client]") {
    MockServer mock;
    mock.server.Post("/v1/chat/completions", [](const httplib::Request& req, httplib::Response& res) {
        REQUIRE(req.body.find("\"tools\"") != std::string::npos);
        REQUIRE(req.body.find("\"list_installed_engines\"") != std::string::npos);
        REQUIRE(req.body.find(R"("tool_choice":"required")") != std::string::npos);
        res.set_content(
            R"({"choices":[{"message":{"role":"assistant","content":null,"tool_calls":)"
            R"([{"id":"call_1","type":"function",)"
            R"("function":{"name":"list_installed_engines","arguments":"{}"}}]}}]})",
            "application/json");
    });
    mock.start();

    LmStudioClient client(mock.connection());
    ChatCompletionRequest request;
    request.model = "test-model";
    request.messages.push_back({"user", "please list engines"});
    request.tools.push_back(ToolSpec{
        .name = "list_installed_engines",
        .description = "Lists engines.",
        .parametersSchemaJson = R"({"type":"object","properties":{}})"
    });

    auto result = client.chatCompletion(request);

    REQUIRE(result.success);
    REQUIRE(result.content.empty());
    REQUIRE(result.toolCalls.size() == 1);
    REQUIRE(result.toolCalls[0].id == "call_1");
    REQUIRE(result.toolCalls[0].name == "list_installed_engines");
    REQUIRE(result.toolCalls[0].argumentsJson == "{}");
}

TEST_CASE("LmStudioClient::chatCompletion echoes assistant tool_calls and tool results on the wire",
    "[llm][lm-studio-client]") {
    MockServer mock;
    mock.server.Post("/v1/chat/completions", [](const httplib::Request& req, httplib::Response& res) {
        REQUIRE(req.body.find("\"tool_call_id\":\"call_1\"") != std::string::npos);
        REQUIRE(req.body.find("\"role\":\"tool\"") != std::string::npos);
        REQUIRE(req.body.find("\"tool_calls\"") != std::string::npos);
        res.set_content(R"({"choices":[{"message":{"role":"assistant","content":"Done."}}]})", "application/json");
    });
    mock.start();

    LmStudioClient client(mock.connection());
    ChatCompletionRequest request;
    request.model = "test-model";

    ChatMessage assistantMessage;
    assistantMessage.role = "assistant";
    assistantMessage.toolCalls.push_back(ToolCall{.id = "call_1", .name = "some_tool", .argumentsJson = "{}"});
    request.messages.push_back(assistantMessage);

    ChatMessage toolMessage;
    toolMessage.role = "tool";
    toolMessage.toolCallId = "call_1";
    toolMessage.content = "result text";
    request.messages.push_back(toolMessage);

    auto result = client.chatCompletion(request);

    REQUIRE(result.success);
    REQUIRE(result.content == "Done.");
}
