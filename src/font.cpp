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

#include "font.h"
#include "chess-font.h"
#include "inter-variable.h"
#include <imgui.h>
#include <stdexcept>
#include <filesystem>

namespace font {

    void loadFonts() {
        auto& io = ImGui::GetIO();

        io.Fonts->AddFontDefault(); // Default UI font
        //chessFont = io.Fonts->AddFontFromFileTTF(path.c_str(), size);
        ImFontConfig fontCfg;
        fontCfg.FontDataOwnedByAtlas = false;

        chessFont = io.Fonts->AddFontFromMemoryTTF(
            reinterpret_cast<void*>(const_cast<uint32_t*>(chessFontData)),
            static_cast<int>(chessFontSize),
            32,
            &fontCfg
        );

        interVariable = io.Fonts->AddFontFromMemoryTTF(
            reinterpret_cast<void*>(const_cast<uint32_t*>(interVariableData)),
            static_cast<int>(interVariableSize),
            16,
            &fontCfg
        );

        io.FontDefault = io.Fonts->Fonts[2]; 
    }

}

