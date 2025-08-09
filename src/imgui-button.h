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

#include <imgui.h>

namespace QaplaButton {

    using IconDrawCallback = void(*)(ImDrawList*, ImVec2 topLeft, ImVec2 size);

    /**
     * @brief Draws a clickable icon-style button with optional icon and label below.
     *
     * @param id        Unique ImGui ID.
     * @param label     Text label shown below the button.
     * @param size      Size of the clickable icon area (excluding label).
     * @param iconDraw  Optional callback to draw the icon content (may be nullptr).
     * @return true if the button was clicked.
     */
    inline bool drawIconButton(const char* id, const char* label, ImVec2 size, IconDrawCallback iconDraw = nullptr) {
        ImVec2 topLeft = ImGui::GetCursorScreenPos();
        bool clicked = ImGui::InvisibleButton(id, size);
        bool hovered = ImGui::IsItemHovered();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImU32 bgColor = hovered
            ? IM_COL32(80, 80, 80, 255)
            : IM_COL32(120, 120, 120, 255);

        drawList->AddRectFilled(topLeft, ImVec2(topLeft.x + size.x, topLeft.y + size.y), bgColor, 6.0f);

        if (iconDraw) {
            ImVec2 center = ImVec2(topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f);
            iconDraw(drawList, center, size);
        }

        ImVec2 labelSize = ImGui::CalcTextSize(label);
        ImVec2 labelPos = ImVec2(topLeft.x + size.x * 0.5f - labelSize.x * 0.5f, topLeft.y + size.y + 4.0f);
        drawList->AddText(labelPos, IM_COL32(192, 192, 192, 255), label);

        return clicked;
    }

    /**
     * @brief Calculates the total area needed to draw the icon button including its label.
     *
     * @param size   Size of the clickable icon area (excluding label).
     * @param label  The label text shown below the button.
     * @return Total size including label.
     */
    inline ImVec2 calcIconButtonTotalSize(ImVec2 size, const char* label) {
        ImVec2 labelSize = ImGui::CalcTextSize(label);
        float totalHeight = size.y + 4.0f + labelSize.y;
        float totalWidth = std::max(size.x, labelSize.x);
        return ImVec2(totalWidth, totalHeight);
    }

} // namespace QaplaButton
