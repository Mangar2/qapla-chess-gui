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

#include "chatbot-step-epd-continue-existing.h"
#include "epd-data.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

bool ChatbotStepEpdContinueExisting::hasIncompleteAnalysis() const {
    const auto& epdData = EpdData::instance();
    // Analysis can be continued if:
    // - State is Stopped (not running, not cleared)
    // - There are total tests (analysis was started at some point)
    // - There are remaining tests (not all completed)
    return epdData.state == EpdData::State::Stopped && 
           epdData.totalTests > 0 && 
           epdData.remainingTests > 0;
}

std::string ChatbotStepEpdContinueExisting::draw() {
    // If no incomplete analysis exists, skip this step and go to menu
    if (!hasIncompleteAnalysis()) {
        finished_ = true;
        return "menu";
    }

    // If already finished, just show the message
    if (finished_) {
        QaplaWindows::ImGuiControls::textDisabled(finishedMessage_);
        return "";
    }

    auto& epdData = EpdData::instance();
    
    // Show information about the existing analysis
    std::string message = "There is an existing EPD analysis that can be continued. "
        "Progress: " + std::to_string(epdData.totalTests - epdData.remainingTests) + 
        " of " + std::to_string(epdData.totalTests) + " tests completed.\n\n"
        "Would you like to continue it?";
    QaplaWindows::ImGuiControls::textWrapped(message.c_str());
    
    ImGui::Spacing();
    ImGui::Spacing();

    if (QaplaWindows::ImGuiControls::textButton("Yes, continue analysis")) {
        finishedMessage_ = "Continuing existing EPD analysis.";
        finished_ = true;
        return "start";
    }
    
    ImGui::SameLine();
    if (QaplaWindows::ImGuiControls::textButton("No")) {
        finishedMessage_ = "";
        finished_ = true;
        return "menu";
    }

    ImGui::SameLine();
    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finishedMessage_ = "";
        finished_ = true;
        return "stop";
    }
    
    return "";
}

} // namespace QaplaWindows::ChatBot
