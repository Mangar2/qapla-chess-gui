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
#include "llm/remote-control-server.h"

#include <httplib.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

using namespace QaplaLlm;
namespace Json = QaplaTester::Json;

namespace {

constexpr const char* PROBE_TOOL = "test_remote_control_probe";
constexpr const char* TOKEN = "s3cret";

/**
 * @brief Registers the probe tool once into the process-wide registry the server serves.
 *
 * The server deliberately has no registry of its own -- it exists to expose the one the GUI
 * already fills -- so a test of it is necessarily a test through that singleton.
 */
void ensureProbeToolRegistered() {
    if (GuiToolRegistry::instance().hasTool(PROBE_TOOL)) {
        return;
    }
    GuiToolRegistry::instance().registerTool(GuiToolDefinition{
        .name = PROBE_TOOL,
        .description = "Test-only probe: echoes back its \"text\" argument.",
        .handler = [](const Json::JsonValue& arguments) -> GuiToolResult {
            std::string text = (arguments.contains("text") && arguments.at("text").is_string())
                ? arguments.at("text").as_string()
                : "";
            return GuiToolResult{.success = true, .content = "probe:" + text};
        }});
}

/**
 * @brief Issues one request while draining the tool queue, standing in for the frame loop.
 *
 * Anything that reaches a tool blocks the server thread until the UI thread has run the handler
 * (see GuiToolRegistry::callTool), so a test that just calls the client and waits would deadlock
 * against itself. This is the same shape as runOnWorkerWhilePolling() in
 * gui-tool-registry-test.cpp, one layer further out.
 */
template <typename RequestFn>
httplib::Result requestWhilePolling(RequestFn request) {
    std::atomic<bool> done{false};
    httplib::Result result{nullptr, httplib::Error::Unknown};
    std::thread thread([&]() {
        result = request();
        done.store(true, std::memory_order_release);
    });

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!done.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
        GuiToolRegistry::instance().processQueue();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    thread.join();
    REQUIRE(done.load());
    return result;
}

/** @brief RAII around the singleton server, so a failing assertion still releases the port. */
struct RunningServer {
    explicit RunningServer(std::string token) {
        RemoteControlOptions options;
        options.enabled = true;
        options.port = 0; // let the OS pick, so the test never fights a busy port
        options.token = std::move(token);
        REQUIRE(RemoteControlServer::instance().start(options));
    }

    ~RunningServer() {
        RemoteControlServer::instance().stop();
    }

    [[nodiscard]] httplib::Client client() const {
        httplib::Client client("127.0.0.1", RemoteControlServer::instance().port());
        client.set_read_timeout(5, 0);
        return client;
    }
};

} // namespace

TEST_CASE("RemoteControlServer answers /health without entering the tool queue",
    "[llm][remote-control]") {
    RunningServer server{TOKEN};

    // No polling around this one on purpose: it must answer while the UI thread is doing
    // something else entirely, which is what makes it usable as a liveness check.
    auto response = server.client().Get("/health");

    REQUIRE(response);
    REQUIRE(response->status == 200);
    REQUIRE(response->body.find("true") != std::string::npos);
}

TEST_CASE("RemoteControlServer requires the configured token", "[llm][remote-control]") {
    RunningServer server{TOKEN};
    auto client = server.client();

    SECTION("a request without the token is refused") {
        auto response = client.Get("/tools");
        REQUIRE(response);
        REQUIRE(response->status == 401);
    }

    SECTION("a request with the token is served") {
        httplib::Headers headers{{"Authorization", std::string("Bearer ") + TOKEN}};
        auto response = client.Get("/tools", headers);
        REQUIRE(response);
        REQUIRE(response->status == 200);
    }
}

TEST_CASE("RemoteControlServer publishes the registry's tools", "[llm][remote-control]") {
    ensureProbeToolRegistered();
    RunningServer server{""}; // no token configured: everything is open on loopback

    auto response = server.client().Get("/tools");

    REQUIRE(response);
    REQUIRE(response->status == 200);
    REQUIRE(response->body.find(PROBE_TOOL) != std::string::npos);
}

TEST_CASE("RemoteControlServer runs a tool on the UI thread and reports its real result",
    "[llm][remote-control]") {
    ensureProbeToolRegistered();
    RunningServer server{""};

    auto response = requestWhilePolling([&]() {
        return server.client().Post(
            std::string("/tools/") + PROBE_TOOL, R"({"text":"hello"})", "application/json");
    });

    REQUIRE(response);
    REQUIRE(response->status == 200);
    REQUIRE(response->body.find("probe:hello") != std::string::npos);
    REQUIRE(response->body.find("\"ok\":true") != std::string::npos);

    // The same call has to be visible to the user, not just to the caller -- that is the whole
    // point of the mode (see ChatbotRemoteControl).
    auto entries = RemoteControlServer::instance().entries();
    REQUIRE_FALSE(entries.empty());
    const auto& last = entries.back();
    REQUIRE(last.toolName == PROBE_TOOL);
    REQUIRE(last.success);
    REQUIRE(last.content == "probe:hello");
    REQUIRE(last.arguments.find("hello") != std::string::npos);
}

TEST_CASE("RemoteControlServer rejects an unknown tool by name", "[llm][remote-control]") {
    RunningServer server{""};

    // Answered from the tool list without ever being enqueued, so no polling is needed here.
    auto response = server.client().Post("/tools/no_such_tool", "{}", "application/json");

    REQUIRE(response);
    REQUIRE(response->status == 404);
}

TEST_CASE("RemoteControlServer refuses to start twice and reports the port it bound",
    "[llm][remote-control]") {
    RunningServer server{""};
    REQUIRE(RemoteControlServer::instance().isRunning());
    REQUIRE(RemoteControlServer::instance().port() > 0);

    RemoteControlOptions second;
    second.enabled = true;
    second.port = 0;
    REQUIRE_FALSE(RemoteControlServer::instance().start(second));
}
