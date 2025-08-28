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

#include <functional>
#include <imgui.h>
#include <string>

namespace QaplaButton {

    using IconDrawCallback = std::function<void(ImDrawList*, ImVec2 topLeft, ImVec2 size)>;

    void drawNew(ImDrawList* list, ImVec2 topLeft, ImVec2 size);

    void drawNow(ImDrawList* list, ImVec2 topLeft, ImVec2 size);

    void drawStop(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool active = false);

    void drawRestart(ImDrawList* list, ImVec2 topLeft, ImVec2 size);

    void drawConfig(ImDrawList* list, ImVec2 topLeft, ImVec2 size);

    void drawText(const std::string& text, ImDrawList* list, ImVec2 topLeft, ImVec2 size);

    void drawPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool active = false);

    void drawAnalyze(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool active = false);

    void drawAutoPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool active = false);

    void drawManualPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool active = false);

    void drawAdd(ImDrawList* list, ImVec2 topLeft, ImVec2 size);

    void drawRemove(ImDrawList* list, ImVec2 topLeft, ImVec2 size);

    void drawAutoDetect(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool rotate = false);

    /**
     * @brief Draws a clickable icon-style button with optional icon and label below.
     *
     * @param id        Unique ImGui ID.
     * @param label     Text label shown below the button.
     * @param size      Size of the clickable icon area (excluding label).
     * @param iconDraw  Optional callback to draw the icon content (may be nullptr).
     * @return true if the button was clicked.
     */
    bool drawIconButton(const std::string& id, const std::string& label, ImVec2 size, bool active,
        IconDrawCallback iconDraw = nullptr);

    /**
     * @brief Calculates the total area needed to draw the icon button including its label.
     *
     * @param size   Size of the clickable icon area (excluding label).
     * @param label  The label text shown below the button.
     * @return Total size including label.
     */
    ImVec2 calcIconButtonTotalSize(ImVec2 size, const char* label);

    /**
     * @brief Calculates the total size required to render multiple icon buttons, including their labels.
     *
     * This function computes the maximum dimensions needed to render a set of icon buttons by iterating
     * through their labels and determining the largest size required for any button. The size of each
     * button is calculated using `calcIconButtonTotalSize`, which includes both the icon area and the
     * label below it.
     *
     * @param buttonSize The size of the clickable icon area for each button (excluding labels).
     * @param labels     A vector of label texts, one for each button.
     * @return The total size (width and height) required to render all buttons, considering the largest
     *         dimensions among them.
     */
    inline ImVec2 calcIconButtonsTotalSize(ImVec2 buttonSize, const std::vector<std::string>& labels) {
        auto totalSize = buttonSize;
        for (const auto& label : labels) {
            auto total = QaplaButton::calcIconButtonTotalSize(buttonSize, label.c_str());
            totalSize.x = std::max(totalSize.x, total.x);
            totalSize.y = std::max(totalSize.y, total.y);
        }
        totalSize.x = std::round(totalSize.x);
        totalSize.y = std::round(totalSize.y);
		return totalSize;
    }
    
} // namespace QaplaButton
