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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include "chatbot-step-bookeval-limit.h"
#include "book-eval-data.h"
#include "imgui-controls.h"

#include <imgui.h>

#include <format>
#include <vector>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepBookEvalLimit::draw() {
    if (finished_) {
        ImGuiControls::textDisabled(summary_);
        return "";
    }

    auto& data = BookEvalData::instance();
    auto& config = data.config();

    ImGuiControls::textWrapped(
        "How far may the engine search each position? Every position gets the same limit; a fixed "
        "depth gives the same scores on every machine.");
    ImGui::Spacing();

    static const std::vector<std::string> limits = {
        BookEvalData::LIMIT_DEPTH, BookEvalData::LIMIT_NODES, BookEvalData::LIMIT_MOVETIME };
    ImGui::SetNextItemWidth(150.0F);
    if (ImGuiControls::selectionBox("Limit", config.limit, limits)) {
        data.updateConfiguration();
    }
    ImGui::SetNextItemWidth(150.0F);
    const char* label = config.limit == BookEvalData::LIMIT_NODES ? "Nodes"
        : config.limit == BookEvalData::LIMIT_MOVETIME ? "Time per position (ms)" : "Depth";
    if (ImGuiControls::inputInt<uint64_t>(label, config.limitValue, 1, 1000000000, 1, 10)) {
        data.updateConfiguration();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGuiControls::textButton("Continue")) {
        finished_ = true;
        summary_ = std::format("Every position is searched with {}.", data.timeControl());
    }
    ImGui::SameLine();
    if (ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        summary_ = "The evaluation was not started.";
        return "stop";
    }
    return "";
}

} // namespace QaplaWindows::ChatBot
