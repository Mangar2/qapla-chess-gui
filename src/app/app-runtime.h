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

#pragma once

#include "board-workspace.h"
#include "command-line.h"
#include "callback-manager.h"

#include <atomic>
#include <memory>

namespace QaplaApp {

    /**
     * @brief Says what the command line got wrong, before anything is opened.
     *
     * None of it stops the start: an option nobody recognised is worth a line on stderr, not a
     * refusal to run an application that is driven by hand anyway.
     */
    void reportCommandLineMessages(const CommandLineOptions& options);

    /**
     * @brief Sends everything this session stores to the directory --config-dir named, if it did.
     *
     * Called before the application starts, because the first setting is read the moment a window
     * is built and the answer has to be final by then.
     *
     * Unlike every other option, a bad value stops the start instead of being reported and
     * ignored. The others are settings, and a wrong setting still leaves a usable GUI; this one is
     * a promise that this session will not touch the configuration the user works with. Falling
     * back on that configuration is precisely the outcome the caller was ruling out.
     *
     * @return false when the directory could not be used, and the GUI must not start.
     */
    bool applyConfigDirectory(const CommandLineOptions& options);

    /** @brief Applies the options that only hold for this session, before anything is drawn. */
    void applySessionOptions(const CommandLineOptions& options);

    /**
     * @brief The application without its window: what every frontend runs the same way.
     *
     * A frontend owns the window, the event loop and the drawing toolkit. It creates one
     * AppRuntime, and in each frame calls beginFrame(), draw() and endFrame() in that order; the
     * runtime does the rest -- the data written in from other threads, the polling, the windows'
     * description through QaplaUi, saving, and the remote control's requests. When quitRequested()
     * turns true the frontend leaves its loop and calls shutdownBeforeWindowCloses(), closes its
     * window, and calls shutdownAfterWindowClosed().
     *
     * The order of the frame is the one the stall measurement (UiThreadWatch) and the HTTP
     * integration tests rely on: frameBegin, data, poll, draw ... autosave, frameEnd.
     */
    class AppRuntime {
    public:
        /**
         * @brief Loads the configuration, builds the windows and opens the remote control if asked.
         *
         * @param remoteControl What the command line said about the remote control.
         * @param autoRunTests True while the ImGui test engine runs its suites: the chatbot
         *                     panel then belongs to the tests.
         */
        AppRuntime(const QaplaLlm::RemoteControlOptions& remoteControl, bool autoRunTests);
        ~AppRuntime();

        AppRuntime(const AppRuntime&) = delete;
        AppRuntime& operator=(const AppRuntime&) = delete;

        /** @brief Starts a frame: the frame measurement, the data from other threads, the polls. */
        void beginFrame();

        /** @brief Describes the windows, through QaplaUi. */
        void draw();

        /** @brief Ends a frame: the remote shutdown request, autosave, the frame measurement. */
        void endFrame();

        /**
         * @brief Whether the application is to end: its window's close button, the AI's
         * close_application tool or POST /shutdown.
         */
        [[nodiscard]] bool quitRequested() const { return quitRequested_; }
        void requestQuit() { quitRequested_ = true; }

        /**
         * @brief Lets go of what must not outlive the window: the remote control and the
         * chatbot's threads.
         */
        void shutdownBeforeWindowCloses();

        /**
         * @brief Ends the games and engines and saves what the session stores.
         */
        void shutdownAfterWindowClosed();

        [[nodiscard]] QaplaWindows::BoardWorkspace& workspace() { return workspace_; }

    private:
        static QaplaWindows::BoardWorkspace buildWindows();

        QaplaWindows::BoardWorkspace workspace_;
        std::unique_ptr<QaplaWindows::Callback::UnregisterHandle> quitCallbackHandle_;
        std::atomic<bool> quitRequested_{ false };
    };

} // namespace QaplaApp
