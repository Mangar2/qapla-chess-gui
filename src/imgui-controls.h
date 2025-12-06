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
#include "i18n.h"

#include "time-control.h"
#include "engine-option.h"

#include <imgui.h>
#include <string>
#include <optional>
#include <functional>
#include <algorithm>

namespace QaplaWindows::ImGuiControls {

    /**
     * @brief Creates a translated label with a unique identifier.
     * 
     * If the label does not contain "###", it appends it to ensure uniqueness.
     * 
     * @param topic The translation topic (e.g., "Button", "Input").
     * @param label The label text to translate.
     * @return The translated label with unique identifier.
     */
    inline std::string createLabel(const std::string& topic, const std::string& label) {
        std::string modLabel = label;
        if (std::string_view(modLabel).find("###") == std::string_view::npos) {
            modLabel += std::format("###{}", modLabel);
        }
        return Translator::instance().translate(topic, modLabel);
    }

    /**
     * @brief Displays a tooltip when hovering over the last item.
     * 
     * It translates the tooltip text before displaying.
     * 
     * @param text The tooltip text to display.
     */
    void hooverTooltip(const std::string& text);

    /**
     * @brief Wrapper around ImGui::Checkbox with automatic label translation.
     * @param label The checkbox label to translate and display.
     * @param value Reference to the boolean value to modify.
     * @return True if the value was changed, false otherwise.
     */
    bool checkbox(const char* label, bool& value);

    /**
     * @brief Wrapper around ImGui::Button with automatic label translation.
     * @param label The button label to translate and display.
     * @param size Optional size for the button.
     * @return True if the button was clicked, false otherwise.
     */
    bool textButton(const char* label, ImVec2 size = ImVec2(0, 0));

    /**
     * @brief Wrapper around ImGui::TextWrapped with automatic text translation.
     * @param text The text to translate and display.
     */
    void textWrapped(const std::string& text);

    /**
     * @brief Wrapper around ImGui::TextDisabled with automatic text translation.
     * @param text The text to translate and display.
     */
    void textDisabled(const std::string& text);

    /**
     * @brief Displays an annotation text on the same line with optional red highlighting.
     * @param text The annotation text to display.
     * @param red If true, displays in red; if false, displays in disabled text color.
     */
    void annotate(const std::string& text, bool red = true);

    /**
     * @brief Wrapper around ImGui::InputText for std::string input.
     * @param label Label to display next to the input box.
     * @param value Initial value as const string.
     * @param flags Optional ImGuiInputTextFlags.
     * @param callback Optional ImGuiInputTextCallback.
     * @param userData Optional user data pointer for callback.
     * @return optional std::string value after user input.
     */
    std::optional<std::string> inputText(const char* label,
        const std::string& value,
        ImGuiInputTextFlags flags = 0,
        ImGuiInputTextCallback callback = nullptr,
        void* userData = nullptr);

    bool inputText(const char* label, std::string& value,
        ImGuiInputTextFlags flags = 0,
        ImGuiInputTextCallback callback = nullptr,
        void* userData = nullptr);

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
    bool inputInt(const char* label, T& value, T min, T max, int step = 1, int stepFast = 10, 
        ImGuiInputTextFlags flags = 0) 
    {
        static_assert(std::is_integral_v<T>, "inputInt only supports integral types");
		assert(min < max && "Min must be less than max");

        int tempValue = static_cast<int>(value);
        
        auto modLabel = createLabel("Input", label);
        bool modified = ImGui::InputInt(modLabel.c_str(), &tempValue, step, stepFast, flags);

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
    bool sliderInt(const char* label, T& value, T min, T max, const char* format = "%d") {
        static_assert(std::is_integral_v<T>, "sliderInt only supports integral types");
        assert(min < max && "Min must be less than max");

        // Convert value to int for ImGui::SliderInt
        int tempValue = static_cast<int>(value);
        auto modLabel = createLabel("Input", label);
        bool modified = ImGui::SliderInt(modLabel.c_str(), &tempValue, static_cast<int>(min), static_cast<int>(max), format);

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
    bool inputPromille(const char* label, double& value, double min, double max, double step = 0.001);

    /**
     * @brief File input control for selecting and displaying file paths.
     * @param label Label to display next to the input box.
     * @param filePath Reference to the file path string to modify.
     * @param inputWidth Width of the input box.
     * @param buttonLabel Label for the file selection button.
     * @return True if the file path was modified, false otherwise.
     */
    bool existingFileInput(const std::string& label, std::string& filePath, 
        float inputWidth = 200.0F, const char* buttonLabel = "Select");

    /**
     * @brief Directory input control for selecting and displaying directory paths.
     * @param label Label to display next to the input box.
     * @param directoryPath Reference to the directory path string to modify.
     * @param inputWidth Width of the input box.
     * @param buttonLabel Label for the directory selection button.
     * @return True if the directory path was modified, false otherwise.
     */
    bool existingDirectoryInput(const std::string& label, std::string& directoryPath, 
        float inputWidth = 200.0F, const char* buttonLabel = "Browse");

    /**
     * @brief File input control for specifying a new or existing file path (e.g. for saving).
     * @param label Label to display next to the input box.
     * @param filePath Reference to the file path string to modify.
     * @param filters File extension filters to apply in the save dialog.
     * @param inputWidth Width of the input box.
     * @param buttonLabel Label for the file selection button.
     * @return True if the file path was modified, false otherwise.
     */
    bool newFileInput(const std::string& label, std::string& filePath,
        const std::vector<std::pair<std::string, std::string>>& filters = {},
        float inputWidth = 200.0F, const char* buttonLabel = "Select");


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
    bool selectionBox(const char* label, int& currentItem, const std::vector<std::string>& options);

    /**
     * @brief Selection box for choosing a string from a list of options.
     * @param label Label to display next to the selection box.
     * @param currentItem Reference to the currently selected item (index in the options vector).
     * @param options Vector of strings representing the selectable options.
     * @param boxWidth Width of the selection box.
     * @return True if the selection was changed, false otherwise.
     */
    bool selectionBox(const char* label, std::string& currentItem, const std::vector<std::string>& options);

    /**
     * @brief Input field for a boolean value using a selection box with "Yes" and "No" options.
     * @param label Label to display next to the input field.
     * @param value Reference to the boolean value to modify.
     * @param boxWidth Width of the selection box.
     * @return True if the value was modified, false otherwise.
     */
    bool booleanInput(const char* label, bool& value);

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
    bool triStateInput(const char* label, bool& major, bool& minor, 
        std::vector<std::string> labels = {
            Translator::instance().translate("Option", "Inactive"),
            Translator::instance().translate("Option", "Test"),
            Translator::instance().translate("Option", "Active")
        });

    /**
     * @brief Checkbox input for a boolean value.
     * @param label Label to display next to the checkbox.
     * @param value Reference to the boolean value to modify.
     * @return True if the value was modified, false otherwise.
     */
    bool checkbox(std::string label, bool& value);


    /**
     * @brief Collapsing header with a checkbox to select/deselect.
     * 
     * @param label Label to display next to the checkbox and header.
     * @param selected Reference to the boolean value indicating selection state.
     * @param flags Optional ImGuiTreeNodeFlags for the collapsing header.
     * @param contentCallback Optional callback function to render content when expanded.
     * @return True if the selection state or content was modified, false otherwise.
     */
    bool collapsingSelection(std::string label, bool& selected, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None,
        const std::function<bool()>& contentCallback = nullptr);

    /**
    * @brief Creates an input control for an EngineOption.
    * @param option The EngineOption to create the input for.
    * @param value Reference to the current value of the option as a string.
    * @param inputWidth Optional width for a file Input field.
    * @return True if the value was modified, false otherwise.
    */
    bool engineOptionControl(const QaplaTester::EngineOption& option, 
        std::string& value, float fileInputWidth = 400.0F);

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
    bool optionalInput(const char* label, std::optional<T>& value, const std::function<bool(T&)>& inputCallback) {
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
    bool timeControlInput(std::string& timeControl, bool blitz = false, float inputWidth = 0.0);

    void drawBoxWithShadow(ImVec2 topLeft, ImVec2 bottomRight);

    void drawDot(float offsetX = 3.0F, float offsetY = 3.0F);

    /**
     * @brief Custom CollapsingHeader that can display a red dot in the upper right corner.
     * @param label Label of the collapsing header.
     * @param flags Optional ImGuiTreeNodeFlags.
     * @param showDot If true, a red dot is displayed in the upper right corner.
     * @return True if the header is open, false otherwise.
     */
    bool CollapsingHeaderWithDot(const char* label, ImGuiTreeNodeFlags flags = 0, bool showDot = false);

    /**
     * @brief Custom TabItem that can display a red dot in the upper right corner.
     * @param label Label of the tab item.
     * @param p_open Optional pointer to a boolean that indicates if the tab is open.
     * @param flags Optional ImGuiTabItemFlags.
     * @param showDot If true, a red dot is displayed in the upper right corner of the tab.
     * @return True if the tab is selected, false otherwise.
     */
    bool TabItemWithDot(const char* label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0,  bool showDot = false);

    /**
     * @brief Displays a list of options as buttons for selection.
     * @param options The list of option texts to display.
     * @return The index of the selected option, or -1 if none selected.
     */
    int optionSelector(const std::vector<std::string>& options);

}
