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
 * @author Volker B�hm
 * @copyright Copyright (c) 2025 Volker B�hm
 */

#include "board-data.h"
#include "qapla-tester/logger.h"

#include "configuration.h"
#include "time-control-window.h"
#include "board-window.h"
#include "move-list-window.h"
#include "engine-window.h"
#include "clock-window.h"
#include "epd-window.h"
#include "tournament-data.h"
#include "tournament-window.h"
#include "imgui-tab-bar.h"
#include "horizontal-split-container.h"
#include "vertical-split-container.h"
#include "board-workspace.h"
#include "engine-setup-window.h"
#include "snackbar.h"

#include <iostream>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "font.h"

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

        auto& boardData = QaplaWindows::BoardData::instance();

        auto ClockMovesContainer = std::make_unique<QaplaWindows::VerticalSplitContainer>();
        ClockMovesContainer->setFixedTopHeight(120.0f);
        ClockMovesContainer->setTop(std::make_unique<QaplaWindows::ClockWindow>());
        ClockMovesContainer->setBottom(std::make_unique<QaplaWindows::MoveListWindow>());

        auto BoardMovesContainer = std::make_unique<QaplaWindows::HorizontalSplitContainer>();
        BoardMovesContainer->setLeft(std::make_unique<QaplaWindows::BoardWindow>());
        BoardMovesContainer->setRight(std::move(ClockMovesContainer));

        auto BoardEngineContainer = std::make_unique<QaplaWindows::VerticalSplitContainer>();
        BoardEngineContainer->setTop(std::move(BoardMovesContainer));
        BoardEngineContainer->setBottom(std::make_unique<QaplaWindows::EngineWindow>());

		auto tabBar = std::make_unique<QaplaWindows::ImGuiTabBar>();
        tabBar->addTab("Engines", std::make_unique<QaplaWindows::EngineSetupWindow>());
        tabBar->addTab("Clock", std::make_unique<QaplaWindows::TimeControlWindow>());
		tabBar->addTab("Tournament", std::make_unique<QaplaWindows::TournamentWindow>());
        tabBar->addTab("Epd", std::make_unique<QaplaWindows::EpdWindow>());

        auto mainContainer = std::make_unique<QaplaWindows::HorizontalSplitContainer>();
		mainContainer->setRight(std::move(BoardEngineContainer));
		mainContainer->setLeft(std::move(tabBar));


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

        auto workspace = initWindows();

        auto* window = initGlfwContext();
        initGlad();
        initImGui(window);
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

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

  			QaplaWindows::BoardData::instance().pollData();
            QaplaWindows::TournamentData::instance().pollData();

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
        return 0;
    }

} // namespace

#ifdef _WIN32
#include <windows.h>
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    try {
        try {
            auto code = runApp();
            QaplaConfiguration::Configuration::instance().saveFile();
            return code;
        }
        catch (...) {
            QaplaConfiguration::Configuration::instance().saveFile();
			throw; 
        }
    }
    catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Fatal Error", MB_ICONERROR | MB_OK);
        return 1;
    }
}
#else
int main() {
    try {
        try {
            auto code = runApp();
            QaplaConfiguration::Configuration::instance().saveFile();
            return code;
        }
        catch (...) {
            QaplaConfiguration::Configuration::instance().saveFile();
			throw; 
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
#endif
