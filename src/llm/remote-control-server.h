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

#pragma once

#include <memory>
#include <string>

/**
 * @file
 * @brief Drives the running GUI from outside the process, over HTTP on localhost.
 *
 * The point is to be watched. Anything started this way runs in the GUI the user is looking at:
 * the games appear on the boards, the tables fill in, the tabs on the left keep working. That is
 * the whole reason this exists rather than the caller driving qapla-engine-tester headlessly --
 * see docs/grobplan-clop-cli-http.md.
 *
 * There is almost nothing here, and that is the design. GuiToolRegistry is already a
 * name-and-JSON remote control: exportToolSpecs() publishes what can be called, callTool() takes
 * a name plus an arguments object, and it is already built to be called from a foreign thread and
 * executed on the UI thread. This server is the socket in front of that, nothing more -- the same
 * eleven tools the local AI chat uses, in the same wording, out of the same registry. A tool added
 * for one is a tool the other has.
 */

namespace QaplaLlm {

/**
 * @brief How the remote control was asked for, on the command line.
 *
 * Deliberately not a tool and not a button: whoever starts the GUI decides what the session is
 * for. A remote control that could switch itself on would be a way in that nobody opened.
 *
 * Filled by QaplaApp::parseCommandLine (see src/command-line.h), which is where the switches
 * that set these fields are spelled out, together with every other option the executable has.
 */
struct RemoteControlOptions {
    bool enabled = false;

    /**
     * @brief TCP port on 127.0.0.1. Never any other interface -- see start().
     *
     * 0 asks the OS for a free one, which RemoteControlServer::port() then reports.
     */
    int port = 8137;

    /** @brief Shared secret required as "Authorization: Bearer <token>"; empty means none. */
    std::string token;
};


/**
 * @brief The HTTP server, its worker thread, and the log of what has been called.
 *
 * Endpoints, all on 127.0.0.1 only:
 * - `GET /health` -- answers without touching the GUI, so a caller can tell "the app is up" from
 *   "the app is busy". The only one that does not need the token.
 * - `GET /tools` -- the tool specs, in the same OpenAI function-calling shape the local chat
 *   sends to LM Studio (see toolSpecsToJson).
 * - `POST /tools/<name>` -- body is the arguments object; answers {"ok":…, "content":…}.
 * - `GET /status` -- sugar for POST /tools/get_status with no arguments.
 * - `GET /wait?type=<tournament|sprt|epd>[&timeout=<seconds>]` -- does not answer until that
 *   activity has stopped running, then reports why (`finished`, `stopped`, `timeout`, `closed`,
 *   `not_running`) together with the full status, results included. See ActivityWatch.
 * - `POST /shutdown` -- asks the application to close, the same way the window's close button
 *   does. Not a tool: `close_application` is deliberately local-only, because an AI watching the
 *   GUI has no business ending it. A test harness does, and it needs the ordinary shutdown to
 *   run so that what the session stored is actually written. See isShutdownRequested().
 *
 * Every call that reaches a tool is announced on StaticCallbacks::remoteCall(), whether it
 * succeeded or not. The server keeps no log of its own: whoever wants to show one registers for
 * it and keeps its own -- so the window that displays these calls need not know that they arrive
 * over HTTP, and this server need not know that anything is displaying them.
 *
 * While it listens, the bound port is also written to `remote-control.port` in the configuration
 * directory (QaplaHelpers::OsHelpers::getConfigDirectory()) and removed again by stop(). A caller
 * that started the GUI with `--remote-control-port=0` and `--config-dir` therefore has a reliable
 * way to learn the port it got, on every platform -- the line printed on stdout is for people
 * reading a log, and on Windows there may not be a console to print it to.
 */
class RemoteControlServer {
public:
    [[nodiscard]] static RemoteControlServer& instance();

    RemoteControlServer(const RemoteControlServer&) = delete;
    RemoteControlServer& operator=(const RemoteControlServer&) = delete;

    /**
     * @brief Binds 127.0.0.1 on the configured port and serves until stop().
     *
     * Binding is done synchronously so a port already in use is reported here rather than
     * disappearing into the worker thread: a GUI that silently isn't reachable is worse than one
     * that says why at startup.
     *
     * @return false if the port could not be bound, or if a server is already running.
     */
    bool start(const RemoteControlOptions& options);

    /**
     * @brief Stops serving and joins the worker thread. Safe to call when not running.
     *
     * Deliberately does not touch anything that is already running in the GUI: ending the remote
     * control closes the channel, it does not stop a tournament. Whatever is playing keeps
     * playing, and is the user's to drive by hand from then on.
     */
    void stop();

    [[nodiscard]] bool isRunning() const;

    /** @brief The port actually bound, or 0 when not running. */
    [[nodiscard]] int port() const;

    /**
     * @brief Whether a caller has asked, over `POST /shutdown`, for the application to close.
     *
     * A flag rather than an action, and read by the frame loop, because closing has to happen on
     * the UI thread: the request arrives on a server thread, and the subscribers of the message
     * that ends the application touch state the UI thread draws from. Within one serving session
     * it latches -- there is nothing to take back after asking an application to quit; start()
     * begins the next session without it.
     */
    [[nodiscard]] bool isShutdownRequested() const;

private:
    RemoteControlServer();
    ~RemoteControlServer();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace QaplaLlm
