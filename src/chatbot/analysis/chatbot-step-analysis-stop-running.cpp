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

#include "chatbot-step-analysis-stop-running.h"
#include "analysis-data.h"
#include "imgui-controls.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepAnalysisStopRunning::draw() {
    if (finished_) {
        ImGuiControls::textDisabled(finishedMessage_);
        return "";
    }

    auto& analysisData = AnalysisData::instance();

    if (!analysisData.isBusy()) {
        finished_ = true;
        return "continue";
    }

    ImGuiControls::textWrapped(
        "A backward analysis is currently running. Would you like to stop it?");

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGuiControls::textButton("Yes, stop analysis")) {
        analysisData.stop(false);
        finishedMessage_ = "Backward analysis stopped.";
        finished_ = true;
        return "continue";
    }
    ImGuiControls::hooverTooltip(
        "Stop the running analysis. The games already written to the output file stay there.");

    ImGui::SameLine();

    if (ImGuiControls::textButton("Cancel")) {
        finishedMessage_ = "The backward analysis continues.";
        finished_ = true;
        return "stop";
    }

    return "";
}

} // namespace QaplaWindows::ChatBot
