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

#include "remote-control-server.h"

#include "activity-watch.h"
#include "gui-tool-registry.h"
#include "lm-studio-client.h"

#include <base-elements/qapla-json.h>

#include <httplib.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <format>
#include <optional>
#include <string_view>

namespace QaplaLlm {

namespace {

    constexpr const char* LOOPBACK = "127.0.0.1";

    /** @brief Keeps the on-screen log from growing without bound over a long session. */
    constexpr std::size_t MAX_LOG_ENTRIES = 500;

    /** @brief "--remote-control-port=8137" -> "8137" for prefix "--remote-control-port=". */
    [[nodiscard]] std::string valueAfter(std::string_view argument, std::string_view prefix) {
        if (argument.size() <= prefix.size() || argument.compare(0, prefix.size(), prefix) != 0) {
            return {};
        }
        return std::string(argument.substr(prefix.size()));
    }

    [[nodiscard]] std::string localTimeText() {
        auto now = std::chrono::system_clock::now();
        auto asTime = std::chrono::system_clock::to_time_t(now);
        std::tm parts{};
#ifdef _WIN32
        localtime_s(&parts, &asTime);
#else
        localtime_r(&asTime, &parts);
#endif
        return std::format("{:02}:{:02}:{:02}", parts.tm_hour, parts.tm_min, parts.tm_sec);
    }

    [[nodiscard]] std::string jsonError(const std::string& message) {
        auto object = QaplaTester::Json::JsonValue::object();
        object["ok"] = false;
        object["content"] = message;
        return object.stringify();
    }

    /** @brief The default and the bounds for a caller's own wait limit, in seconds. */
    constexpr long DEFAULT_WAIT_SECONDS = 300;
    constexpr long MIN_WAIT_SECONDS = 1;
    constexpr long MAX_WAIT_SECONDS = 3600;

    [[nodiscard]] std::optional<Actions::Activity> activityFromName(const std::string& name) {
        if (name == "tournament") {
            return Actions::Activity::Tournament;
        }
        if (name == "sprt") {
            return Actions::Activity::Sprt;
        }
        if (name == "epd") {
            return Actions::Activity::Epd;
        }
        return std::nullopt;
    }

    /**
     * @brief The caller's limit, clamped.
     *
     * Clamped at the top because an unbounded wait is indistinguishable from a hung one, and at
     * the bottom because a wait of zero is a poll wearing a wait's clothes.
     */
    [[nodiscard]] std::chrono::milliseconds waitTimeoutOf(const httplib::Request& request) {
        long seconds = DEFAULT_WAIT_SECONDS;
        if (request.has_param("timeout")) {
            try {
                seconds = std::stol(request.get_param_value("timeout"));
            } catch (const std::exception&) {
                seconds = DEFAULT_WAIT_SECONDS;
            }
        }
        seconds = std::clamp(seconds, MIN_WAIT_SECONDS, MAX_WAIT_SECONDS);
        return std::chrono::seconds(seconds);
    }

} // namespace

RemoteControlOptions parseRemoteControlOptions(int argc, char** argv) {
    RemoteControlOptions options;
    for (int i = 1; i < argc; ++i) {
        std::string_view argument = argv[i] != nullptr ? argv[i] : "";
        if (argument == "--remote-control") {
            options.enabled = true;
            continue;
        }
        if (auto port = valueAfter(argument, "--remote-control-port="); !port.empty()) {
            // A malformed port is left at the default rather than aborting startup: the GUI is
            // still perfectly usable by hand, and start() reports the port it actually bound.
            try {
                options.port = std::stoi(port);
            } catch (const std::exception&) {
                // keep the default
            }
            continue;
        }
        if (auto token = valueAfter(argument, "--remote-control-token="); !token.empty()) {
            options.token = token;
        }
    }
    return options;
}

struct RemoteControlServer::Impl {
    httplib::Server server;
    std::thread worker;
    RemoteControlOptions options;
    int boundPort = 0;

    mutable std::mutex entriesMutex;
    std::vector<RemoteCallEntry> entries;

    void appendEntry(RemoteCallEntry entry) {
        std::scoped_lock lock(entriesMutex);
        entries.push_back(std::move(entry));
        if (entries.size() > MAX_LOG_ENTRIES) {
            entries.erase(entries.begin(), entries.begin() + (entries.size() - MAX_LOG_ENTRIES));
        }
    }

    [[nodiscard]] bool isAuthorized(const httplib::Request& request) const {
        if (options.token.empty()) {
            return true;
        }
        auto header = request.get_header_value("Authorization");
        return header == "Bearer " + options.token;
    }

    /**
     * @brief Runs one tool and answers with its result, logging it either way.
     *
     * The blocking is deliberate and is the whole contract: callTool() hands the call to the UI
     * thread and waits for it, so the response body describes what the GUI has actually done, not
     * what it has been asked to do. A caller can act on the answer immediately -- see the abrupt
     * stop in gui-action-tournament.cpp for what reporting an intention instead used to cost.
     */
    void runTool(const std::string& name, const std::string& argumentsJson,
        httplib::Response& response) {
        auto result =
            GuiToolRegistry::instance().callTool(name, argumentsJson, CallOrigin::Remote);

        appendEntry(RemoteCallEntry{.time = localTimeText(),
            .toolName = name,
            .arguments = argumentsJson,
            .success = result.success,
            .content = result.content,
            .renderWidget = result.renderWidget});

        auto object = QaplaTester::Json::JsonValue::object();
        object["ok"] = result.success;
        object["content"] = result.content;
        response.set_content(object.stringify(), "application/json");
    }
};

RemoteControlServer::RemoteControlServer() : impl_(std::make_unique<Impl>()) {
}

RemoteControlServer::~RemoteControlServer() {
    stop();
}

RemoteControlServer& RemoteControlServer::instance() {
    static RemoteControlServer instance;
    return instance;
}

bool RemoteControlServer::start(const RemoteControlOptions& options) {
    if (isRunning()) {
        return false;
    }
    impl_->options = options;

    impl_->server.Get("/health", [](const httplib::Request&, httplib::Response& response) {
        // Answers off the server thread without ever entering the tool queue, so it stays a
        // truthful "the process is alive" even while the UI thread is busy with a long call.
        auto object = QaplaTester::Json::JsonValue::object();
        object["ok"] = true;
        response.set_content(object.stringify(), "application/json");
    });

    impl_->server.Get("/tools", [this](const httplib::Request& request, httplib::Response& response) {
        if (!impl_->isAuthorized(request)) {
            response.status = 401;
            response.set_content(jsonError("Unauthorized."), "application/json");
            return;
        }
        // The exact shape the local chat sends to LM Studio -- one description of the tools, not
        // two that could drift apart -- minus what only makes sense with a person at the window
        // (see CallOrigin).
        response.set_content(
            toolSpecsToJson(GuiToolRegistry::instance().exportToolSpecs(CallOrigin::Remote)),
            "application/json");
    });

    impl_->server.Post("/tools/:name",
        [this](const httplib::Request& request, httplib::Response& response) {
            if (!impl_->isAuthorized(request)) {
                response.status = 401;
                response.set_content(jsonError("Unauthorized."), "application/json");
                return;
            }
            auto name = request.path_params.at("name");
            if (!GuiToolRegistry::instance().hasTool(name)) {
                response.status = 404;
                response.set_content(jsonError("No such tool: " + name), "application/json");
                return;
            }
            // A tool that exists but is not for remote callers is answered by runTool() with a
            // 200 and ok:false, not a 404 -- it is a refusal with a reason, not a wrong address,
            // and the reason belongs where every other tool refusal goes.
            impl_->runTool(name, request.body, response);
        });

    impl_->server.Get("/wait", [this](const httplib::Request& request, httplib::Response& response) {
        if (!impl_->isAuthorized(request)) {
            response.status = 401;
            response.set_content(jsonError("Unauthorized."), "application/json");
            return;
        }
        auto activity = activityFromName(request.get_param_value("type"));
        if (!activity) {
            response.status = 400;
            response.set_content(
                jsonError("Pass type=tournament, type=sprt or type=epd."), "application/json");
            return;
        }

        // The one call that is meant to take a long time. It holds this connection and this
        // server thread and nothing else -- the UI thread is untouched throughout, so the
        // application stays as responsive to the person watching it as it ever was, and waits on
        // the other two activities are unaffected.
        auto waited = ActivityWatch::instance().waitUntilIdle(*activity, waitTimeoutOf(request));

        // The state as it now stands, gathered after waking rather than described from the
        // snapshot: a caller woken by a finished SPRT test wants the decision, and this is the
        // same text -- results included -- that get_status would give it in a second round-trip
        // it should not have to make.
        auto arguments = QaplaTester::Json::JsonValue::object();
        arguments["type"] = request.get_param_value("type");
        auto status = GuiToolRegistry::instance().callTool(
            "get_status", arguments.stringify(), CallOrigin::Remote);

        auto object = QaplaTester::Json::JsonValue::object();
        object["ok"] = true;
        object["reason"] = waitReasonName(waited.reason);
        object["run"] = static_cast<double>(waited.snapshot.run);
        object["revision"] = static_cast<double>(waited.snapshot.revision);
        object["content"] = status.content;
        response.set_content(object.stringify(), "application/json");
    });

    impl_->server.Get("/status", [this](const httplib::Request& request, httplib::Response& response) {
        if (!impl_->isAuthorized(request)) {
            response.status = 401;
            response.set_content(jsonError("Unauthorized."), "application/json");
            return;
        }
        // Sugar, not a second implementation: the overview a caller wants here is exactly what
        // get_status answers with no arguments.
        impl_->runTool("get_status", "{}", response);
    });

    // Bound here rather than inside the worker so a port that is already taken is an answer this
    // function can give, instead of a silent failure the user only notices by nothing responding.
    // Port 0 asks the OS for a free one; port() then reports which, so a caller that does not
    // care about the number does not have to gamble on one being free.
    impl_->boundPort = options.port == 0 ? impl_->server.bind_to_any_port(LOOPBACK)
                                         : (impl_->server.bind_to_port(LOOPBACK, options.port)
                                                   ? options.port
                                                   : 0);
    if (impl_->boundPort <= 0) {
        impl_->boundPort = 0;
        return false;
    }

    impl_->worker = std::thread([this]() { impl_->server.listen_after_bind(); });

    // Waited for, not assumed: httplib's stop() is a no-op while the server has not started
    // listening yet (it checks is_running_ before touching the socket), so a start immediately
    // followed by a stop would leave the worker listening forever with nothing left to tell it
    // otherwise -- and the join below it would never return.
    impl_->server.wait_until_ready();
    if (!impl_->server.is_running()) {
        if (impl_->worker.joinable()) {
            impl_->worker.join();
        }
        impl_->boundPort = 0;
        return false;
    }
    return true;
}

void RemoteControlServer::stop() {
    if (impl_->boundPort == 0) {
        return;
    }
    // Before the server: a handler parked in a wait would otherwise hold the thread this function
    // is about to join, for as long as its caller's timeout -- and would tell that caller nothing
    // about why it stopped being answered.
    ActivityWatch::instance().cancelWaits();
    impl_->server.stop();
    if (impl_->worker.joinable()) {
        impl_->worker.join();
    }
    impl_->boundPort = 0;
}

bool RemoteControlServer::isRunning() const {
    return impl_->boundPort != 0;
}

int RemoteControlServer::port() const {
    return impl_->boundPort;
}

std::vector<RemoteCallEntry> RemoteControlServer::entries() const {
    std::scoped_lock lock(impl_->entriesMutex);
    return impl_->entries;
}

} // namespace QaplaLlm
