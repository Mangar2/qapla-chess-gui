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
#include <thread>

using namespace QaplaLlm;
namespace Json = QaplaTester::Json;

/**
 * @file
 * @brief The shutdown deadlock, reproduced.
 *
 * Pressing "End remote control" while a request was in flight froze the window. The button runs
 * on the UI thread while it is drawing, not polling -- so the request's call was sitting in the
 * queue, its server thread parked in callTool() waiting for the UI thread to drain it. stop()
 * then joined that server thread, and processQueue() is only ever run by the UI thread, which was
 * now in the join. Neither could move until the tool's own timeout expired: thirty seconds for
 * most tools, two minutes for the engine tool.
 *
 * The test puts the two threads into exactly that arrangement -- fire a request, deliberately do
 * not poll, call stop() -- and requires stop() to come back promptly with the pending call
 * actually carried out.
 */

TEST_CASE("RemoteControlServer::stop returns while a tool call is still in flight",
    "[llm][remote-control][shutdown]") {
    auto& registry = GuiToolRegistry::instance();
    std::atomic<int> handlerRuns{0};

    // Registered once per process, like every other tool -- the registry has no removal, and this
    // name is used nowhere else.
    if (!registry.hasTool("shutdown_probe")) {
        registry.registerTool(GuiToolDefinition{
            .name = "shutdown_probe",
            .description = "Ordinary, fast handler.",
            .parametersSchema = Json::JsonValue::object(),
            .handler = [&](const Json::JsonValue&) -> GuiToolResult {
                handlerRuns.fetch_add(1);
                return GuiToolResult{.success = true, .content = "ran"};
            }});
    }

    RemoteControlOptions options{.enabled = true, .port = 0, .token = ""};
    REQUIRE(RemoteControlServer::instance().start(options));
    const int port = RemoteControlServer::instance().port();
    REQUIRE(port > 0);

    std::atomic<bool> requestDone{false};
    std::thread caller([&]() {
        httplib::Client client("127.0.0.1", port);
        client.set_read_timeout(60, 0);
        static_cast<void>(client.Post("/tools/shutdown_probe", "{}", "application/json"));
        requestDone.store(true);
    });

    // Deliberately NOT drained here, which is the whole point: at the moment the user presses the
    // button, the UI thread is drawing, not polling. The call sits in the queue, its server
    // thread parked in callTool(), and the only thread that could release it is the one about to
    // call stop().
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    REQUIRE(handlerRuns.load() == 0);

    const auto before = std::chrono::steady_clock::now();
    RemoteControlServer::instance().stop();
    const auto elapsed = std::chrono::steady_clock::now() - before;

    caller.join();

    // Well under the tool timeout that used to bound this. The figure is not the point; not
    // being "however long the pending call was allowed to take" is.
    REQUIRE(elapsed < std::chrono::seconds(5));
    REQUIRE(handlerRuns.load() == 1);
    REQUIRE(requestDone.load());
    REQUIRE_FALSE(RemoteControlServer::instance().isRunning());
}
