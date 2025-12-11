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
#include "logger.h"
#include "game-manager-pool.h"

#include "configuration.h"
#include "configuration-window.h"
#include "time-control-window.h"
#include "epd-window.h"
#include "epd-data.h"
#include "viewer-board-window-list.h"
#include "engine-test-window.h"
#include "tournament-window.h"
#include "sprt-tournament-window.h"
#include "imgui-tab-bar.h"
#include "imgui-game-list.h"
#include "horizontal-split-container.h"
#include "vertical-split-container.h"
#include "board-workspace.h"
#include "engine-setup-window.h"
#include "snackbar.h"
#include "tutorial.h"
#include "callback-manager.h"
#include "data/dark-wood-background.h"
#include "font.h"
#include "background-renderer.h"
#include "test-system/test-manager.h"
#include "chatbot/chatbot-window.h"

#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include "imgui_te_engine.h"
#include "imgui_te_ui.h"
#endif

#ifndef _WIN32
#include <csignal>
#include <cstdlib>
#endif

using QaplaTester::Logger;
using QaplaTester::TraceLevel;
using QaplaTester::GameManagerPool;

namespace {

    /**
     * @brief Frame rate limiter to ensure consistent frame timing across platforms
     * 
     * VSync (glfwSwapInterval) doesn't work reliably on all platforms/drivers,
     * especially on Linux. This class provides a fallback frame rate limiter.
     */
    class FrameRateLimiter {
    public:
        explicit FrameRateLimiter(double targetFps = 60.0) 
            : targetFrameTime_(1.0 / targetFps)
            , lastFrameTime_(glfwGetTime()) {}

        void waitForNextFrame() {
            const double currentTime = glfwGetTime();
            const double deltaTime = currentTime - lastFrameTime_;
            
            if (deltaTime < targetFrameTime_) {
                const double sleepTime = targetFrameTime_ - deltaTime;
                std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
            }
            
            lastFrameTime_ = glfwGetTime();
        }

    private:
        double targetFrameTime_;
        double lastFrameTime_;
    };

   
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
        glfwSwapInterval(1);
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

    void initImGui(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
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

        auto boardTabBar = std::make_unique<QaplaWindows::ImGuiTabBar>();
        auto instances = QaplaWindows::InteractiveBoardWindow::loadInstances();
        size_t index = 0;
        boardTabBar->addTab("Chatbot", std::make_unique<QaplaWindows::ChatBot::ChatbotWindow>());
        for (auto& instance : instances) {
            auto title = instance->getTitle();
            boardTabBar->addTab(title, std::move(instance), 
                index == 0 ? ImGuiTabItemFlags_None : ImGuiTabItemFlags_NoAssumedClosure);
            index++;
        }
        boardTabBar->setAddTabCallback([&](QaplaWindows::ImGuiTabBar& tb) {
            auto instance = QaplaWindows::InteractiveBoardWindow::createInstance();
            auto title = instance->getTitle();
            tb.addTab(title, std::move(instance), ImGuiTabItemFlags_NoAssumedClosure | ImGuiTabItemFlags_SetSelected);
        });
        boardTabBar->setDynamicTabsCallback([&]() {
            QaplaWindows::ViewerBoardWindowList::drawAllTabs();
        });

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

    int runApp() {
        
        QaplaConfiguration::Configuration::instance().loadFile();
        QaplaConfiguration::Configuration::loadLoggerConfiguration();
        QaplaWindows::EpdData::instance().loadFile();
        QaplaWindows::Tutorial::instance().loadConfiguration();
        QaplaWindows::SnackbarManager::instance().loadConfiguration();
        auto workspace = initWindows();

        auto* window = initGlfwContext();
        initGlad();
        initImGui(window);
        try {
            // Load embedded background image
            initBackgroundImageFromMemory(darkwood, darkwoodSize);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to load background image: " << e.what() << "\n";
        }
        FontManager::loadFonts();
        
        QaplaTest::TestManager testManager;
        testManager.init();

        FrameRateLimiter frameRateLimiter(60.0);
        
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
            
            drawBackgroundImage();
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cerr << "OpenGL ERROR: " << std::hex << err << "\n";
            }


            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            QaplaWindows::StaticCallbacks::poll().invokeAll();

            workspace.draw();
			QaplaWindows::SnackbarManager::instance().draw();

            testManager.drawDebugWindows();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
            
            testManager.onPostSwap();

            QaplaWindows::StaticCallbacks::autosave().invokeAll();
            
            frameRateLimiter.waitForNextFrame();
        }

        testManager.stop();
        shutdownImGui();
        testManager.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
        GameManagerPool::getInstance().stopAll();
        GameManagerPool::getInstance().waitForTask();
        QaplaWindows::StaticCallbacks::save().invokeAll();
        return 0;
    }

} // namespace

#ifdef _WIN32
#include <windows.h>
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
        auto code = runApp();
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
int main() {
    // Ignore SIGPIPE to prevent crashes when writing to closed pipes (e.g., chess engines)
    std::signal(SIGPIPE, SIG_IGN);

    try {
        auto code = runApp();
        return code;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
#endif
