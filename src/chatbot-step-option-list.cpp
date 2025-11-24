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

ChatbotStepOptionList::ChatbotStepOptionList(std::string prompt, std::vector<Option> options)
    : m_prompt(std::move(prompt)), m_options(std::move(options)) {
}

void ChatbotStepOptionList::draw() {
    ImGui::TextWrapped("%s", m_prompt.c_str());
    ImGui::Spacing();

    for (const auto& option : m_options) {
        if (QaplaWindows::ImGuiControls::textButton(option.text.c_str())) {
            if (option.onSelected) {
                option.onSelected();
            }
            m_finished = true;
        }
    }
}

bool ChatbotStepOptionList::isFinished() const {
    return m_finished;
}
