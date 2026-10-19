/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "app-runtime.h"

#include <base-elements/logger.h>
#include <game-manager/game-manager-pool.h>
#include <engine-handling/engine-capabilities.h>

#include "configuration.h"
#include "config-group-loader.h"
#include "configuration-window.h"
#include "epd-window.h"
#include "epd-data.h"
#include "engine-test-window.h"
#include "imgui-board-tab-bar.h"
#include "tournament-window.h"
#include "sprt-tournament-window.h"
#include "imgui-tab-bar.h"
#include "imgui-game-list.h"
#include "horizontal-split-container.h"
#include "engine-setup-window.h"
#include "snackbar.h"
#include "tutorial.h"
#include "chatbot/chatbot-window.h"
#include "llm/remote-control-integration.h"
#include "llm/llm-chat-integration.h"
#include "llm/remote-control-server.h"
#include "os-helpers.h"
#include "ui-thread-watch.h"
#include "ui-update-queue.h"

#include <filesystem>
#include <iostream>
#include <system_error>

using QaplaTester::GameManagerPool;

namespace QaplaApp {

    void reportCommandLineMessages(const CommandLineOptions& options) {
        for (const auto& message : options.messages) {
            std::cerr << message << '\n';
        }
    }

    void applySessionOptions(const CommandLineOptions& options) {
        if (options.remoteDesktop) {
            QaplaConfiguration::Configuration::setRemoteDesktopOverride();
        }
    }

    bool applyConfigDirectory(const CommandLineOptions& options) {
        if (options.configDirectory.empty()) {
            return true;
        }

        std::error_code error;
        std::filesystem::create_directories(options.configDirectory, error);
        if (error) {
            std::cerr << "Cannot use \"" << options.configDirectory
                      << "\" as the configuration directory: " << error.message() << '\n';
            return false;
        }

        // Stored as an absolute path so it stays the same directory no matter what the engines and
        // dialogs started from here do with the working directory.
        auto absolutePath = std::filesystem::absolute(options.configDirectory, error);
        QaplaHelpers::OsHelpers::setConfigDirectoryOverride(
            error ? options.configDirectory : absolutePath.string());
        return true;
    }

    QaplaWindows::BoardWorkspace AppRuntime::buildWindows() {

        QaplaWindows::BoardWorkspace workspace;
        workspace.maximize(true);

        auto boardTabBar = std::make_unique<QaplaWindows::ImGuiBoardTabBar>();

        auto taskTabBar = std::make_unique<QaplaWindows::ImGuiTabBar>();
        taskTabBar->addTab("Engines", std::make_unique<QaplaWindows::EngineSetupWindow>(false));
        taskTabBar->addTab("Tournament", std::make_unique<QaplaWindows::TournamentWindow>());
        taskTabBar->addTab("SPRT", std::make_unique<QaplaWindows::SprtTournamentWindow>());
        taskTabBar->addTab("Pgn", std::make_unique<QaplaWindows::ImGuiGameList>());
        taskTabBar->addTab("Epd", std::make_unique<QaplaWindows::EpdWindow>());
        taskTabBar->addTab("Test", std::make_unique<QaplaWindows::EngineTestWindow>());
        taskTabBar->addTab("Settings", std::make_unique<QaplaWindows::ConfigurationWindow>());

        auto mainContainer = std::make_unique<QaplaWindows::HorizontalSplitContainer>(
            "main", QaplaUi::WindowFlags::None);
        mainContainer->setRight(std::move(boardTabBar));
        mainContainer->setLeft(std::move(taskTabBar));
        mainContainer->setPresetWidth(400.0F, true);

        workspace.setRootWindow(std::move(mainContainer));
        return workspace;
    }

    AppRuntime::AppRuntime(const QaplaLlm::RemoteControlOptions& remoteControl, bool autoRunTests) {

        // Installed before the first load so a stored section that no longer matches the
        // schema is reported rather than silently replaced by defaults -- registered here,
        // alongside the other GUI callbacks, because config-group-loader.cpp itself must
        // stay free of GUI dependencies (see setConfigLoadErrorReporter).
        QaplaConfiguration::setConfigLoadErrorReporter(
            [](const std::string& sectionName, const std::string& message) {
                QaplaWindows::SnackbarManager::instance().showWarning(
                    "Could not load the \"" + sectionName + "\" settings, using defaults: " + message,
                    false, "configuration");
            });

        QaplaConfiguration::Configuration::instance().loadFile();
        QaplaConfiguration::Configuration::loadLoggerConfiguration();
        QaplaWindows::EpdData::instance().loadFile();
        QaplaWindows::Tutorial::instance().loadConfiguration();
        QaplaWindows::SnackbarManager::instance().loadConfiguration();

        QaplaConfiguration::EngineCapabilities::setNotificationCallback(
            [](const std::string& message, const std::string& type) {
                auto& snackbar = QaplaWindows::SnackbarManager::instance();
                if (type == "warning") {
                    snackbar.showWarning(message, false, "engine");
                } else if (type == "success") {
                    snackbar.showSuccess(message, false, "engine");
                } else {
                    snackbar.showNote(message, false, "engine");
                }
            });

        workspace_ = buildWindows();
        // Detection runs on a thread of its own; its results are data, and data is written in
        // the frame loop. Installed before anything can start detecting.
        QaplaConfiguration::EngineCapabilities::setApplyChangeCallback(
            [](std::function<void()> change) {
                QaplaWindows::UiUpdateQueue::instance().post(std::move(change));
            });

        QaplaLlm::initializeLlmChat();

        // After initializeLlmChat(), which is what registers the tools and hooks the tool queue
        // into the frame loop -- the remote control serves exactly those and nothing of its own.
        // During an automated test run the chatbot window belongs to the tests, so the channel is
        // opened without its panel.
        if (remoteControl.enabled) {
            // A caller over HTTP is answered once the run has really started, and that waiting
            // sits on this thread -- see UiThreadWatch::REMOTE_CONTROL_STALL_THRESHOLD.
            QaplaWindows::UiThreadWatch::setStallThreshold(
                QaplaWindows::UiThreadWatch::REMOTE_CONTROL_STALL_THRESHOLD);
            if (QaplaLlm::startRemoteControl(remoteControl, !autoRunTests)) {
                // For a person reading a log. A program that has to know the port reads
                // remote-control.port in the configuration directory instead, which does not
                // depend on there being a console to write to -- see RemoteControlServer.
                std::cout << "QAPLA_REMOTE_CONTROL port="
                          << QaplaLlm::RemoteControlServer::instance().port() << "\n"
                          << std::flush;
            }
        }

        // Lets the AI-chatbot's close_application tool (see src/llm/actions/gui-action-app.cpp) quit
        // the app the same way the OS window-close button does -- the frontend leaves its loop on
        // its next check and runs the exact same shutdown sequence (including the final
        // StaticCallbacks::save().invokeAll() flush) rather than an abrupt process exit.
        quitCallbackHandle_ = QaplaWindows::StaticCallbacks::message().registerCallback(
            [this](const std::string& msg) {
                if (msg == "quit_application") {
                    requestQuit();
                }
            }
        );
    }

    AppRuntime::~AppRuntime() = default;

    void AppRuntime::beginFrame() {
        // From here to the end of the frame is work. The waiting before it is deliberate -- it
        // keeps the GUI from spinning -- and counting it would hide the thing being looked for: a
        // frame that takes long because something on this thread would not let go. See
        // QaplaWindows::UiThreadWatch.
        QaplaWindows::UiThreadWatch::instance().frameBegin();

        {
            // Data first: whatever another thread worked out since the last frame is written
            // into the GUI's data here, before anything reads it to draw. See UiUpdateQueue.
            QaplaWindows::UiThreadWatch::Section section("apply");
            QaplaWindows::UiUpdateQueue::instance().applyAll();
        }

        {
            QaplaWindows::UiThreadWatch::Section section("poll");
            QaplaWindows::StaticCallbacks::poll().invokeAll();
        }
    }

    void AppRuntime::draw() {
        workspace_.draw();
        QaplaWindows::SnackbarManager::instance().draw();
    }

    void AppRuntime::endFrame() {
        // Asked for over POST /shutdown, carried out here: the request arrives on a server
        // thread, and ending the application is the UI thread's job -- same flag, same
        // effect and same shutdown sequence as the window's close button.
        if (QaplaLlm::RemoteControlServer::instance().isShutdownRequested()) {
            requestQuit();
        }

        {
            QaplaWindows::UiThreadWatch::Section section("autosave");
            QaplaWindows::StaticCallbacks::autosave().invokeAll();
        }

        QaplaWindows::UiThreadWatch::instance().frameEnd();
    }

    void AppRuntime::shutdownBeforeWindowCloses() {
        // Before the windows go: a handler still waiting on the tool queue would be waiting on a
        // UI thread that is no longer running one.
        QaplaLlm::RemoteControlServer::instance().stop();

        // And before anything static goes: whatever the chatbot window is holding has to let go
        // of its callback registrations here, while the callback managers are still alive. A
        // subscription released during static destruction reaches a manager whose mutex has
        // already been destroyed -- which does not fail quietly, it aborts the process on the
        // way out. Torn down deliberately at a known point instead.
        QaplaWindows::ChatBot::ChatbotWindow::instance()->releaseThreads();
    }

    void AppRuntime::shutdownAfterWindowClosed() {
        quitCallbackHandle_.reset();
        GameManagerPool::getInstance().stopAll();
        {
            // On the way out, and still the user interface's thread: waiting for the games to
            // end is not work of its own -- see UiThreadWatch::Waiting.
            QaplaWindows::UiThreadWatch::Waiting waiting(
                std::string(QaplaWindows::UiThreadWatch::POOL_SECTION) + ":shutdown");
            GameManagerPool::getInstance().waitForTask();
        }
        // And then the managers themselves, which is what takes the engines with them: stopAll()
        // ends the games, but every engine keeps a worker thread that sends "quit" and writes
        // that down. Left to finish on their own, those threads were still logging while static
        // destruction had begun -- the process aborted on the way out with "Pure virtual function
        // called", reproducibly on Linux. shutdown() destroys the managers here, and destroying a
        // worker joins its threads.
        GameManagerPool::getInstance().shutdown();
        QaplaWindows::StaticCallbacks::save().invokeAll();
    }

} // namespace QaplaApp
