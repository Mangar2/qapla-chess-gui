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

#include "app/app-runtime.h"
#include "configuration.h"
#include "data/dark-wood-background.h"
#include "background-renderer.h"
#include "test-system/test-manager.h"
#include "data/logo-data.h"
#include "imgui-frame-rate-limiter.h"
#include "imgui-ui-backend.h"
#include "os-dialogs.h"
#include "os-helpers.h"
#include "command-line.h"
#include "version.h"
#include "ui-thread-watch.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <system_error>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "../extern/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../extern/stb/stb_image_write.h"

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

        const std::string title = "Qapla Chess GUI " + std::string(QaplaApp::VERSION);
        auto* window = glfwCreateWindow(1400, 800, title.c_str(), nullptr, nullptr);
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

    /**
     * @brief Writes the frame just rendered to a PNG file, once, if QAPLA_SCREENSHOT asks for it.
     *
     * For a developer checking what a frontend draws on a machine where nothing may take
     * pictures of the screen: QAPLA_SCREENSHOT names the file, QAPLA_SCREENSHOT_FRAME the frame
     * (default 60), and QAPLA_SCREENSHOT_QUIT=1 ends the application afterwards.
     *
     * @return true if the application is to end now.
     */
    bool saveScreenshotIfRequested(GLFWwindow* window, int frame) {
        static const auto path = QaplaHelpers::OsHelpers::getEnv("QAPLA_SCREENSHOT");
        if (!path) {
            return false;
        }
        static const int wantedFrame = [] {
            auto value = QaplaHelpers::OsHelpers::getEnv("QAPLA_SCREENSHOT_FRAME");
            return value ? std::stoi(*value) : 60;
        }();
        if (frame != wantedFrame) {
            return false;
        }
        int width{};
        int height{};
        glfwGetFramebufferSize(window, &width, &height);
        std::vector<unsigned char> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        stbi_flip_vertically_on_write(1);
        stbi_write_png(path->c_str(), width, height, 4, pixels.data(), width * 4);
        std::cout << "QAPLA_SCREENSHOT " << *path << "\n" << std::flush;
        return QaplaHelpers::OsHelpers::getEnv("QAPLA_SCREENSHOT_QUIT").has_value();
    }

    void shutdownImGui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    int runApp(const QaplaLlm::RemoteControlOptions& remoteControl) {

        // Read here because the runtime needs to know: during an automated test run the chatbot
        // window belongs to the tests, so the remote control is opened without its panel.
        const bool autoRunTests = QaplaHelpers::OsHelpers::getEnv("QAPLA_AUTO_RUN_TESTS").has_value();

        QaplaApp::AppRuntime runtime(remoteControl, autoRunTests);

        auto* window = initGlfwContext();
        initGlad();
        setWindowIcon(window);
        initImGui(window);
#ifdef _WIN32
        // The file dialogs belong to this window: they stay in front of it and block only it.
        QaplaWindows::OsDialogs::setNativeWindowProvider(
            [window]() -> void* { return glfwGetWin32Window(window); });
#endif

        try {
            // Load embedded background image
            initBackgroundImageFromMemory(darkwood, darkwoodSize);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to load background image: " << e.what() << "\n";
        }
        QaplaImGui::loadFonts();
        
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
        if (autoRunTests) {
            // Printed the moment it happens, not summed up at the end: what a test run writes
            // around it says which test was on screen, and that is half the answer.
            QaplaWindows::UiThreadWatch::instance().setStallCallback(
                [](const QaplaWindows::UiThreadWatch::Stall& stall) {
                    std::cout << "QAPLA_STALL frame=" << stall.frame
                        << " elapsed=" << stall.elapsedMs
                        << " work=" << stall.frameMs
                        << " waited=" << stall.waitedMs
                        << " unnamed=" << stall.unnamedMs
                        << " residual=" << stall.residualMs
                        << " section=" << stall.section;
                    // Own time first, then the time including whatever ran nested inside: the
                    // first says who spent it, the second where it sat.
                    for (const auto& section : stall.sections) {
                        std::cout << " " << section.name << "=" << section.time.selfMs
                            << "/" << section.time.totalMs;
                    }
                    std::cout << std::endl; // NOLINT(performance-avoid-endl) -- flushed on purpose
                });
        }

        int autoRunFrameCount = 0;
        int frameCount = 0;

        // What runApp() returns when the tests were run: only an all-green run is a zero, so a
        // release script can gate on the exit code instead of reading stdout.
        int autoRunExitCode = 0;

        bool remoteDesktopMode = QaplaConfiguration::Configuration::isRemoteDesktopMode();
        auto frameRateLimiter = ImGuiFrameRateLimiter::forMode(remoteDesktopMode);
        
        std::cout << (remoteDesktopMode ? "Remote Desktop mode - " : "Normal mode - ")
                  << frameRateLimiter.getModeDescription() << "\n";
        
        while (glfwWindowShouldClose(window) == 0 && !runtime.quitRequested()) {
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

            runtime.beginFrame();

            {
                QaplaWindows::UiThreadWatch::Section section("draw");
                runtime.draw();
                testManager.drawDebugWindows();
            }

            {
                QaplaWindows::UiThreadWatch::Section section("render");
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            if (saveScreenshotIfRequested(window, ++frameCount)) {
                runtime.requestQuit();
            }

            {
                // Timed apart from the drawing above, and taken out of the frame: this is where
                // the window system decides when the frame is over. See UiThreadWatch::Waiting.
                QaplaWindows::UiThreadWatch::Waiting waiting(
                    QaplaWindows::UiThreadWatch::SWAP_SECTION);
                glfwSwapBuffers(window);
            }
            
            {
                // Named like everything else in the frame: this is where the ImGui test engine
                // does its own work, and it can hold the thread for a while. It used to run
                // outside every section, so its time landed in the frame with no name on it --
                // and the frame was then reported against the longest section that did have one.
                QaplaWindows::UiThreadWatch::Section section("test-engine");
                testManager.onPostSwap();
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
                    // What the run cost the UI thread, in the run's own output: the frame report
                    // was only readable through the remote control, which goes away with the
                    // process -- so a finished run said nothing about how it had behaved.
                    const auto frames = QaplaWindows::UiThreadWatch::instance().report();
                    std::cout << "QAPLA_FRAME_REPORT frames=" << frames.frames
                        << " stalls=" << frames.stalls
                        << " elapsedMs=" << frames.elapsedMs
                        << " workMs=" << frames.workMs
                        << " waitedMs=" << frames.waitedMs
                        << " worstFrameMs=" << frames.worstFrameMs
                        << " worstFrameWaitedMs=" << frames.worstFrameWaitedMs
                        << " worstFrameUnnamedMs=" << frames.worstFrameUnnamedMs
                        << " worstFrameResidualMs=" << frames.worstFrameResidualMs
                        << " worstSection=" << (frames.worstSection.empty() ? "-" : frames.worstSection)
                        << "\n";
                    // The worst frame broken down: one name is an address, the breakdown is the
                    // proof -- and it shows how much of the frame no section claimed at all.
                    if (!frames.worstFrameSections.empty()) {
                        std::cout << "QAPLA_FRAME_SECTIONS";
                        for (const auto& section : frames.worstFrameSections) {
                            std::cout << " " << section.name << "=" << section.time.selfMs
                                << "/" << section.time.totalMs;
                        }
                        std::cout << "\n";
                    }
                    // Nothing tested is not a pass, it is a run that never happened -- a suite
                    // that registers no test at all has to be as loud as a failing one.
                    autoRunExitCode = (tested > 0 && success == tested) ? 0 : 1;
                    runtime.requestQuit();
                }
            }

            runtime.endFrame();
        }

        runtime.shutdownBeforeWindowCloses();

        testManager.stop();
        shutdownImGui();
        testManager.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
        runtime.shutdownAfterWindowClosed();
        return autoRunExitCode;
    }

} // namespace

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>

namespace {

/** @brief Whether a standard handle already leads somewhere: a pipe, or a file. */
[[nodiscard]] bool isRedirected(DWORD standardHandle) {
    const HANDLE handle = GetStdHandle(standardHandle);
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    const DWORD type = GetFileType(handle);
    return type == FILE_TYPE_PIPE || type == FILE_TYPE_DISK;
}

} // namespace

/**
 * @brief Gives a windowed application a console to print to, when nobody else is listening.
 *
 * Whatever the caller redirected stays redirected. Started from a test runner, the standard
 * handles are pipes the runner reads, and pointing them at CONOUT$ instead threw every line of
 * the run at a console nobody was looking at: the test summary, the test log, the frame report.
 * The runner saw an empty stream and could not tell a passing run from a failing one -- on
 * Windows the whole GUI suite reported nothing at all.
 *
 * @return true if a console was attached and has to be freed on the way out.
 */
bool attachToParentConsole() {
    const bool outputRedirected = isRedirected(STD_OUTPUT_HANDLE);
    const bool errorRedirected = isRedirected(STD_ERROR_HANDLE);
    if (outputRedirected && errorRedirected) {
        return false;
    }
    if (AttachConsole(ATTACH_PARENT_PROCESS) == 0) {
        return false;
    }

    FILE* reopened = nullptr;
    if (!outputRedirected) {
        freopen_s(&reopened, "CONOUT$", "w", stdout);
    }
    if (!errorRedirected) {
        freopen_s(&reopened, "CONOUT$", "w", stderr);
    }
    if (!isRedirected(STD_INPUT_HANDLE)) {
        freopen_s(&reopened, "CONIN$", "r", stdin);
    }

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

int APIENTRY WinMain([[maybe_unused]] HINSTANCE hInstance, 
    [[maybe_unused]] HINSTANCE hPrevInstance, 
    [[maybe_unused]] LPSTR lpCmdLine, 
    [[maybe_unused]] int nShowCmd) 
{
    const bool hasConsole = attachToParentConsole();
    // Two different questions, and they used to share one answer. attachToParentConsole() says
    // whether a console was attached and therefore has to be freed again; it says nothing about
    // whether anyone can read what the process prints. A caller that redirected stdout is read
    // by whoever set up that pipe -- and is precisely the caller for whom no console is
    // attached, since redirected handles are left alone.
    const bool canPrint = hasConsole || isRedirected(STD_OUTPUT_HANDLE);

    try {
        // WinMain hands the command line over as one unsplit string; __argc/__argv are the same
        // arguments already tokenized by the CRT, which is what the parser wants.
        auto options = QaplaApp::parseCommandLine(__argc, __argv);
        QaplaApp::reportCommandLineMessages(options);

        if (options.helpRequested) {
            // Anywhere the output can be read -- a console, or a pipe the caller set up -- the
            // help is written there. Double-clicked there is neither, and a message box is the
            // only place the answer can be read at all. Printing into a redirect matters beyond
            // tidiness: a message box in a session with no screen waits for a click that nobody
            // can give, which is a hung process rather than an answer.
            if (canPrint) {
                std::cout << QaplaApp::helpText() << std::flush;
                if (hasConsole) {
                    FreeConsole();
                }
            } else {
                MessageBoxA(nullptr, QaplaApp::helpText().c_str(), "Qapla Chess GUI",
                    MB_ICONINFORMATION | MB_OK);
            }
            return 0;
        }

        if (!QaplaApp::applyConfigDirectory(options)) {
            if (hasConsole) {
                FreeConsole();
            }
            return 1;
        }
        QaplaApp::applySessionOptions(options);

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
        QaplaApp::reportCommandLineMessages(options);

        if (options.helpRequested) {
            std::cout << QaplaApp::helpText() << std::flush;
            return 0;
        }

        if (!QaplaApp::applyConfigDirectory(options)) {
            return 1;
        }
        QaplaApp::applySessionOptions(options);

        auto code = runApp(options.remoteControl);
        return code;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
#endif
