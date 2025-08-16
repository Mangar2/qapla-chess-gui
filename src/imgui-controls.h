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
        buffer.resize(1024);  // Fixed buffer size

        if (ImGui::InputText(label, buffer.data(), buffer.size(), flags, callback, userData)) {
            size_t nullPos = buffer.find('\0');
            return buffer.substr(0, nullPos);
        }

        return std::nullopt;
    }

    /**
     * @brief Template for InputInt with min/max validation for various integer types.
     * @tparam T Integer type (e.g., int, short, long).
     * @param label Label to display next to the input box.
     * @param value Reference to the integer value to modify.
     * @param min Minimum allowed value.
     * @param max Maximum allowed value.
     * @param flags Optional ImGuiInputTextFlags.
     * @return True if the value was modified, false otherwise.
     */
    template <typename T>
    inline bool inputInt(const char* label, T& value, T min, T max, int step = 1, int stepFast = 10, 
        ImGuiInputTextFlags flags = 0) 
    {
        static_assert(std::is_integral_v<T>, "inputInt only supports integral types");
		assert(min < max && "Min must be less than max");

        int tempValue = static_cast<int>(value);
        bool modified = ImGui::InputInt(label, &tempValue, step, stepFast, flags);

        if (tempValue < static_cast<int>(min)) tempValue = static_cast<int>(min);
        if (tempValue > static_cast<int>(max)) tempValue = static_cast<int>(max);

        if (modified) {
            value = static_cast<T>(tempValue);
        }

        return modified;
    }

    /**
     * @brief Template for SliderInt with min/max validation for various integer types.
     * @tparam T Integer type (e.g., int, short, long).
     * @param label Label to display next to the slider.
     * @param value Reference to the integer value to modify.
     * @param min Minimum allowed value.
     * @param max Maximum allowed value.
     * @param format Optional format string (e.g., "%d").
     * @return True if the value was modified, false otherwise.
     */
    template <typename T>
    inline bool sliderInt(const char* label, T& value, T min, T max, const char* format = "%d") {
        static_assert(std::is_integral_v<T>, "sliderInt only supports integral types");
        assert(min < max && "Min must be less than max");

        // Convert value to int for ImGui::SliderInt
        int tempValue = static_cast<int>(value);
        bool modified = ImGui::SliderInt(label, &tempValue, static_cast<int>(min), static_cast<int>(max), format);

        // Update the original value if it was modified
        if (modified) {
            value = static_cast<T>(tempValue);
        }

        return modified;
    }

    inline void drawBoxWithShadow(ImVec2 topLeft, ImVec2 bottomRight)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImGuiStyle& style = ImGui::GetStyle();

        ImU32 borderCol = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 shadowCol = ImGui::GetColorU32(ImGuiCol_BorderShadow);

        const float shadowSpread = 2.0f; 
        ImVec2 shadowMin = ImVec2(topLeft.x + shadowSpread, topLeft.y + shadowSpread);
        ImVec2 shadowMax = ImVec2(bottomRight.x + shadowSpread, bottomRight.y + shadowSpread);
        drawList->AddRect(shadowMin, shadowMax, shadowCol, style.FrameRounding + shadowSpread);

        drawList->AddRect(topLeft, bottomRight, borderCol, style.FrameRounding, 0, 2.0f);
    }

}
