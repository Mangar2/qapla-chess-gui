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

#include "qapla-tester/game-state.h"
#include "qapla-tester/game-record.h"
#include "board-data.h"
#include "qapla-tester/compute-task.h"
#include "qapla-tester/engine-worker-factory.h"
#include "qapla-tester/engine-config-manager.h"

#include "board-window.h"
#include "move-list-window.h"
#include "engine-window.h"
#include "clock-window.h"
#include "horizontal-split-container.h"
#include "vertical-split-container.h"
#include "board-workspace.h"

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

        auto* window = glfwCreateWindow(800, 800, "Qapla Chess GUI", nullptr, nullptr);
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
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    void shutdownImGui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void executeCommand(ComputeTask& compute, const std::string& command) {
        if (command == "Stop") {
            compute.stop();
        } else if (command == "Newgame") {
            compute.newGame();
        } else if (command == "Play") {
            compute.computeMove();
        } else if (command == "Analyze") {
            //compute.analyze();
        } else if (command == "Auto") {
            compute.autoPlay();
        } else if (command == "Manual") {
            compute.stop();
        } else {
            std::cerr << "Unknown command: " << command << '\n';
        }
	}

    int runApp() {
        ComputeTask compute;
        EngineConfigManager configManager;
        configManager.loadFromFile("./test/engines.ini");
		EngineWorkerFactory::setConfigManager(configManager);
        auto config = EngineWorkerFactory::getConfigManager().getConfig("Qapla 0.4.0");
		auto engines = EngineWorkerFactory::createEngines(*config, 2);
        compute.initEngines(std::move(engines));
		TimeControl timeControl;
        timeControl.addTimeSegment({ 0, 1000000, 1000 }); 
        //timeControl.addTimeSegment({ 0, 1000, 10 }); 
		compute.setTimeControl(timeControl);
        compute.setPosition(true);
		compute.autoPlay(true);
		auto boardData = std::make_shared<QaplaWindows::BoardData>();
                        
        QaplaWindows::BoardWorkspace workspace;
        workspace.maximize(true);
        auto vSplitContainer = std::make_unique<QaplaWindows::VerticalSplitContainer>();
        auto hSplitContainer = std::make_unique<QaplaWindows::HorizontalSplitContainer>();
        auto vSplitRight = std::make_unique<QaplaWindows::VerticalSplitContainer>();
		auto boardWindow = std::make_unique<QaplaWindows::BoardWindow>(boardData);
        boardWindow->setExecuteCallback([&compute](const char* command) {
            executeCommand(compute, command);
			});
        hSplitContainer->setLeft(std::move(boardWindow));
		vSplitRight->setTop(std::make_unique<QaplaWindows::ClockWindow>(boardData));
        vSplitRight->setBottom(std::make_unique<QaplaWindows::MoveListWindow>(boardData));
		vSplitRight->setFixedTopHeight(120.0f);
		hSplitContainer->setRight(std::move(vSplitRight));
        vSplitContainer->setTop(std::move(hSplitContainer));
        vSplitContainer->setBottom(std::make_unique<QaplaWindows::EngineWindow>(boardData));
		workspace.setRootWindow(std::move(vSplitContainer));

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

            auto engineRecords = compute.getEngineRecords();
            boardData->setEngineRecords(engineRecords);

            compute.getGameContext().withGameRecord([&](const GameRecord& g) {
                boardData->setGameIfDifferent(g);
                });

            workspace.draw();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
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
        return runApp();
    }
    catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Fatal Error", MB_ICONERROR | MB_OK);
        return 1;
    }
}
#else
int main() {
    try {
        return runApp();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}
#endif
