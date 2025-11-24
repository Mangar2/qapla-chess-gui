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

#include "chatbot-step-option-list.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepOptionList::ChatbotStepOptionList(std::string prompt, std::vector<Option> options)
    : prompt_(std::move(prompt)), options_(std::move(options)) {
}

void ChatbotStepOptionList::draw() {
    ImGui::TextWrapped("%s", prompt_.c_str());
    ImGui::Spacing();

    size_t num_options = options_.size();
    const int max_per_row = 4;
    size_t rows = (num_options + max_per_row - 1) / max_per_row;

    for (size_t row = 0; row < rows; ++row) {
        size_t start = row * max_per_row;
        size_t end = std::min(start + max_per_row, num_options);

        // Berechne die maximale Button-Breite basierend auf dem längsten Text
        float max_button_width = 0.0f;
        for (size_t i = start; i < end; ++i) {
            float text_width = ImGui::CalcTextSize(options_[i].text.c_str()).x;
            float button_width_calc = text_width + 2 * ImGui::GetStyle().FramePadding.x;
            if (button_width_calc > max_button_width) {
                max_button_width = button_width_calc;
            }
        }

        float button_width = max_button_width;

        for (size_t i = start; i < end; ++i) {
            if (QaplaWindows::ImGuiControls::textButton(options_[i].text.c_str(), ImVec2(button_width, 0))) {
                if (options_[i].onSelected) {
                    options_[i].onSelected();
                }
                finished_ = true;
                return;
            }
            if (i < end - 1) ImGui::SameLine();
        }
    }
}

bool ChatbotStepOptionList::isFinished() const {
    return finished_;
}

} // namespace QaplaWindows::ChatBot
