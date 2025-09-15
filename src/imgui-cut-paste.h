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

#pragma once

#include <string>
#include <optional>

struct GLFWwindow;

namespace QaplaWindows {

/**
 * @brief Utility functions for clipboard operations in ImGui applications
 * 
 * This namespace provides cross-platform clipboard functionality using GLFW
 * for reading clipboard content and detecting paste keyboard shortcuts.
 */
namespace ImGuiCutPaste {

    /**
     * @brief Gets text content from the system clipboard
     * 
     * @param window GLFW window handle
     * @return std::optional<std::string> The clipboard text content if available,
     *                                   std::nullopt if clipboard is empty or contains non-text data
     */
    std::optional<std::string> getClipboardString(GLFWwindow* window);

    /**
     * @brief Checks for paste keyboard shortcut and returns clipboard content if triggered
     * 
     * This function should be called regularly in the GUI loop to detect paste shortcuts.
     * It handles platform-specific shortcuts:
     * - Windows/Linux: Ctrl+V
     * - macOS: Cmd+V
     * 
     * @param window GLFW window handle
     * @return std::optional<std::string> The clipboard text content if paste shortcut was pressed,
     *                                   std::nullopt if no paste shortcut was detected
     */
    std::optional<std::string> checkForPaste(GLFWwindow* window);

} // namespace ImGuiCutPaste

} // namespace QaplaWindows