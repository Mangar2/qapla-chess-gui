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

#include "chatbot-step-epd-stop-running.h"
#include "epd-data.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepEpdStopRunning::draw() {
    if (finished_) {
        QaplaWindows::ImGuiControls::textDisabled(finishedMessage_);
        return "";
    }
    
    auto& epdData = EpdData::instance();
    
    if (!epdData.isRunning() && !epdData.isStarting()) {
        finished_ = true;
        return "continue";
    }

    QaplaWindows::ImGuiControls::textWrapped(
        "An EPD analysis is currently running. Would you like to stop it?");
    
    ImGui::Spacing();
    ImGui::Spacing();

    if (QaplaWindows::ImGuiControls::textButton("Yes, stop analysis")) {
        epdData.stopPool(false);
        finishedMessage_ = "EPD analysis stopped.";
        finished_ = true;
        return "continue";
    }

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finishedMessage_ = "EPD analysis continues.";
        finished_ = true;
        return "stop";
    }
    
    return "";
}

} // namespace QaplaWindows::ChatBot
