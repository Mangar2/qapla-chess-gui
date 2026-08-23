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
#include "callback-manager.h"
#include "os-helpers.h"

#include <httplib.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
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

/**
 * @brief RAII around the singleton server, so a failing assertion still releases the port.
 *
 * Also gives the server a configuration directory of its own for the duration: it writes
 * remote-control.port into whatever getConfigDirectory() answers, and a test has no business
 * putting files into the configuration the developer works with.
 */
struct RunningServer {
    explicit RunningServer(std::string token)
        : previousConfigDirectory_(QaplaHelpers::OsHelpers::configDirectoryOverride()) {
        configDirectory_ = std::filesystem::temp_directory_path() / "qapla-remote-control-test";
        std::filesystem::remove_all(configDirectory_);
        std::filesystem::create_directories(configDirectory_);
        QaplaHelpers::OsHelpers::setConfigDirectoryOverride(configDirectory_.string());

        RemoteControlOptions options;
        options.enabled = true;
        options.port = 0; // let the OS pick, so the test never fights a busy port
        options.token = std::move(token);
        REQUIRE(RemoteControlServer::instance().start(options));
    }

    ~RunningServer() {
        RemoteControlServer::instance().stop();
        QaplaHelpers::OsHelpers::setConfigDirectoryOverride(previousConfigDirectory_);
        std::error_code error;
        std::filesystem::remove_all(configDirectory_, error);
    }

    /** @brief Where the server keeps remote-control.port for this test. */
    [[nodiscard]] std::filesystem::path portFile() const {
        return configDirectory_ / "remote-control.port";
    }

    [[nodiscard]] httplib::Client client() const {
        httplib::Client client("127.0.0.1", RemoteControlServer::instance().port());
        client.set_read_timeout(5, 0);
        return client;
    }

private:
    std::filesystem::path configDirectory_;
    std::string previousConfigDirectory_;
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

    // Registered before the call, exactly as the window does it.
    std::optional<QaplaWindows::RemoteCallEntry> announced;
    std::mutex announcedMutex;
    auto subscription = QaplaWindows::StaticCallbacks::remoteCall().registerCallback(
        [&](const QaplaWindows::RemoteCallEntry& entry) {
            // The channel fires on whichever thread answered the call, not this one.
            std::scoped_lock lock(announcedMutex);
            announced = entry;
        });

    auto response = requestWhilePolling([&]() {
        return server.client().Post(
            std::string("/tools/") + PROBE_TOOL, R"({"text":"hello"})", "application/json");
    });

    REQUIRE(response);
    REQUIRE(response->status == 200);
    REQUIRE(response->body.find("probe:hello") != std::string::npos);
    REQUIRE(response->body.find("\"ok\":true") != std::string::npos);

    // The same call has to reach whoever is showing it, not just the caller -- that is the whole
    // point of the mode (see ChatbotRemoteControl). Checked by listening the way that window
    // does, because that is the whole of the contract: the server keeps no log to read back.
    REQUIRE(announced.has_value());
    REQUIRE(announced->toolName == PROBE_TOOL);
    REQUIRE(announced->success);
    REQUIRE(announced->content == "probe:hello");
    REQUIRE(announced->arguments.find("hello") != std::string::npos);
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

TEST_CASE("RemoteControlServer publishes the bound port as a file a caller can read",
    "[llm][remote-control]") {
    // How a test harness that started the GUI with --remote-control-port=0 learns which port it
    // got. stdout would not do: on Windows the executable has no console unless it inherits one.
    RunningServer server{TOKEN};

    REQUIRE(std::filesystem::exists(server.portFile()));

    std::ifstream file(server.portFile());
    int reported = 0;
    file >> reported;
    REQUIRE(reported == RemoteControlServer::instance().port());
}

TEST_CASE("RemoteControlServer takes the port file away with the channel",
    "[llm][remote-control]") {
    std::filesystem::path path;
    {
        RunningServer server{TOKEN};
        path = server.portFile();
        REQUIRE(std::filesystem::exists(path));
        RemoteControlServer::instance().stop();
        REQUIRE_FALSE(std::filesystem::exists(path));
    }
}

TEST_CASE("RemoteControlServer accepts a shutdown request only with the token",
    "[llm][remote-control]") {
    RunningServer server{TOKEN};
    auto client = server.client();

    SECTION("without the token nothing is asked of the application") {
        auto response = client.Post("/shutdown", "", "application/json");
        REQUIRE(response);
        REQUIRE(response->status == 401);
        REQUIRE_FALSE(RemoteControlServer::instance().isShutdownRequested());
    }

    SECTION("with the token the request is recorded for the frame loop to act on") {
        // Answered here, carried out there: the flag is what the UI thread reads, because closing
        // the window is its job and not the server thread's.
        httplib::Headers headers{{"Authorization", std::string("Bearer ") + TOKEN}};
        auto response = client.Post("/shutdown", headers, "", "application/json");
        REQUIRE(response);
        REQUIRE(response->status == 200);
        REQUIRE(response->body.find("\"ok\":true") != std::string::npos);
        REQUIRE(RemoteControlServer::instance().isShutdownRequested());
    }
}

TEST_CASE("A new serving session does not inherit the last one's shutdown request",
    "[llm][remote-control]") {
    {
        RunningServer server{""};
        auto response = server.client().Post("/shutdown", "", "application/json");
        REQUIRE(response);
        REQUIRE(RemoteControlServer::instance().isShutdownRequested());
    }

    RunningServer server{""};
    REQUIRE_FALSE(RemoteControlServer::instance().isShutdownRequested());
}
