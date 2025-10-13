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
#include "setup-board-window.h"

#include "qapla-engine/types.h"
#include "font.h"
#include "imgui-button.h"

#include <imgui.h>
#include <string>

namespace QaplaWindows
{

    static bool drawSetupButton(
        const std::string& button, const std::string& label,
        const ImVec2& buttonSize,
        QaplaButton::ButtonState state)
    {
        return QaplaButton::drawIconButton(
            button, label, buttonSize, state,
            [state, &button](ImDrawList *drawList, ImVec2 topLeft, ImVec2 size)
            {
                if (button == "Setup")
                {
                    QaplaButton::drawSetup(drawList, topLeft, size, state);
                }
                else if (button == "New")
                {
                    QaplaButton::drawNew(drawList, topLeft, size, state);
                }
                else if (button == "Clear")
                {
                    QaplaButton::drawStop(drawList, topLeft, size, state);
                }
            });
    }

    std::string SetupBoardWindow::drawButtons()
    {
        constexpr float space = 3.0F;
        constexpr float topOffset = 5.0F;
        constexpr float bottomOffset = 8.0F;
        constexpr float leftOffset = 20.0F;
        ImVec2 boardPos = ImGui::GetCursorScreenPos();

        constexpr ImVec2 buttonSize = {25.0F, 25.0F};
        const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Setup");
        auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
        std::string clickedButton;

        for (const std::string button : {"Setup", "New", "Clear"})
        {
            ImGui::SetCursorScreenPos(pos);
            auto state = QaplaButton::ButtonState::Normal;
            if (drawSetupButton(button, button, buttonSize, state))
            {
               clickedButton = button;
            }
            pos.x += totalSize.x + space;
        }

        ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
        return clickedButton;
    }
}
