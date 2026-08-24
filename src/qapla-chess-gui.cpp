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

#include "interactive-board-window.h"
#include <base-elements/logger.h>
#include <game-manager/game-manager-pool.h>

#include "configuration.h"
#include "config-group-loader.h"
#include "configuration-window.h"
#include "time-control-window.h"
#include "epd-window.h"
#include "epd-data.h"
#include "viewer-board-window-list.h"
#include "engine-test-window.h"
#include "imgui-board-tab-bar.h"
#include "tournament-window.h"
#include "sprt-tournament-window.h"
#include "imgui-tab-bar.h"
#include "imgui-game-list.h"
#include "horizontal-split-container.h"
#include "vertical-split-container.h"
#include "board-workspace.h"
#include "engine-setup-window.h"
#include "snackbar.h"
#include <engine-handling/engine-capabilities.h>
#include "tutorial.h"
#include "callback-manager.h"
#include "data/dark-wood-background.h"
#include "font.h"
#include "background-renderer.h"
#include "test-system/test-manager.h"
#include "chatbot/chatbot-window.h"
#include "llm/remote-control-integration.h"
#include "llm/llm-chat-integration.h"
#include "llm/remote-control-server.h"
#include "data/logo-data.h"
#include "imgui-frame-rate-limiter.h"
#include "os-helpers.h"
#include "command-line.h"
#include "ui-thread-watch.h"
#include "ui-update-queue.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "../extern/stb/stb_image.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include "imgui_te_engine.h"
#include "imgui_te_ui.h"
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#include <cstdlib>
#endif

using QaplaTester::Logger;
using QaplaTester::TraceLevel;
using QaplaTester::GameManagerPool;
using QaplaWindows::ImGuiFrameRateLimiter;

namespace {

    void glfwErrorCallback(int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << '\n';
    }

    GLFWwindow* initGlfwContext() {
        glfwSetErrorCallback(glfwErrorCallback);

#ifndef _WIN32
        // Force X11 backend on Linux to ensure window decorations work properly
        // Must be set before glfwInit() is called
        setenv("GDK_BACKEND", "x11", 1);  // 1 = overwrite existing value
        // Also prevent GLFW from using Wayland
        setenv("GLFW_IM_MODULE", "none", 0);
        unsetenv("WAYLAND_DISPLAY");  // Force GLFW to use X11
#endif
        
        if (glfwInit() == 0) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        // Ensure all window decorations are enabled (including minimize button)
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);

        auto* window = glfwCreateWindow(1400, 800, "Qapla Chess GUI", nullptr, nullptr);
        if (window == nullptr) {
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(window);
        
        // VSync is counterproductive in Remote Desktop scenarios
        // as it adds latency on top of network latency
        // Check configuration setting instead of auto-detection
        if (QaplaConfiguration::Configuration::isRemoteDesktopMode()) {
            glfwSwapInterval(0); // Disable VSync for RDP
            std::cout << "Remote Desktop mode enabled (from config) - VSync disabled\n";
        } else {
            glfwSwapInterval(1);
        }
        
        return window;
    }

    void initGlad() {
        if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
            throw std::runtime_error("Failed to initialize GLAD");
        }
        
        // Debug: Check OpenGL renderer (GPU vs Software rendering)
        const GLubyte* renderer = glGetString(GL_RENDERER);
        const GLubyte* version = glGetString(GL_VERSION);
        std::cout << "OpenGL Renderer: " << renderer << std::endl;
        std::cout << "OpenGL Version: " << version << std::endl;
    }

    /**
     * @brief Sets the application icon for the window
     * 
     * Uses embedded logo data (40x40 pixels). GLFW handles platform differences:
     * - Windows: Sets taskbar icon (resources.rc only affects file explorer)
     * - Linux/X11: Sets _NET_WM_ICON property
     * - Wayland/macOS: Handled by GLFW appropriately
     */
    void setWindowIcon(GLFWwindow* window) {
        int width{};
        int height{};
        int channels{};
        
        // Decode from embedded binary data (logo.png compiled into executable)
        // IMPORTANT: Force 4 channels (RGBA) as required by GLFW
        unsigned char* pixels = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(logopng),
            static_cast<int>(logopngSize),
            &width, &height, &channels, 4
        );
        
        if (pixels != nullptr) {
            GLFWimage icon;
            icon.width = width;
            icon.height = height;
            icon.pixels = pixels;
            
            glfwSetWindowIcon(window, 1, &icon);
            
            stbi_image_free(pixels);
        } else {
            std::cerr << "ERROR: Failed to decode window icon from embedded data\n";
            std::cerr << "STB Error: " << stbi_failure_reason() << "\n";
        }
    }

    void initImGui(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // The window layout is session state like everything else, but ImGui writes it to
        // "imgui.ini" next to the working directory rather than to our configuration directory.
        // Left that way for an ordinary start -- moving it would lose the layout people already
        // have -- and redirected when --config-dir asked for a directory of its own, or a test run
        // would still write into whatever directory it happened to be started from. The string has
        // to outlive the context: ImGui keeps the pointer, not a copy.
        static std::string imGuiIniPath;
        auto configDirectory = QaplaHelpers::OsHelpers::configDirectoryOverride();
        if (!configDirectory.empty()) {
            imGuiIniPath = (std::filesystem::path(configDirectory) / "imgui.ini").string();
            ImGui::GetIO().IniFilename = imGuiIniPath.c_str();
        }

        ImGui::StyleColorsDark();
        ImGui::GetStyle().Colors[ImGuiCol_BorderShadow] = ImVec4(0.25F, 0.28F, 0.32F, 0.40F);
        //ImGui::StyleColorsClassic();
		//ImGui::StyleColorsLight();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    void shutdownImGui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    QaplaWindows::BoardWorkspace initWindows() {

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
            "main", ImGuiWindowFlags_None);
		mainContainer->setRight(std::move(boardTabBar));
		mainContainer->setLeft(std::move(taskTabBar));
        mainContainer->setPresetWidth(400.0F, true);

        workspace.setRootWindow(std::move(mainContainer));
        return workspace;
    }

    int runApp(const QaplaLlm::RemoteControlOptions& remoteControl) {

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

        auto workspace = initWindows();
        // Detection runs on a thread of its own; its results are data, and data is written in
        // the frame loop. Installed before anything can start detecting.
        QaplaConfiguration::EngineCapabilities::setApplyChangeCallback(
            [](std::function<void()> change) {
                QaplaWindows::UiUpdateQueue::instance().post(std::move(change));
            });

        QaplaLlm::initializeLlmChat();

        // After initializeLlmChat(), which is what registers the tools and hooks the tool queue
        // into the frame loop -- the remote control serves exactly those and nothing of its own.
        // Read here because the remote control below needs to know: during an automated test run
        // the chatbot window belongs to the tests, so the channel is opened without its panel.
        const bool autoRunTests = QaplaHelpers::OsHelpers::getEnv("QAPLA_AUTO_RUN_TESTS").has_value();

        if (remoteControl.enabled) {
            if (QaplaLlm::startRemoteControl(remoteControl, !autoRunTests)) {
                // For a person reading a log. A program that has to know the port reads
                // remote-control.port in the configuration directory instead, which does not
                // depend on there being a console to write to -- see RemoteControlServer.
                std::cout << "QAPLA_REMOTE_CONTROL port="
                          << QaplaLlm::RemoteControlServer::instance().port() << "\n"
                          << std::flush;
            }
        }

        auto* window = initGlfwContext();
        initGlad();
        setWindowIcon(window);
        initImGui(window);

        // Lets the AI-chatbot's close_application tool (see src/llm/actions/gui-action-app.cpp) quit
        // the app the same way the OS window-close button does -- setting this flag makes the
        // main loop below exit normally on its next check, running the exact same shutdown
        // sequence (including the final StaticCallbacks::save().invokeAll() flush) rather than
        // an abrupt process exit. `window` has no other accessor outside this function, so the
        // subscription is registered here, where it's in scope, and kept alive for the whole
        // loop below.
        auto quitCallbackHandle = QaplaWindows::StaticCallbacks::message().registerCallback(
            [window](const std::string& msg) {
                if (msg == "quit_application") {
                    glfwSetWindowShouldClose(window, 1);
                }
            }
        );
        try {
            // Load embedded background image
            initBackgroundImageFromMemory(darkwood, darkwoodSize);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to load background image: " << e.what() << "\n";
        }
        FontManager::loadFonts();
        
        QaplaTest::TestManager testManager;
        testManager.init();

        // Headless/CI support: when QAPLA_AUTO_RUN_TESTS is set, queue all registered
        // ImGui Test Engine suites, print a summary once they finish, and exit.
        if (autoRunTests) {
            if (QaplaHelpers::OsHelpers::configDirectoryOverride().empty()) {
                // Not refused, because that would break every way these tests are started today,
                // but not passed over in silence either: this run reads the settings of the last
                // real session and writes its own over them when it ends.
                std::cerr << "WARNING: the tests are running against the configuration in "
                          << QaplaHelpers::OsHelpers::getConfigDirectory()
                          << " and will overwrite it. Pass --config-dir=<path> to keep them "
                             "apart.\n";
            }
            testManager.queueAllTests();
        }
        int autoRunFrameCount = 0;

        // What runApp() returns when the tests were run: only an all-green run is a zero, so a
        // release script can gate on the exit code instead of reading stdout.
        int autoRunExitCode = 0;

        bool remoteDesktopMode = QaplaConfiguration::Configuration::isRemoteDesktopMode();
        auto frameRateLimiter = ImGuiFrameRateLimiter::forMode(remoteDesktopMode);
        
        std::cout << (remoteDesktopMode ? "Remote Desktop mode - " : "Normal mode - ")
                  << frameRateLimiter.getModeDescription() << "\n";
        
        while (glfwWindowShouldClose(window) == 0) {
            if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE) {
                glfwWaitEvents(); 
                continue;
            }
            glfwPollEvents();

            int width{};
            int height{};
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);
            glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
            
            // Skip background image in Remote Desktop for better performance
            if (!remoteDesktopMode) {
                drawBackgroundImage();
            }
            
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cerr << "OpenGL ERROR: " << std::hex << err << "\n";
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Frame rate limiter needs to be called after ImGui::NewFrame()
            // so it can access ImGuiIO for activity detection
            frameRateLimiter.waitForNextFrame();

            // From here to the end of the loop body is work. The waiting above is deliberate --
            // it keeps the GUI from spinning -- and counting it would hide the thing being
            // looked for: a frame that takes long because something on this thread would not let
            // go. See QaplaWindows::UiThreadWatch.
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

            {
                QaplaWindows::UiThreadWatch::Section section("draw");
                workspace.draw();
                QaplaWindows::SnackbarManager::instance().draw();
                testManager.drawDebugWindows();
            }

            {
                QaplaWindows::UiThreadWatch::Section section("render");
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glfwSwapBuffers(window);
            }
            
            testManager.onPostSwap();

            // Asked for over POST /shutdown, carried out here: the request arrives on a server
            // thread, and ending the application is the UI thread's job -- same flag, same
            // effect and same shutdown sequence as the window's close button.
            if (QaplaLlm::RemoteControlServer::instance().isShutdownRequested()) {
                glfwSetWindowShouldClose(window, 1);
            }

            if (autoRunTests) {
                ++autoRunFrameCount;
                if (autoRunFrameCount > 5 && testManager.isQueueEmpty()) {
                    int tested = 0;
                    int success = 0;
                    int inQueue = 0;
                    testManager.getResultSummary(tested, success, inQueue);
                    std::cout << "QAPLA_TEST_SUMMARY tested=" << tested
                        << " success=" << success << " inQueue=" << inQueue << "\n";
                    // Nothing tested is not a pass, it is a run that never happened -- a suite
                    // that registers no test at all has to be as loud as a failing one.
                    autoRunExitCode = (tested > 0 && success == tested) ? 0 : 1;
                    glfwSetWindowShouldClose(window, 1);
                }
            }

            {
                QaplaWindows::UiThreadWatch::Section section("autosave");
                QaplaWindows::StaticCallbacks::autosave().invokeAll();
            }

            QaplaWindows::UiThreadWatch::instance().frameEnd();
        }

        // Before the windows go: a handler still waiting on the tool queue would be waiting on a
        // UI thread that is no longer running one.
        QaplaLlm::RemoteControlServer::instance().stop();

        // And before anything static goes: whatever the chatbot window is holding has to let go
        // of its callback registrations here, while the callback managers are still alive. A
        // subscription released during static destruction reaches a manager whose mutex has
        // already been destroyed -- which does not fail quietly, it aborts the process on the
        // way out. Torn down deliberately at a known point instead.
        QaplaWindows::ChatBot::ChatbotWindow::instance()->releaseThreads();

        testManager.stop();
        shutdownImGui();
        testManager.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
        GameManagerPool::getInstance().stopAll();
        GameManagerPool::getInstance().waitForTask();
        QaplaWindows::StaticCallbacks::save().invokeAll();
        return autoRunExitCode;
    }

    /**
     * @brief Says what the command line got wrong, before anything is opened.
     *
     * None of it stops the start: an option nobody recognised is worth a line on stderr, not a
     * refusal to run an application that is driven by hand anyway.
     */
    void reportCommandLineMessages(const QaplaApp::CommandLineOptions& options) {
        for (const auto& message : options.messages) {
            std::cerr << message << '\n';
        }
    }

    /**
     * @brief Sends everything this session stores to the directory --config-dir named, if it did.
     *
     * Called before runApp(), because the first setting is read the moment a window is built and
     * the answer has to be final by then.
     *
     * Unlike every other option, a bad value stops the start instead of being reported and
     * ignored. The others are settings, and a wrong setting still leaves a usable GUI; this one is
     * a promise that this session will not touch the configuration the user works with. Falling
     * back on that configuration is precisely the outcome the caller was ruling out.
     *
     * @return false when the directory could not be used, and the GUI must not start.
     */
    bool applyConfigDirectory(const QaplaApp::CommandLineOptions& options) {
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

} // namespace

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>

bool attachToParentConsole() {
    if (AttachConsole(ATTACH_PARENT_PROCESS) != 0) {
        // Redirect the CRT standard input, output, and error handles to the console
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
        freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
        
        // Synchronize C++ streams with C streams
        std::ios::sync_with_stdio(true);
        std::wcout.clear();
        std::cout.clear();
        std::wcerr.clear();
        std::cerr.clear();
        std::wcin.clear();
        std::cin.clear();
        
        return true;
    }
    return false;
}

int APIENTRY WinMain([[maybe_unused]] HINSTANCE hInstance, 
    [[maybe_unused]] HINSTANCE hPrevInstance, 
    [[maybe_unused]] LPSTR lpCmdLine, 
    [[maybe_unused]] int nShowCmd) 
{
    bool hasConsole = attachToParentConsole();

    try {
        // WinMain hands the command line over as one unsplit string; __argc/__argv are the same
        // arguments already tokenized by the CRT, which is what the parser wants.
        auto options = QaplaApp::parseCommandLine(__argc, __argv);
        reportCommandLineMessages(options);

        if (options.helpRequested) {
            // Started from a console, the help belongs in it. Double-clicked there is no console
            // to write to, and a message box is the only place the answer can be read at all.
            if (hasConsole) {
                std::cout << QaplaApp::helpText() << std::flush;
                FreeConsole();
            } else {
                MessageBoxA(nullptr, QaplaApp::helpText().c_str(), "Qapla Chess GUI",
                    MB_ICONINFORMATION | MB_OK);
            }
            return 0;
        }

        if (!applyConfigDirectory(options)) {
            if (hasConsole) {
                FreeConsole();
            }
            return 1;
        }

        auto code = runApp(options.remoteControl);
        if (hasConsole) {
            FreeConsole();
        }
        return code;
    }
    catch (const std::exception& e) {
        if (hasConsole) {
            std::cerr << "Fatal error: " << e.what() << '\n';
        }
        MessageBoxA(nullptr, e.what(), "Fatal Error", MB_ICONERROR | MB_OK);
        if (hasConsole) {
            FreeConsole();
        }
        return 1;
    }
}
#else
int main(int argc, char** argv) {
    // Ignore SIGPIPE to prevent crashes when writing to closed pipes (e.g., chess engines)
    std::signal(SIGPIPE, SIG_IGN);

    try {
        auto options = QaplaApp::parseCommandLine(argc, argv);
        reportCommandLineMessages(options);

        if (options.helpRequested) {
            std::cout << QaplaApp::helpText() << std::flush;
            return 0;
        }

        if (!applyConfigDirectory(options)) {
            return 1;
        }

        auto code = runApp(options.remoteControl);
        return code;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
#endif
