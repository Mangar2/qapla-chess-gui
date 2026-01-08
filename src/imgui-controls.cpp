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

#include "imgui-controls.h"

#include "os-dialogs.h"
#include "snackbar.h"
#include "i18n.h"
#include "base-elements/string-helper.h"

#include "base-elements/time-control.h"
#include "engine-handling/engine-option.h"

#include <imgui.h>
#include <string>
#include <optional>
#include <functional>
#include <algorithm>

namespace QaplaWindows::ImGuiControls {

    void hooverTooltip(const std::string& text) {
        if (ImGui::IsItemHovered() && !text.empty()) {
            auto translatedText = Translator::instance().translate("Tooltip", text);
            ImGui::SetTooltip("%s", translatedText.c_str());
        }
    }

    bool checkbox(const char* label, bool& value) {
        auto modLabel = createLabel("Checkbox", label);
        return ImGui::Checkbox(modLabel.c_str(), &value);
    }

    bool textButton(const char* label, ImVec2 size) {
        auto modLabel = createLabel("Button", label);
        return ImGui::Button(modLabel.c_str(), size);
    }

    void textWrapped(const std::string& text) {
        if (text.empty()) {
            return;
        }
        auto translatedText = Translator::instance().translate("Text", text);
        ImGui::TextWrapped("%s", translatedText.c_str());
    }

    void textDisabled(const std::string& text) {
        if (text.empty()) {
            return;
        }
        auto translatedText = Translator::instance().translate("Text", text);
        ImGui::TextDisabled("%s", translatedText.c_str());
    }

    void annotate(const std::string& text, bool red) {
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

    std::optional<std::string> inputText(const char* label,
        const std::string& value,
        ImGuiInputTextFlags flags,
        ImGuiInputTextCallback callback,
        void* userData) {
        std::string buffer = value;
        buffer.resize(1024);  // Fixed buffer size

        auto modLabel = createLabel("Input", label);
        if (ImGui::InputText(modLabel.c_str(), buffer.data(), buffer.size(), flags, callback, userData)) {
            size_t nullPos = buffer.find('\0');
            return buffer.substr(0, nullPos);
        }

        return std::nullopt;
    }

    bool inputText(const char* label, std::string& value,
        ImGuiInputTextFlags flags,
        ImGuiInputTextCallback callback,
        void* userData) 
    {
        const auto& newText = value;
        auto result = inputText(label, newText, flags, callback, userData);
        if (result) {
            value = *result;
            return true;
        }
        return false;
    }

    bool inputFloat(const char* label, float& value, float min, float max, 
        float step, float stepFast, ImGuiInputTextFlags flags) {
        assert(min < max && "Min must be less than max");

        auto modLabel = createLabel("Input", label);
        bool modified = ImGui::InputFloat(modLabel.c_str(), &value, step, stepFast, "%.1f", flags);

        if (value < min) value = min;
        if (value > max) value = max;

        return modified;
    }

    bool inputPromille(const char* label, double& value, double min, double max, double step) {
        assert(min < max && "Min must be less than max");
        assert(step > 0.0 && "Step must be positive");

        int32_t promilleValue = static_cast<int32_t>(value * 1000.0);
        int32_t promilleMin = static_cast<int32_t>(min * 1000.0);
        int32_t promilleMax = static_cast<int32_t>(max * 1000.0);
        int32_t promilleStep = static_cast<int32_t>(step * 1000.0);
        
        auto modLabel = createLabel("Input", label);

        bool modified = ImGui::InputInt(modLabel.c_str(), &promilleValue, promilleStep, promilleStep * 10);

        promilleValue = std::clamp(promilleValue, promilleMin, promilleMax);

        if (modified) {
            value = static_cast<double>(promilleValue) / 1000.0;
            value = std::clamp(value, min, max);
        }

        return modified;
    }

    bool existingFileInput(const std::string& label, std::string& filePath, 
        float inputWidth, const char* buttonLabel) {
        bool modified = false;
		ImGui::PushID(label.c_str()); 
        // Display label
        ImGui::TextUnformatted(label.c_str());

        // File selection button
        if (textButton(buttonLabel)) {
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

    bool existingDirectoryInput(const std::string& label, std::string& directoryPath, 
        float inputWidth, const char* buttonLabel) {
        bool modified = false;
        ImGui::PushID(label.c_str()); 
        
        // Display label
        ImGui::TextUnformatted(label.c_str());

        // Directory selection button
        if (ImGuiControls::textButton(buttonLabel)) {
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

    bool newFileInput(const std::string& label, std::string& filePath,
        const std::vector<std::pair<std::string, std::string>>& filters,
        float inputWidth, const char* buttonLabel) {
        
        bool modified = false;
        ImGui::PushID(label.c_str());

        ImGui::TextUnformatted(label.c_str());

        if (ImGuiControls::textButton(buttonLabel)) {
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

    bool selectionBox(const char* label, int& currentItem, const std::vector<std::string>& options) {
        bool modified = false;
        bool isIndex = currentItem >= 0 && currentItem < static_cast<int>(options.size());
        
        auto modLabel = createLabel("Input", label);
        if (ImGui::BeginCombo(modLabel.c_str(), isIndex ? options[currentItem].c_str() : "Custom"
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

    bool selectionBox(const char* label, std::string& currentItem, const std::vector<std::string>& options) {

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

    bool booleanInput(const char* label, bool& value) {
        // Define the options for the selection box
        static std::vector<std::string> options = {
            Translator::instance().translate("Option", "No"),
            Translator::instance().translate("Option", "Yes")
        };

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

    bool triStateInput(const char* label, bool& major, bool& minor, 
        std::vector<std::string> labels) {

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

    bool checkbox(std::string label, bool& value) {
        return ImGui::Checkbox(label.c_str(), &value);
    }

    bool collapsingSelection(std::string label, bool& selected, ImGuiTreeNodeFlags flags,
        const std::function<bool()>& contentCallback){

        bool modified = false;
        modified = ImGui::Checkbox("##select", &selected);
        ImGui::SameLine(0.0F, 4.0F);
        // Collapsing selection are used to select content items and are thus not translated.
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

    bool engineOptionControl(const QaplaTester::EngineOption& option, 
        std::string& value, float fileInputWidth) {
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
                modified |= existingDirectoryInput(option.name, value);
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

    bool timeControlInput(std::string& timeControl, bool blitz, float inputWidth) {

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

    void drawBoxWithShadow(ImVec2 topLeft, ImVec2 bottomRight)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImGuiStyle& style = ImGui::GetStyle();

        ImU32 borderCol = ImGui::GetColorU32(ImGuiCol_Border);
        ImU32 shadowCol = ImGui::GetColorU32(ImGuiCol_BorderShadow);

        const float shadowSpread = 2.0F; 
        auto shadowMin = ImVec2(topLeft.x + shadowSpread, topLeft.y + shadowSpread);
        auto shadowMax = ImVec2(bottomRight.x + shadowSpread, bottomRight.y + shadowSpread);
        drawList->AddRect(shadowMin, shadowMax, shadowCol, style.FrameRounding + shadowSpread);

        drawList->AddRect(topLeft, bottomRight, borderCol, style.FrameRounding, 0, 2.0F);
    }

    void drawDot(float offsetX, float offsetY) {
        if (ImGui::IsItemVisible()) {
            ImVec2 tabMin = ImGui::GetItemRectMin();
            ImVec2 tabMax = ImGui::GetItemRectMax();

            constexpr float dotRadius = 6.0F;
            auto dotPos = ImVec2(tabMax.x - offsetX, tabMin.y + offsetY);

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddCircleFilled(dotPos, dotRadius, IM_COL32(192, 0, 0, 192));
        }   
    }

    bool CollapsingHeaderWithDot(const char* label, ImGuiTreeNodeFlags flags, bool showDot, bool translate) {

        auto modLabel = translate ? createLabel("Section", label) : std::string(label);
        bool result = ImGui::CollapsingHeader(modLabel.c_str(), flags);
        
        if (showDot) {
            constexpr float dotOffsetX = 20.0F;  // More offset for CollapsingHeader arrow
            constexpr float dotOffsetY = 10.0F;
            drawDot(dotOffsetX, dotOffsetY);
        }
        
        return result;
    }

    bool TabItemWithDot(const char* label, bool* p_open, ImGuiTabItemFlags flags, bool showDot) {
        bool result = ImGui::BeginTabItem(label, p_open, flags);
        
        if (showDot) {
            drawDot();
        }
        
        return result;
    }

    int optionSelector(const std::vector<std::string>& options) {
        for (size_t i = 0; i < options.size(); ++i) {
            auto modLabel = createLabel("Option", options[i]);
            if (textButton(modLabel.c_str())) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

}
