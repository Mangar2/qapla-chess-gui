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

#include "chess-board-window.h"
#include "move-list-window.h"
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

    int runApp() {
        auto gameState = std::make_shared<GameState>();
		auto gameRecord = std::make_shared<GameRecord>();
        gameState->setFen(false, "rnbqkb2/pppppp1P/8/8/8/8/PPPPPPP1/RNBQKBNR w KQkq - 0 1");
                
        QaplaWindows::BoardWorkspace workspace;
        auto vSplitContainer = std::make_unique<QaplaWindows::VerticalSplitContainer>();
        auto hSplitContainer = std::make_unique<QaplaWindows::HorizontalSplitContainer>();
        hSplitContainer->setLeft(std::make_unique<QaplaWindows::ChessBoardWindow>(gameState, gameRecord));
		hSplitContainer->setRight(std::make_unique<QaplaWindows::MoveListWindow>(gameRecord));
        vSplitContainer->setTop(std::move(hSplitContainer));
        vSplitContainer->setBottom(std::make_unique<QaplaWindows::MoveListWindow>(gameRecord));
		workspace.setRootWindow(std::move(vSplitContainer));

        auto* window = initGlfwContext();
        initGlad();
        initImGui(window);
        font::loadChessFont("fonts/chess_merida_unicode.ttf", 32.0f);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            int width{}, height{};
            glfwGetFramebufferSize(window, &width, &height);
            workspace.draw();
            ImGui::Render();
            glViewport(0, 0, width, height);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
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
