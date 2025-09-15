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

#include "imgui-cut-paste.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

namespace QaplaWindows::ImGuiCutPaste {

std::optional<std::string> getClipboardString(GLFWwindow* window) {
    if (!window) {
        return std::nullopt;
    }

    const char* text = glfwGetClipboardString(window);
    if (!text) {
        return std::nullopt;
    }

    return std::string(text);
}

std::optional<std::string> checkForPaste(GLFWwindow* window) {
    if (!window) {
        return std::nullopt;
    }

    // Check for platform-specific paste shortcuts
    // Windows/Linux: Ctrl+V, macOS: Cmd+V (Super key)
    bool ctrlOrCmd = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || 
                     ImGui::IsKeyDown(ImGuiKey_RightCtrl) ||
                     ImGui::IsKeyDown(ImGuiKey_LeftSuper) || 
                     ImGui::IsKeyDown(ImGuiKey_RightSuper);

    if (ctrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_V)) {
        return getClipboardString(window);
    }

    return std::nullopt;
}

} // namespace QaplaWindows::ImGuiCutPaste