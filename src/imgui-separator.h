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

#include "imgui.h"

namespace ImGuiSeparator {

    /**
     * @brief Draws a horizontal separator using ImGui's layout and style.
     */
    inline void Horizontal() {
        ImGui::Separator(); // Uses ImGui's built-in horizontal separator
    }

    /**
     * @brief Draws a vertical separator using ImGui's layout and style.
     * 
     * This function mimics ImGui's layout behavior for horizontal separators.
     */
	inline void Vertical(float thickness = 1.0f) {
		ImVec2 topLeft = ImGui::GetCursorScreenPos();
		ImVec2 avail = ImGui::GetContentRegionAvail();
		ImGui::GetWindowDrawList()->AddRectFilled(
			topLeft, ImVec2(topLeft.x + thickness, topLeft.y + avail.y),
			ImGui::GetColorU32(ImGuiCol_Separator)
		);
	}

}