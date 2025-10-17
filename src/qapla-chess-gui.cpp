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
#include "qapla-tester/logger.h"
#include "qapla-tester/game-manager-pool.h"

#include "configuration.h"
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
#include "callback-manager.h"

#include <iostream>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "font.h"
#include "background-renderer.h"


namespace {


   
    void glfwErrorCallback(int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << '\n';
    }

    GLFWwindow* initGlfwContext() {
        glfwSetErrorCallback(glfwErrorCallback);

        if (glfwInit() == 0) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

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
        taskTabBar->addTab("Clock", std::make_unique<QaplaWindows::TimeControlWindow>());
        taskTabBar->addTab("Tournament", std::make_unique<QaplaWindows::TournamentWindow>());
        taskTabBar->addTab("Sprt", std::make_unique<QaplaWindows::SprtTournamentWindow>());
        taskTabBar->addTab("Pgn", std::make_unique<QaplaWindows::ImGuiGameList>());
        taskTabBar->addTab("Epd", std::make_unique<QaplaWindows::EpdWindow>());
        taskTabBar->addTab("Test", std::make_unique<QaplaWindows::EngineTestWindow>());

        auto mainContainer = std::make_unique<QaplaWindows::HorizontalSplitContainer>(
            "main", ImGuiWindowFlags_None);
		mainContainer->setRight(std::move(boardTabBar));
		mainContainer->setLeft(std::move(taskTabBar));

        workspace.setRootWindow(std::move(mainContainer));
        return workspace;
    }

    void initLogging() {
        Logger::setLogPath("./log");
        Logger::testLogger().setLogFile("report");
        Logger::testLogger().setTraceLevel(TraceLevel::error, TraceLevel::info);
        Logger::engineLogger().setLogFile("enginelog");
        Logger::engineLogger().setTraceLevel(TraceLevel::error, TraceLevel::info);
    }

    int runApp() {
        
        initLogging();
        QaplaConfiguration::Configuration::instance().loadFile();
        QaplaWindows::EpdData::instance().loadFile();
        QaplaWindows::InteractiveBoardWindow::instance().setEngines();
        auto workspace = initWindows();

        auto* window = initGlfwContext();
        initGlad();
        initImGui(window);
        try {
            initBackgroundImage("assets/dark_wood_diff_4k.jpg");
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to load background image: " << e.what() << "\n";
        }
        font::loadFonts();
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
			SnackbarManager::instance().draw();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
            QaplaConfiguration::Configuration::instance().autosave();
            QaplaWindows::EpdData::instance().autosave();
        }

        shutdownImGui();
        glfwDestroyWindow(window);
        glfwTerminate();
        GameManagerPool::getInstance().stopAll();
        GameManagerPool::getInstance().waitForTask();
        QaplaConfiguration::Configuration::instance().saveFile();
        QaplaWindows::EpdData::instance().saveFile();
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

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
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
