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

#pragma once

#include <imgui.h>
#include <string>
#include "imgui-controls.h"

namespace QaplaWindows {

struct ImGuiTextButton {
    std::string label;
    std::string translated = ImGuiControls::createLabel("TextButton", label);
    [[nodiscard]] ImVec2 calcSize() const;
    [[nodiscard]] bool draw(ImVec2 size = ImVec2(0, 0)) const {
        return ImGui::Button(translated.c_str(), size);
    }
};


} // namespace QaplaWindows
