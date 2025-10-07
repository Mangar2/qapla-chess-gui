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

#include "engine-window.h"
#include "imgui-separator.h"
#include "imgui-engine-list.h"
#include "imgui-button.h"

#include <imgui.h>

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;

EngineWindow::EngineWindow()
{
}

EngineWindow::~EngineWindow() = default;

std::string EngineWindow::drawConfigButtonArea(bool noEngines) {
    constexpr float borderX = 20.0F;
    constexpr float borderY = 8.0F;
    constexpr float spacingY = 30.0F;

    constexpr ImVec2 buttonSize = { 25.0F, 25.0F };
    auto topLeft = ImGui::GetCursorScreenPos();

    std::string command;

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x + borderX, topLeft.y + borderY));
    auto state = QaplaButton::ButtonState::Normal;
    if (noEngines)
        state = QaplaButton::ButtonState::Highlighted;
    if (QaplaButton::drawIconButton("Config", "Config", buttonSize, state,
        [state](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
            QaplaButton::drawConfig(drawList, topLeft, size, state);
        }))
    {
        command = "Config";
    }

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x + borderX, topLeft.y + borderY + buttonSize.y + spacingY));
    if (QaplaButton::drawIconButton("SwapButton", "Swap", buttonSize, 
        noEngines ? QaplaButton::ButtonState::Disabled : QaplaButton::ButtonState::Normal,
        [state](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
            QaplaButton::drawSwapEngines(drawList, topLeft, size, state);
        }))
    {
        command = "Swap";
    }

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x + areaWidth, topLeft.y));
    ImGuiSeparator::Vertical();
    return command;
}

std::pair<std::string, std::string> EngineWindow::draw() {
    constexpr float cMinRowHeight = 80.0F;
    constexpr float cEngineInfoWidth = 160.0F;
    constexpr float cMinTableWidth = 200.0F;
    constexpr float cSectionSpacing = 4.0F;

    const auto engineRecords = getEngineRecords();

    auto command = drawConfigButtonArea(engineRecords.empty());
    ImGui::Indent(areaWidth);
    auto [id2, command2] = ImGuiEngineList::draw();
    ImGui::Unindent(areaWidth);
    if (!command.empty()) return { "", command };
    return { id2, command2 };
}

