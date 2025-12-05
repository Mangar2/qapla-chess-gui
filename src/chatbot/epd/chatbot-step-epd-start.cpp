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

#include "chatbot-step-epd-start.h"
#include "epd-data.h"
#include "imgui-controls.h"
#include "callback-manager.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepEpdStart::draw() {
    if (finished_) {
        return "";
    }

    auto& epdData = EpdData::instance();

    if (epdData.isStarting()) {
        QaplaWindows::ImGuiControls::textWrapped("The EPD analysis is starting up, please wait...");
        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
        }
        return "";
    }

    if (!epdData.isRunning()) {
        QaplaWindows::ImGuiControls::textWrapped("Configure EPD analysis concurrency and start:");
        ImGui::Spacing();
        
        auto concurrency = epdData.getExternalConcurrency();
        ImGuiControls::sliderInt<uint32_t>("Concurrency", concurrency, 1, 16);
        epdData.setExternalConcurrency(concurrency);
        QaplaWindows::ImGuiControls::hooverTooltip("Number of positions to analyze in parallel");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (QaplaWindows::ImGuiControls::textButton("Start Analysis")) {
            epdData.analyse();
        }

        ImGui::SameLine();

        if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
            finished_ = true;
            return "stop";
        }
    } else {
        QaplaWindows::ImGuiControls::textWrapped("The EPD analysis started successfully!");
        ImGui::Spacing();
        QaplaWindows::ImGuiControls::textWrapped(
            "You can switch between the running analyses and the chatbot using the tabs at the top of the window. "
            "Each position has its own tab, so you can easily navigate between them.");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (QaplaWindows::ImGuiControls::textButton("Switch to EPD View")) {
            StaticCallbacks::message().invokeAll("switch_to_epd_view");
            finished_ = true;
        }
        ImGui::SameLine();
        if (QaplaWindows::ImGuiControls::textButton("Stay in Chatbot")) {
            finished_ = true;
        }
    }
    return "";
}

} // namespace QaplaWindows::ChatBot
