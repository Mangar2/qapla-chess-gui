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
#include <string>
#include <optional>

namespace QaplaWindows::ImGuiControls {

    /**
     * @brief Wrapper around ImGui::InputText for std::string input.
     * @param label Label to display next to the input box.
     * @param value Initial value as const string.
     * @param flags Optional ImGuiInputTextFlags.
     * @param callback Optional ImGuiInputTextCallback.
     * @param userData Optional user data pointer for callback.
     * @return optional std::string value after user input.
     */
    inline std::optional<std::string> inputText(const char* label,
        const std::string& value,
        ImGuiInputTextFlags flags = 0,
        ImGuiInputTextCallback callback = nullptr,
        void* userData = nullptr) {
        std::string buffer = value;
        buffer.resize(512);  // Fixed buffer size

        if (ImGui::InputText(label, buffer.data(), buffer.size(), flags, callback, userData)) {
            size_t nullPos = buffer.find('\0');
            return buffer.substr(0, nullPos);
        }

        return std::nullopt;
    }

}
