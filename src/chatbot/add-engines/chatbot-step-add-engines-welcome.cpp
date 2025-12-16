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
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "chatbot-step-add-engines-welcome.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepAddEnginesWelcome::ChatbotStepAddEnginesWelcome() = default;

std::string ChatbotStepAddEnginesWelcome::draw() {
    QaplaWindows::ImGuiControls::textWrapped(
        "Welcome to the Add Engines wizard! This will help you add new chess engines "
        "to your global engine list.");
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    QaplaWindows::ImGuiControls::textWrapped(
        "Important: Each engine can only be added once. The file path must be unique. "
        "However, you can use the same engine multiple times with different configurations "
        "in tournaments, EPD analysis, and interactive boards.");
    
    ImGui::Spacing();
    
    QaplaWindows::ImGuiControls::textWrapped(
        "After adding engines, they will be available in all features that require engines.");
    
    ImGui::Spacing();
    ImGui::Spacing();
    
    if (!finished_) {
        if (QaplaWindows::ImGuiControls::textButton("Continue")) {
            finished_ = true;
            return "";
        }
        
        ImGui::SameLine();
        
        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
            return "stop";
        }
    }
    
    return "";
}

} // namespace QaplaWindows::ChatBot
