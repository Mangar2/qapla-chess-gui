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

#include <imgui.h>

namespace QaplaWindows::ImGuiCutPaste {

std::optional<std::string> getClipboardString() {
    const char* text = ImGui::GetClipboardText();
    if (text == nullptr || text[0] == '\0') {
        return std::nullopt;
    }

    return std::string(text);
}

bool setClipboardString(const std::string& text) {
    ImGui::SetClipboardText(text.c_str());
    return true;
}

bool checkForCopy() {

    // Check for platform-specific copy shortcuts
    // Windows/Linux: Ctrl+C, macOS: Cmd+C (Super key)
    bool ctrlOrCmd = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || 
                     ImGui::IsKeyDown(ImGuiKey_RightCtrl) ||
                     ImGui::IsKeyDown(ImGuiKey_LeftSuper) || 
                     ImGui::IsKeyDown(ImGuiKey_RightSuper);

    return ctrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_C);
}

std::optional<std::string> checkForPaste() {
    // Check for platform-specific paste shortcuts
    // Windows/Linux: Ctrl+V, macOS: Cmd+V (Super key)
    bool ctrlOrCmd = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || 
                     ImGui::IsKeyDown(ImGuiKey_RightCtrl) ||
                     ImGui::IsKeyDown(ImGuiKey_LeftSuper) || 
                     ImGui::IsKeyDown(ImGuiKey_RightSuper);

    if (ctrlOrCmd && ImGui::IsKeyPressed(ImGuiKey_V)) {
        return getClipboardString();
    }

    return std::nullopt;
}

} // namespace QaplaWindows::ImGuiCutPaste