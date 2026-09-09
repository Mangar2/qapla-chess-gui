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

#include "chatbot-step-analysis-start.h"
#include "analysis-data.h"
#include "callback-manager.h"
#include "imgui-controls.h"
#include "imgui-game-list.h"

#include <imgui.h>

#include <format>

namespace QaplaWindows::ChatBot {

namespace {
    constexpr uint32_t MIN_CONCURRENCY = 1;
    constexpr uint32_t MAX_CONCURRENCY = 32;
}

std::string ChatbotStepAnalysisStart::draw() {
    if (finished_) {
        return "";
    }

    auto& analysisData = AnalysisData::instance();

    if (analysisData.isBusy()) {
        drawWhileRunning();
        return "";
    }
    if (started_) {
        // The run is over -- it may have been over before this step was drawn again, which is
        // what happens with a handful of short games.
        ImGuiControls::textWrapped(std::format(
            "The backward analysis is finished: {} of {} games analysed.",
            analysisData.getFinishedCount(), analysisData.getTotalCount()));
        ImGui::Spacing();
        if (ImGuiControls::textButton("Close")) {
            finished_ = true;
        }
        return "";
    }
    return drawBeforeStart();
}

std::string ChatbotStepAnalysisStart::drawBeforeStart() {
    auto& analysisData = AnalysisData::instance();
    auto* gameList = ImGuiGameList::instance();
    const size_t gameCount = gameList != nullptr ? gameList->getFilteredGameCount() : 0;

    ImGuiControls::textWrapped(
        "Set how many games are analysed at the same time and start the run. You can change the "
        "number while it runs.");
    ImGui::Spacing();

    auto concurrency = analysisData.getExternalConcurrency();
    if (ImGuiControls::sliderInt<uint32_t>("Concurrency", concurrency, MIN_CONCURRENCY, MAX_CONCURRENCY)) {
        analysisData.setExternalConcurrency(concurrency);
        analysisData.updateConfiguration();
    }
    ImGuiControls::hooverTooltip("Number of games analysed in parallel, each on an engine of its own.");

    ImGui::Spacing();
    ImGuiControls::textWrapped(std::format("{} game{} will be analysed, backwards, from the last "
        "move of each game to its first.", gameCount, gameCount == 1 ? "" : "s"));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginDisabled(gameCount == 0);
    if (ImGuiControls::textButton("Start Analysis")) {
        // Handed over now: from here the analysis works on its own copy and the Pgn view can be
        // used for anything else -- including the file this run is writing.
        analysisData.setGames(gameList->getFilteredGames());
        analysisData.analyse();
        started_ = analysisData.isBusy();
    }
    ImGuiControls::hooverTooltip(
        "Start the backward analysis with the selected engines, games and time per move.");
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }

    return "";
}

void ChatbotStepAnalysisStart::drawWhileRunning() {
    auto& analysisData = AnalysisData::instance();

    const auto finished = analysisData.getFinishedCount();
    const auto total = analysisData.getTotalCount();

    ImGuiControls::textWrapped(std::format("The backward analysis is running: {} of {} games done.",
        finished, total));
    ImGui::Spacing();
    ImGui::ProgressBar(total == 0 ? 0.0F : static_cast<float>(finished) / static_cast<float>(total),
        ImVec2(-10.0F, 20.0F));

    ImGui::Spacing();
    auto concurrency = analysisData.getExternalConcurrency();
    if (ImGuiControls::sliderInt<uint32_t>("Concurrency", concurrency, MIN_CONCURRENCY, MAX_CONCURRENCY)) {
        analysisData.setExternalConcurrency(concurrency);
        // Straight through, not debounced: the debounce counts down on every call, and this one
        // is only made when the slider moves. A number set once and then left alone would have
        // counted down to nothing and never reached the pool.
        analysisData.setPoolConcurrency(concurrency, true, true);
        analysisData.updateConfiguration();
    }
    ImGuiControls::hooverTooltip("Changes how many games are analysed at the same time, right away.");

    ImGui::Spacing();
    ImGuiControls::textWrapped(
        "The Pgn view is free again: load the output file there with Open or Recent to look at "
        "the games that are already done, while the rest are still being analysed.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGuiControls::textButton("Stop")) {
        analysisData.stop(false);
    }
    ImGuiControls::hooverTooltip("Stop the analysis. The games already written stay in the output file.");

    ImGui::SameLine();

    if (ImGuiControls::textButton("Switch to Pgn View")) {
        StaticCallbacks::message().invokeAll("switch_to_pgn_view");
    }
    ImGuiControls::hooverTooltip("Open the Pgn view to load and inspect games.");
}

} // namespace QaplaWindows::ChatBot
