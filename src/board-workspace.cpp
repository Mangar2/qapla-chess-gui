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
#include "board-workspace.h"
#include <imgui.h>

namespace QaplaWindows {

    void BoardWorkspace::setRootWindow(std::unique_ptr<EmbeddedWindow> window) {
        rootWindow = std::move(window);
    }

    void BoardWorkspace::drawMaximized()
    {
        if (!maximized_) return;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        /*
        ImGui::SetNextWindowSizeConstraints(ImVec2(150, 150), ImVec2(1000, 1000));
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
        ImGui::Begin(id_.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        draw();
        ImGui::End();
        */
    }

    void BoardWorkspace::draw() {
        try {
            ImVec2 pos = ImGui::GetMainViewport()->Pos;
            ImVec2 size = ImGui::GetMainViewport()->Size;

            ImGui::SetNextWindowPos(pos);
            ImGui::SetNextWindowSize(size);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

            ImGui::Begin("RootOverlay", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNavFocus);

            if (rootWindow) {
                rootWindow->draw();
            }

            ImGui::End();
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(1);
        }
        catch (...) {
            ImGui::TextUnformatted("Error rendering Board Workspace");
        }
    }



} // namespace QaplaWindows
