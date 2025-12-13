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

#include "chatbot-step-board-time-control.h"
#include "../../time-control-window.h"
#include "../../imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepBoardTimeControl::ChatbotStepBoardTimeControl(TimeControlProvider provider)
    : provider_(std::move(provider)) {}

TimeControlWindow* ChatbotStepBoardTimeControl::getTimeControlWindow() {
    return provider_();
}

std::string ChatbotStepBoardTimeControl::draw() {

    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            "Configure the time control for your board game. "
            "By default, you can choose a Blitz time setting. "
            "Click 'More Options' to access all time control modes.");
        ImGui::Spacing();
    }

    auto* timeControlWindow = getTimeControlWindow();
    
    // Check if target still exists (e.g., board not closed)
    if (timeControlWindow == nullptr) {
        QaplaWindows::ImGuiControls::textWrapped("Error: Board no longer exists.");
        finished_ = true;
        return "stop";
    }
    
    // Draw time control with options
    TimeControlWindow::DrawOptions options;
    options.showOnlyBlitz = !showMoreOptions_;
    
    timeControlWindow->draw(options);

    if (finished_) {
        return "";
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (QaplaWindows::ImGuiControls::textButton("Continue")) {
        finished_ = true;
    }

    ImGui::SameLine();

    const char* optionsLabel = showMoreOptions_ ? "Less Options" : "More Options";
    if (QaplaWindows::ImGuiControls::textButton(optionsLabel)) {
        showMoreOptions_ = !showMoreOptions_;
    }
    QaplaWindows::ImGuiControls::hooverTooltip(
        "Show or hide advanced time control options, such as Tournament, "
        "Time per Move, Fixed Depth, and Nodes per Move.");

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }

    return "";
}

} // namespace QaplaWindows::ChatBot
