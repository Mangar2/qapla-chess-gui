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

#include "llm/lm-studio-locator.h"

#include <httplib.h>

#include <chrono>
#include <filesystem>
#include <thread>

using namespace QaplaLlm;

TEST_CASE("LmStudioLocator::isInstalledAt detects existing paths", "[llm][lm-studio-locator]") {
    auto tempDir = std::filesystem::temp_directory_path() / "qapla-lmstudio-locator-test";
    std::filesystem::create_directories(tempDir);

    REQUIRE(LmStudioLocator::isInstalledAt({tempDir}));
    REQUIRE_FALSE(LmStudioLocator::isInstalledAt({tempDir / "does-not-exist"}));
    REQUIRE_FALSE(LmStudioLocator::isInstalledAt({}));

    std::filesystem::remove_all(tempDir);
}

TEST_CASE("LmStudioLocator::probeServer detects a running OpenAI-compatible server", "[llm][lm-studio-locator]") {
    httplib::Server server;
    server.Get("/v1/models", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"data": []})", "application/json");
    });

    int port = server.bind_to_any_port("127.0.0.1");
    REQUIRE(port > 0);
    std::thread serverThread([&server]() { server.listen_after_bind(); });

    while (!server.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    LmStudioProbeConfig config;
    config.host = "127.0.0.1";
    config.port = port;
    config.timeoutMs = 1000;

    REQUIRE(LmStudioLocator::probeServer(config));
    REQUIRE(LmStudioLocator::detect(config) == LmStudioStatus::ServerRunning);

    server.stop();
    serverThread.join();
}

TEST_CASE("LmStudioLocator::probeServer returns false when nothing listens", "[llm][lm-studio-locator]") {
    LmStudioProbeConfig config;
    config.host = "127.0.0.1";
    config.port = 1; // nothing listens here; connection should be refused quickly
    config.timeoutMs = 200;

    REQUIRE_FALSE(LmStudioLocator::probeServer(config));
}

TEST_CASE("LmStudioLocator::isLocalHost recognizes loopback aliases only", "[llm][lm-studio-locator]") {
    REQUIRE(LmStudioLocator::isLocalHost("localhost"));
    REQUIRE(LmStudioLocator::isLocalHost("127.0.0.1"));
    REQUIRE(LmStudioLocator::isLocalHost("::1"));

    REQUIRE_FALSE(LmStudioLocator::isLocalHost("192.168.1.42"));
    REQUIRE_FALSE(LmStudioLocator::isLocalHost("my-server.local"));
    REQUIRE_FALSE(LmStudioLocator::isLocalHost(""));
}

TEST_CASE("LmStudioLocator::detect reports RemoteUnreachable for a non-local host with no server, "
    "instead of checking this machine's local installation", "[llm][lm-studio-locator]") {
    LmStudioProbeConfig config;
    config.host = "192.168.255.254"; // non-routable-in-practice address, nothing listens here
    config.port = 1234;
    config.timeoutMs = 200;

    REQUIRE(LmStudioLocator::detect(config) == LmStudioStatus::RemoteUnreachable);
}

TEST_CASE("LmStudioLocator::detect reports ServerRunning for a reachable remote host too",
    "[llm][lm-studio-locator]") {
    httplib::Server server;
    server.Get("/v1/models", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"data": []})", "application/json");
    });

    int port = server.bind_to_any_port("127.0.0.1");
    REQUIRE(port > 0);
    std::thread serverThread([&server]() { server.listen_after_bind(); });
    while (!server.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 127.0.0.1 counts as "local" for isLocalHost(), but the point here is just that a
    // running server always wins regardless of that classification -- detect() checks
    // probeServer() first, before ever looking at isLocalHost().
    LmStudioProbeConfig config;
    config.host = "127.0.0.1";
    config.port = port;
    config.timeoutMs = 1000;

    REQUIRE(LmStudioLocator::detect(config) == LmStudioStatus::ServerRunning);

    server.stop();
    serverThread.join();
}

TEST_CASE("AsyncLmStudioLocator reports readiness asynchronously", "[llm][lm-studio-locator]") {
    LmStudioProbeConfig config;
    config.host = "127.0.0.1";
    config.port = 1;
    config.timeoutMs = 100;

    AsyncLmStudioLocator locator(config);
    REQUIRE_FALSE(locator.isReady());
    locator.start();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!locator.isReady() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    REQUIRE(locator.isReady());
    // No server on port 1 and (most likely) no LM Studio installed on the test
    // machine -> NotInstalled. Assert a valid terminal result rather than the
    // exact value, since installation detection depends on the host machine.
    auto status = locator.status();
    REQUIRE((status == LmStudioStatus::NotInstalled || status == LmStudioStatus::InstalledServerDown));
}
