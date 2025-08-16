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

#include "os-dialogs.h"
#include "snackbar.h"

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

    /**
     * @brief File input control for selecting and displaying file paths.
     * @param label Label to display next to the input box.
     * @param filePath Reference to the file path string to modify.
     * @param inputWidth Width of the input box.
     * @param buttonLabel Label for the file selection button.
     * @return True if the file path was modified, false otherwise.
     */
    inline bool fileInput(const char* label, std::string& filePath, float inputWidth = 200.0f, const char* buttonLabel = "Select") {
        bool modified = false;

        // Display label
        ImGui::TextUnformatted(label);

        // File selection button
        if (ImGui::Button(buttonLabel)) {
            try {
                auto selectedFiles = OsDialogs::openFileDialog();
                if (!selectedFiles.empty()) {
                    filePath = selectedFiles[0]; // Use the first selected file
                    modified = true;
                }
            }
            catch (const std::exception& e) {
                SnackbarManager::instance().showError(e.what());
            }
        }

        // Input box for file path
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::SameLine();
        if (auto path = inputText("##filePath", filePath)) {
            filePath = *path;
            modified = true;
        }

        return modified;
    }

    /**
     * @brief Selection box for choosing a string from a list of options.
     * @param label Label to display next to the selection box.
     * @param currentItem Reference to the currently selected item (index in the options vector).
     * @param options Vector of strings representing the selectable options.
     * @param boxWidth Width of the selection box.
     * @return True if the selection was changed, false otherwise.
     */
    inline bool selectionBox(const char* label, int& currentItem, const std::vector<std::string>& options) {
        bool modified = false;

        // Ensure the current item index is valid
        if (currentItem < 0 || currentItem >= static_cast<int>(options.size())) {
            currentItem = 0; // Default to the first item if out of range
        }

        // Convert options to a format suitable for ImGui
        std::vector<const char*> cStrings;
        cStrings.reserve(options.size());
        for (const auto& option : options) {
            cStrings.push_back(option.c_str());
        }

        // Render the combo box
        if (ImGui::Combo(label, &currentItem, cStrings.data(), static_cast<int>(cStrings.size()))) {
            modified = true;
        }

        return modified;
    }

    /**
     * @brief Input field for a boolean value using a selection box with "Yes" and "No" options.
     * @param label Label to display next to the input field.
     * @param value Reference to the boolean value to modify.
     * @param boxWidth Width of the selection box.
     * @return True if the value was modified, false otherwise.
     */
    inline bool booleanInput(const char* label, bool& value) {
        // Define the options for the selection box
        static const std::vector<std::string> options = { "No", "Yes" };

        // Map the boolean value to the selection index
        int currentItem = value ? 1 : 0;

        // Use the selectionBox function to render the input
        if (selectionBox(label, currentItem, options)) {
            // Update the boolean value based on the selected index
            value = (currentItem == 1);
            return true;
        }

        return false;
    }

    /**
    * @brief Creates an input control for an EngineOption.
    * @param option The EngineOption to create the input for.
    * @param value Reference to the current value of the option as a string.
    * @param inputWidth Optional width for a file Input field.
    * @return True if the value was modified, false otherwise.
    */
    inline bool engineOptionControl(const EngineOption& option, std::string& value, float fileInputWidth = 400.0f) {
        bool modified = false;

        switch (option.type) {
        case EngineOption::Type::File: {
            // File input
            modified = fileInput(option.name.c_str(), value, fileInputWidth);
            break;
        }
        case EngineOption::Type::Check: {
            // Boolean input (converted to "true"/"false")
            bool boolValue = (value == "true");
            if (booleanInput(option.name.c_str(), boolValue)) {
                value = boolValue ? "true" : "false";
                modified = true;
            }
            break;
        }
        case EngineOption::Type::Spin: {
            // Integer input with min/max
            int intValue = value.empty() ? option.min.value_or(0) : std::stoi(value);
            if (inputInt(option.name.c_str(), intValue, option.min.value_or(0), option.max.value_or(100))) {
                value = std::to_string(intValue);
                modified = true;
            }
            break;
        }
        case EngineOption::Type::Slider: {
            // Slider input with min/max
            int intValue = value.empty() ? option.min.value_or(0) : std::stoi(value);
            if (sliderInt(option.name.c_str(), intValue, option.min.value_or(0), option.max.value_or(100))) {
                value = std::to_string(intValue);
                modified = true;
            }
            break;
        }
        case EngineOption::Type::Combo: {
            // Combo box for selecting from predefined options
            int currentIndex = 0;
            auto it = std::find(option.vars.begin(), option.vars.end(), value);
            if (it != option.vars.end()) {
                currentIndex = static_cast<int>(std::distance(option.vars.begin(), it));
            }
            if (selectionBox(option.name.c_str(), currentIndex, option.vars)) {
                value = option.vars[currentIndex];
                modified = true;
            }
            break;
        }
        case EngineOption::Type::String: {
            // Text input
            if (auto newValue = inputText(option.name.c_str(), value)) {
                value = *newValue;
                modified = true;
            }
            break;
        }
        default:
            // Unsupported type
            ImGui::Text("Unsupported option type: %s", EngineOption::to_string(option.type).c_str());
            break;
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
