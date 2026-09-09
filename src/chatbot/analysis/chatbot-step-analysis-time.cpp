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

#include "chatbot-step-analysis-time.h"
#include "analysis-data.h"
#include "imgui-controls.h"

#include <imgui.h>

#include <format>

namespace QaplaWindows::ChatBot {

namespace {
    constexpr uint64_t MIN_MOVE_TIME_MS = 10;
    constexpr uint64_t MAX_MOVE_TIME_MS = 600000;
}

std::string ChatbotStepAnalysisTime::draw() {
    if (finished_) {
        ImGuiControls::textDisabled(summary_);
        return "";
    }

    auto& analysisData = AnalysisData::instance();

    ImGuiControls::textWrapped(
        "How long may the engine think about each position? Every position of every game is given "
        "the same time, which is what makes their evaluations comparable.");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(150.0F);
    auto moveTimeMs = analysisData.config().moveTimeMs;
    if (ImGuiControls::inputInt<uint64_t>("Time per move (ms)", moveTimeMs,
            MIN_MOVE_TIME_MS, MAX_MOVE_TIME_MS, 100, 1000)) {
        analysisData.config().moveTimeMs = moveTimeMs;
        analysisData.updateConfiguration();
    }
    ImGuiControls::hooverTooltip(
        "The search time for a single position. A game of 80 half moves takes 80 times this.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGuiControls::textButton("Continue")) {
        finished_ = true;
        summary_ = std::format("Every position gets {} ms.", analysisData.config().moveTimeMs);
    }

    ImGui::SameLine();

    if (ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        summary_ = "The analysis was not started.";
        return "stop";
    }

    return "";
}

} // namespace QaplaWindows::ChatBot
