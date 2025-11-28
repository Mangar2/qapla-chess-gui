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
 * @author GitHub Copilot
 * @copyright Copyright (c) 2025 GitHub Copilot
 */

#include "chatbot-step-select-option.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepSelectOption::ChatbotStepSelectOption(std::string prompt, std::vector<std::string> options, std::function<void(int)> onSelected)
    : prompt_(std::move(prompt)), options_(std::move(options)), onSelected_(std::move(onSelected)) {
}

std::string ChatbotStepSelectOption::draw() {
    ImGuiControls::textWrapped(prompt_);
    ImGui::Spacing();

    int selected = QaplaWindows::ImGuiControls::optionSelector(options_);
    if (selected != -1) {
        if (onSelected_) {
            onSelected_(selected);
        }
        finished_ = true;
    }
    return "";
}

} // namespace QaplaWindows::ChatBot
