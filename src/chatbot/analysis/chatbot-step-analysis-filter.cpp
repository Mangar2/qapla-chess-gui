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

#include "chatbot-step-analysis-filter.h"
#include "callback-manager.h"
#include "imgui-controls.h"
#include "imgui-game-list.h"

#include <imgui.h>

#include <format>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepAnalysisFilter::draw() {
    if (finished_) {
        ImGuiControls::textDisabled(summary_);
        return "";
    }

    auto* gameList = ImGuiGameList::instance();
    if (gameList == nullptr) {
        finished_ = true;
        summary_ = "The Pgn view is not available, nothing can be filtered.";
        return "stop";
    }

    const auto loaded = gameList->getLoadedGameCount();
    const auto filtered = gameList->getFilteredGameCount();

    ImGuiControls::textWrapped(
        "Choose which of the loaded games to analyse. The filter is the one of the Pgn view; "
        "leave it alone to analyse all of them.");
    ImGui::Spacing();

    if (filtered == loaded) {
        ImGuiControls::textWrapped(std::format("All {} games will be analysed.", loaded));
    } else {
        ImGuiControls::textWrapped(
            std::format("{} of {} games pass the filter.", filtered, loaded));
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGuiControls::textButton("Filter Games")) {
        gameList->requestFilterDialog();
        StaticCallbacks::message().invokeAll("switch_to_pgn_view");
    }
    ImGuiControls::hooverTooltip("Open the filter dialog of the Pgn view.");

    ImGui::SameLine();

    ImGui::BeginDisabled(filtered == 0 || gameList->isFilterDialogOpen());
    if (ImGuiControls::textButton("Continue")) {
        finished_ = true;
        summary_ = std::format("{} of {} games will be analysed.", filtered, loaded);
    }
    ImGuiControls::hooverTooltip("Continue with the games the filter lets through.");
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        summary_ = "The analysis was not started.";
        return "stop";
    }

    return "";
}

} // namespace QaplaWindows::ChatBot
