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
#include <vector>

namespace QaplaButton {

    enum class ButtonState {
        Normal,
        Active,
        Highlighted,
        Disabled,
        Animated
    };

    /**
     * @brief Represents a command in a popup menu with its display state.
     */
    struct PopupCommand {
        std::string name;
        ButtonState state = ButtonState::Normal;
    };

    using IconDrawCallback = std::function<void(ImDrawList*, ImVec2 topLeft, ImVec2 size)>;

    void drawNew(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawNow(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawStop(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawClear(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawCancel(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawGrace(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawRestart(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawConfig(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawText(const std::string& text, ImDrawList* list, ImVec2 topLeft, ImVec2 size);

    void drawPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawAnalyze(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawAutoPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawManualPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawAdd(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawRemove(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawAutoDetect(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawSwapEngines(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawSetup(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawSave(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawOpen(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawTest(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawFilter(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawCopy(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawPaste(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawMore(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawTimeClock(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    void drawLog(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state = ButtonState::Normal);

    /**
     * @brief Shows a popup menu with selectable commands.
     *
     * @param popupId   Unique ID for the popup.
     * @param commands  List of PopupCommand structs containing command name and state.
     * @return The selected command string, or empty string if nothing was selected.
     */
    std::string showCommandPopup(const std::string& popupId, 
                                 const std::vector<PopupCommand>& commands);

    /**
     * @brief Draws a clickable icon-style button with optional icon and label below.
     *
     * @param id        Unique ImGui ID.
     * @param label     Text label shown below the button.
     * @param size      Size of the clickable icon area (excluding label).
     * @param iconDraw  Optional callback to draw the icon content (may be nullptr).
     * @param highlighted Whether the button is highlighted.
     * @param disabled    Whether the button is disabled.
     * @return true if the button was clicked.
     */
    bool drawIconButton(const std::string& id, const std::string& label, ImVec2 size, ButtonState state,
        const IconDrawCallback& iconDraw = nullptr);

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
    ImVec2 calcIconButtonsTotalSize(ImVec2 buttonSize, const std::vector<std::string>& labels);
    
} // namespace QaplaButton
