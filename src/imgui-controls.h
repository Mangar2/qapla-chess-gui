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

#include "qapla-tester/time-control.h"
#include "qapla-tester/engine-option.h"

#include <imgui.h>
#include <string>
#include <optional>
#include <functional>
#include <algorithm>

namespace QaplaWindows::ImGuiControls {

    /**
     * @brief Displays an annotation text on the same line with optional red highlighting.
     * @param text The annotation text to display.
     * @param red If true, displays in red; if false, displays in disabled text color.
     */
    inline void annotate(const std::string& text, bool red = true) {
        if (text.empty()) {
            return;
        }
        ImGui::SameLine();
        if (red) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", text.c_str());
        } else {
            ImGui::TextDisabled("%s", text.c_str());
        }
    }

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

    inline bool inputText(const char* label, std::string& value,
        ImGuiInputTextFlags flags = 0,
        ImGuiInputTextCallback callback = nullptr,
        void* userData = nullptr) 
    {
        const auto& newText = value;
        auto result = inputText(label, newText, flags, callback, userData);
        if (result) {
            value = *result;
            return true;
        }
        return false;
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
     * @brief Input control for selecting and displaying values in promille (‰).
     * 
     * @param label Label to display next to the input box.
     * @param value Reference to the double value to modify.
     * @param min Minimum allowed value.
     * @param max Maximum allowed value.
     * @param step Step size for increment/decrement.
     * @return True if the value was modified, false otherwise.
     */
    inline bool inputPromille(const char* label, double& value, double min, double max, double step = 0.001) {
        assert(min < max && "Min must be less than max");
        assert(step > 0.0 && "Step must be positive");

        int32_t promilleValue = static_cast<int32_t>(value * 1000.0);
        int32_t promilleMin = static_cast<int32_t>(min * 1000.0);
        int32_t promilleMax = static_cast<int32_t>(max * 1000.0);
        int32_t promilleStep = static_cast<int32_t>(step * 1000.0);

        bool modified = ImGui::InputInt(label, &promilleValue, promilleStep, promilleStep * 10);

        promilleValue = std::clamp(promilleValue, promilleMin, promilleMax);

        if (modified) {
            value = static_cast<double>(promilleValue) / 1000.0;
            value = std::clamp(value, min, max);
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
    inline bool existingFileInput(const std::string& label, std::string& filePath, 
        float inputWidth = 200.0F, const char* buttonLabel = "Select") {
        bool modified = false;
		ImGui::PushID(label.c_str()); 
        // Display label
        ImGui::TextUnformatted(label.c_str());

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
        modified |= inputText("##filePath", filePath);
		ImGui::PopID(); 
        return modified;
    }

    /**
     * @brief Directory input control for selecting and displaying directory paths.
     * @param label Label to display next to the input box.
     * @param directoryPath Reference to the directory path string to modify.
     * @param inputWidth Width of the input box.
     * @param buttonLabel Label for the directory selection button.
     * @return True if the directory path was modified, false otherwise.
     */
    inline bool existingDialogInput(const std::string& label, std::string& directoryPath, 
        float inputWidth = 200.0F, const char* buttonLabel = "Browse") {
        bool modified = false;
        ImGui::PushID(label.c_str()); 
        
        // Display label
        ImGui::TextUnformatted(label.c_str());

        // Directory selection button
        if (ImGui::Button(buttonLabel)) {
            try {
                auto selectedPath = OsDialogs::selectFolderDialog(directoryPath);
                if (!selectedPath.empty()) {
                    directoryPath = selectedPath;
                    modified = true;
                }
            }
            catch (const std::exception& e) {
                SnackbarManager::instance().showError(e.what());
            }
        }

        // Input box for directory path
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::SameLine();
        modified |= inputText("##directoryPath", directoryPath);
        ImGui::PopID(); 
        return modified;
    }

    /**
     * @brief File input control for specifying a new or existing file path (e.g. for saving).
     * @param label Label to display next to the input box.
     * @param filePath Reference to the file path string to modify.
     * @param filters File extension filters to apply in the save dialog.
     * @param inputWidth Width of the input box.
     * @param buttonLabel Label for the file selection button.
     * @return True if the file path was modified, false otherwise.
     */
    inline bool newFileInput(const std::string& label, std::string& filePath,
        const std::vector<std::pair<std::string, std::string>>& filters = {},
        float inputWidth = 200.0F, const char* buttonLabel = "Select") {
        
        bool modified = false;
        ImGui::PushID(label.c_str());

        ImGui::TextUnformatted(label.c_str());

        if (ImGui::Button(buttonLabel)) {
            try {
                auto selectedPath = OsDialogs::saveFileDialog(filters, filePath);
                if (!selectedPath.empty()) {
                    filePath = selectedPath;
                    modified = true;
                }
            }
            catch (const std::exception& e) {
                SnackbarManager::instance().showError(e.what());
            }
        }

        ImGui::SetNextItemWidth(inputWidth);
        ImGui::SameLine();
        modified |= inputText("##filePath", filePath);

        ImGui::PopID();
        return modified;
    }


    /**
     * @brief Selection box for choosing a string from a list of options.
     * 
	 * If currentItem is out of bounds, it displays "Custom".
     * 
     * @param label Label to display next to the selection box.
     * @param currentItem Reference to the currently selected item (index in the options vector).
     * @param options Vector of strings representing the selectable options.
     * @param boxWidth Width of the selection box.
     * @return True if the selection was changed, false otherwise.
     */
    inline bool selectionBox(const char* label, int& currentItem, const std::vector<std::string>& options) {
        bool modified = false;
        bool isIndex = currentItem >= 0 && currentItem < static_cast<int>(options.size());
        if (ImGui::BeginCombo(label, isIndex ? options[currentItem].c_str() : "Custom"
        )) {
            for (size_t i = 0; i < options.size(); ++i) {
                bool isSelected = (currentItem == static_cast<int>(i));
                if (ImGui::Selectable(options[i].c_str(), isSelected)) {
                    currentItem = static_cast<int>(i);
					modified = true;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
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
    inline bool selectionBox(const char* label, std::string& currentItem, const std::vector<std::string>& options) {

        int currentIndex = static_cast<int>(std::ranges::find_if(options, 
            [&](const std::string& option) {
                const auto lowerOption = QaplaHelpers::to_lowercase(option);
                const auto lowerCurrent = QaplaHelpers::to_lowercase(currentItem);
                return lowerOption == lowerCurrent;
            }
        ) - options.begin());
        if (std::cmp_equal(currentIndex, options.size())) {
            currentIndex = 0; // default to first item if not found
        }
        auto modified = selectionBox(label, currentIndex, options);
        if (modified) {
            if (currentIndex >= 0 && currentIndex < static_cast<int>(options.size())) {
                currentItem = options[currentIndex];
            }
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
     * @brief Tri-state input for two boolean values using a selection box with three options.
     * 
     * Logic: the major controls if the feature is active at all, 0 = inactive, 2 = active, 
     * the minor controls if it is in test mode, 1 = test mode (only if major is true).
     * 
     * @param label Label to display next to the input field.
     * @param major Reference to the major boolean value to modify.
     * @param minor Reference to the minor boolean value to modify.
     * @param labels Optional labels for the three states (default: {"Inactive", "Test", "Active"}).
     */
    inline bool triStateInput(const char* label, bool& major, bool& minor, 
        std::vector<std::string> labels = {"Inactive", "Test", "Active"}) {

        // Map the boolean value to the selection index
        int currentItem = major ? 2 : 0;
        if (major && minor) {
            currentItem = 1;
        }
        
        // Use the selectionBox function to render the input
        if (selectionBox(label, currentItem, labels)) {
            // Update the boolean value based on the selected index
            major = (currentItem != 0);
            minor = (currentItem == 1);
            return true;
        }

        return false;
    }

    /**
     * @brief Checkbox input for a boolean value.
     * @param label Label to display next to the checkbox.
     * @param value Reference to the boolean value to modify.
     * @return True if the value was modified, false otherwise.
     */
    inline bool checkbox(std::string label, bool& value) {
        return ImGui::Checkbox(label.c_str(), &value);
    }


    /**
     * @brief Collapsing header with a checkbox to select/deselect.
     * @param label Label to display next to the checkbox and header.
     * @param selected Reference to the boolean value indicating selection state.
     * @param flags Optional ImGuiTreeNodeFlags for the collapsing header.
     * @param contentCallback Optional callback function to render content when expanded.
     * @return True if the selection state or content was modified, false otherwise.
     */
    inline bool collapsingSelection(std::string label, bool& selected, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None,
        const std::function<bool()>& contentCallback = nullptr){
        bool modified = false;
        modified = ImGui::Checkbox("##select", &selected);
        ImGui::SameLine(0.0F, 4.0F);
        if (ImGui::CollapsingHeader(label.c_str(), flags)) {
            ImGui::Indent(10.0F);
            // call callback to draw content
            if ((flags & ImGuiTreeNodeFlags_Leaf) == 0 && contentCallback) {
                modified |= contentCallback();
            }
            ImGui::Unindent(10.0F);
        }
        return modified;
    }

    /**
    * @brief Creates an input control for an EngineOption.
    * @param option The EngineOption to create the input for.
    * @param value Reference to the current value of the option as a string.
    * @param inputWidth Optional width for a file Input field.
    * @return True if the value was modified, false otherwise.
    */
    inline bool engineOptionControl(const QaplaTester::EngineOption& option, 
        std::string& value, float fileInputWidth = 400.0F) {
        bool modified = false;

        switch (option.type) {
        case QaplaTester::EngineOption::Type::File: {
            // File input
            modified = existingFileInput(option.name.c_str(), value, fileInputWidth);
            break;
        }
        case QaplaTester::EngineOption::Type::Check: {
            // Boolean input (converted to "true"/"false")
            bool boolValue = (value == "true");
            if (booleanInput(option.name.c_str(), boolValue)) {
                value = boolValue ? "true" : "false";
                modified = true;
            }
            break;
        }
        case QaplaTester::EngineOption::Type::Spin: {
            // Integer input with min/max
            int intValue = value.empty() ? option.min.value_or(0) : std::stoi(value);
            if (inputInt(option.name.c_str(), intValue, option.min.value_or(0), option.max.value_or(100))) {
                value = std::to_string(intValue);
                modified = true;
            }
            break;
        }
        case QaplaTester::EngineOption::Type::Slider: {
            // Slider input with min/max
            int intValue = value.empty() ? option.min.value_or(0) : std::stoi(value);
            if (sliderInt(option.name.c_str(), intValue, option.min.value_or(0), option.max.value_or(100))) {
                value = std::to_string(intValue);
                modified = true;
            }
            break;
        }
        case QaplaTester::EngineOption::Type::Combo: {
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
        case QaplaTester::EngineOption::Type::String: {
            // Check if the option name contains "Path" (case insensitive)
            std::string optionNameLower = option.name;
            std::transform(optionNameLower.begin(), optionNameLower.end(), optionNameLower.begin(), ::tolower);
            
            if (optionNameLower.find("path") != std::string::npos) {
                // Use directory selection dialog for path options
                modified |= existingDialogInput(option.name, value);
            } else {
                // Use regular text input for other string options
                modified |= inputText(option.name.c_str(), value);
            }
            break;
        }
        default:
            ImGui::Text("Unsupported option type: %s", QaplaTester::EngineOption::to_string(option.type).c_str());
            break;
        }

        return modified;
    }

    /**
     * @brief Input control for optional values.
     * 
     * Renders a checkbox to indicate if the value is present, and if so, renders the input control using the provided callback.
     * 
     * @tparam T Type of the optional value.
     * @param label Label to display next to the checkbox and input box.
     * @param value Reference to the std::optional value to modify.
     * @param inputCallback Callback function that takes a reference to T and returns true if modified.
     * @return True if the value was modified, false otherwise.
     */
    template <typename T>
    inline bool optionalInput(const char* label, std::optional<T>& value, const std::function<bool(T&)>& inputCallback) {
        bool modified = false;

        bool hasValue = value.has_value();
        if (ImGui::Checkbox(label, &hasValue)) {
            if (hasValue) {
                value = T{}; 
            }
            else {
                value.reset(); 
            }
            modified = true;
        }

        if (hasValue && value.has_value()) {
            ImGui::SameLine();
            T tempValue = *value;
            if (inputCallback(tempValue)) {
                value = tempValue;
                modified = true;
            }
        }

        return modified;
    }

    /**
     * @brief Input control for time control settings.
     * @param timeControl format: moves/time+inc.
     * @return True if the time control was modified, false otherwise.
	 */
    inline bool timeControlInput(std::string& timeControl, bool blitz = false, float inputWidth = 0.0) {

        // Extract time from timeControl string
		uint64_t baseTimeMs = 0;
		uint64_t incrementMs = 0;
		uint32_t movesToPlay = 0;
        try {
            // Parse the time control string
			QaplaTester::TimeSegment ts = QaplaTester::TimeSegment::fromString(timeControl);
            baseTimeMs = ts.baseTimeMs;
            incrementMs = ts.incrementMs;
            movesToPlay = ts.movesToPlay;
        }
        catch (const std::exception& e) {
            SnackbarManager::instance().showError(e.what());
            return false; // Invalid time control format
		}

        // Convert base time to hours, minutes, and seconds
        uint32_t hours = static_cast<int>(baseTimeMs / 3600000);
        uint32_t minutes = static_cast<int>((baseTimeMs % 3600000) / 60000);
        uint32_t seconds = static_cast<int>((baseTimeMs % 60000) / 1000);

        // Convert increment to minutes, seconds, and milliseconds
        uint32_t incrementMinutes = static_cast<int>(incrementMs / 60000);
        uint32_t incrementSeconds = static_cast<int>((incrementMs % 60000) / 1000);
        uint32_t incrementMilliseconds = static_cast<int>(incrementMs % 1000);

        // Input fields for base time
        if (inputWidth > 0) ImGui::SetNextItemWidth(inputWidth);
        if (!blitz) {
            inputInt<uint32_t>("Hours", hours, 0, 10000);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Hours of base time for the moves to play");
            }
        }
        if (inputWidth > 0) ImGui::SetNextItemWidth(inputWidth);
        inputInt<uint32_t>("Minutes", minutes, 0, 59);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Minutes of base time for the moves to play");
        }
        if (inputWidth > 0) ImGui::SetNextItemWidth(inputWidth);
		inputInt<uint32_t>("Seconds", seconds, 0, 59);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Seconds of base time for the moves to play");
        }

        // Input fields for increment
        if (inputWidth > 0) ImGui::SetNextItemWidth(inputWidth);
		if (!blitz) {
            inputInt<uint32_t>("Increment Minutes", incrementMinutes, 0, 59);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Minutes added per move");
            }
        }
        if (inputWidth > 0) ImGui::SetNextItemWidth(inputWidth);
		inputInt<uint32_t>("Increment Seconds", incrementSeconds, 0, 59);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Seconds added per move");
        }
        if (inputWidth > 0) ImGui::SetNextItemWidth(inputWidth);
		inputInt<uint32_t>("Increment Milliseconds", incrementMilliseconds, 0, 999, 10, 100);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Milliseconds added per move");
        }

        if (inputWidth > 0) ImGui::SetNextItemWidth(inputWidth);
		if (!blitz) {
            inputInt<uint32_t>("Moves to Play", movesToPlay, 0, 1000);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    "Moves to play before time resets.\n"
                    "0 = no reset (entire game)\n"
                    "e.g., 40 = add base time after 40 moves"
                );
            }
        }
        QaplaTester::TimeSegment res;
		res.movesToPlay = movesToPlay;
        res.baseTimeMs = static_cast<uint64_t>(hours) * 3600000 +
            static_cast<uint64_t>(minutes) * 60000 +
			static_cast<uint64_t>(seconds) * 1000;
        res.incrementMs = static_cast<uint64_t>(incrementMinutes) * 60000 +
            static_cast<uint64_t>(incrementSeconds) * 1000 +
			static_cast<uint64_t>(incrementMilliseconds);

        std::string resStr = to_string(res);
		bool result = resStr != timeControl;
		timeControl = resStr;
		return result;
	}

    inline void drawBoxWithShadow(ImVec2 topLeft, ImVec2 bottomRight)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImGuiStyle& style = ImGui::GetStyle();

        ImU32 borderCol = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 shadowCol = ImGui::GetColorU32(ImGuiCol_BorderShadow);

        const float shadowSpread = 2.0F; 
        ImVec2 shadowMin = ImVec2(topLeft.x + shadowSpread, topLeft.y + shadowSpread);
        ImVec2 shadowMax = ImVec2(bottomRight.x + shadowSpread, bottomRight.y + shadowSpread);
        drawList->AddRect(shadowMin, shadowMax, shadowCol, style.FrameRounding + shadowSpread);

        drawList->AddRect(topLeft, bottomRight, borderCol, style.FrameRounding, 0, 2.0F);
    }

    inline void drawDot(float offsetX = 3.0F, float offsetY = 3.0F) {
        if (ImGui::IsItemVisible()) {
            ImVec2 tabMin = ImGui::GetItemRectMin();
            ImVec2 tabMax = ImGui::GetItemRectMax();

            constexpr float dotRadius = 6.0F;
            ImVec2 dotPos = ImVec2(tabMax.x - offsetX, tabMin.y + offsetY);

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddCircleFilled(dotPos, dotRadius, IM_COL32(192, 0, 0, 192));
        }   
    }

    /**
     * @brief Custom CollapsingHeader that can display a red dot in the upper right corner.
     * @param label Label of the collapsing header.
     * @param flags Optional ImGuiTreeNodeFlags.
     * @param showDot If true, a red dot is displayed in the upper right corner.
     * @return True if the header is open, false otherwise.
     */
    inline bool CollapsingHeaderWithDot(const char* label, ImGuiTreeNodeFlags flags = 0, bool showDot = false) {
        bool result = ImGui::CollapsingHeader(label, flags);
        
        if (showDot) {
            constexpr float dotOffsetX = 20.0F;  // More offset for CollapsingHeader arrow
            constexpr float dotOffsetY = 10.0F;
            drawDot(dotOffsetX, dotOffsetY);
        }
        
        return result;
    }

    /**
     * @brief Custom TabItem that can display a red dot in the upper right corner.
     * @param label Label of the tab item.
     * @param p_open Optional pointer to a boolean that indicates if the tab is open.
     * @param flags Optional ImGuiTabItemFlags.
     * @param showDot If true, a red dot is displayed in the upper right corner of the tab.
     * @return True if the tab is selected, false otherwise.
     */
    inline bool TabItemWithDot(const char* label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0,  bool showDot = false) {
        bool result = ImGui::BeginTabItem(label, p_open, flags);
        
        if (showDot) {
            drawDot();
        }
        
        return result;
    }

}
