/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "imgui-text-button.h"
#include "imgui-controls.h"
#include "i18n.h"
#include <imgui.h>
#include <iostream>

namespace QaplaWindows {

    [[nodiscard]] ImVec2 ImGuiTextButton::calcSize() const {
        const ImVec2 textSize = ImGui::CalcTextSize(translated.c_str(), NULL, true);
        const ImGuiStyle& style = ImGui::GetStyle();    
        constexpr float BORDER = 5.0F;
        return ImVec2(
            textSize.x + 2.0f * style.FramePadding.x + BORDER,
            textSize.y + 2.0f * style.FramePadding.y + BORDER
        );
    }


} // namespace QaplaWindows
