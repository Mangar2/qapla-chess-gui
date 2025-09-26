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
#include "tournament-data.h"
#include "tournament-window.h"
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

        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        auto* window = glfwCreateWindow(1400, 800, "Qapla Chess GUI", nullptr, nullptr);
        if (!window)
            throw std::runtime_error("Failed to create GLFW window");

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        return window;
    }

    void initGlad() {
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("Failed to initialize GLAD");
    }

    void initImGui(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui::GetStyle().Colors[ImGuiCol_BorderShadow] = ImVec4(0.25f, 0.28f, 0.32f, 0.40f);
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
        for (auto& instance : instances) {
            auto title = instance->getTitle();
            boardTabBar->addTab(title, std::move(instance), true);
        }
        boardTabBar->setAddTabCallback([&](QaplaWindows::ImGuiTabBar& tb) {
            auto instance = QaplaWindows::InteractiveBoardWindow::createInstance();
            auto tabName = instance->getTitle();
            tb.addTab(tabName, std::move(instance), true);
        });
        boardTabBar->setDynamicTabsCallback([&]() {
            QaplaWindows::TournamentData::instance().drawTabs();
        });

		auto taskTabBar = std::make_unique<QaplaWindows::ImGuiTabBar>();
        taskTabBar->addTab("Engines", std::make_unique<QaplaWindows::EngineSetupWindow>());
        taskTabBar->addTab("Clock", std::make_unique<QaplaWindows::TimeControlWindow>());
        
        auto tournamentWindow = std::make_unique<QaplaWindows::TournamentWindow>();
        tournamentWindow->setEngineConfigurationCallback([](const std::vector<QaplaWindows::ImGuiEngineSelect::EngineConfiguration>& configs) {
            QaplaWindows::TournamentData::instance().setEngineConfigurations(configs);
        });
        taskTabBar->addTab("Tournament", std::move(tournamentWindow));

        taskTabBar->addTab("Pgn", std::make_unique<QaplaWindows::ImGuiGameList>());
        
        // Setup EPD window with engine configuration callback
        auto epdWindow = std::make_unique<QaplaWindows::EpdWindow>();
        epdWindow->setEngineConfigurationCallback([](const std::vector<QaplaWindows::ImGuiEngineSelect::EngineConfiguration>& configs) {
            // Update EpdData with engine configurations
            QaplaWindows::EpdData::instance().setEngineConfigurations(configs);
        });
        taskTabBar->addTab("Epd", std::move(epdWindow));

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
        QaplaWindows::InteractiveBoardWindow::instance().setEngines();
        auto workspace = initWindows();

        auto* window = initGlfwContext();
        initGlad();
        initImGui(window);
        initBackgroundImage("assets/dark_wood_diff_4k.jpg");
        font::loadFonts();
        while (!glfwWindowShouldClose(window)) {
            if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE) {
                glfwWaitEvents(); 
                continue;
            }
            glfwPollEvents();

            int width{}, height{};
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            drawBackgroundImage();
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cerr << "OpenGL ERROR: " << std::hex << err << "\n";
            }


            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            QaplaWindows::TournamentData::instance().pollData();
            QaplaWindows::StaticCallbacks::poll().invokeAll();

            workspace.draw();
			SnackbarManager::instance().draw();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
            QaplaConfiguration::Configuration::instance().autosave();
        }

        shutdownImGui();
        glfwDestroyWindow(window);
        glfwTerminate();
        GameManagerPool::getInstance().stopAll();
        GameManagerPool::getInstance().waitForTask();
        QaplaConfiguration::Configuration::instance().saveFile();
        return 0;
    }

} // namespace

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>

bool attachToParentConsole() {
    // Versuche, sich an die Console des Elternprozesses anzuhängen
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // Umleitung von stdout, stdin, stderr zur bestehenden Console
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
        freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
        
        // Synchronisiere C++ streams mit C streams
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

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
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
