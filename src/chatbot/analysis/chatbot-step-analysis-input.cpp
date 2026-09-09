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

#include "chatbot-step-analysis-input.h"
#include "analysis-data.h"
#include "callback-manager.h"
#include "imgui-controls.h"
#include "os-dialogs.h"
#include "imgui-game-list.h"

#include <imgui.h>

#include <filesystem>
#include <format>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepAnalysisInput::draw() {
    if (finished_) {
        ImGuiControls::textDisabled(summary_);
        return "";
    }

    if (state_ == State::Loading) {
        drawLoading();
        return "";
    }
    return drawChoosing();
}

std::string ChatbotStepAnalysisInput::drawChoosing() {
    ImGuiControls::textWrapped(
        "Select the PGN file with the games to analyse. The games are loaded into the Pgn view, "
        "so you can see and filter them before the analysis starts.");
    ImGui::Spacing();

    auto& analysisData = AnalysisData::instance();
    auto& filePath = analysisData.config().inputFile;

    if (ImGuiControls::recentFileInput("Pgn file", filePath,
            analysisData.recentInputFiles().get(),
            []() { const auto files = OsDialogs::openPgnFile(); return files.empty() ? std::string{} : files.front(); },
            500.0F)) {
        analysisData.updateConfiguration();
    }

    const bool fileExists = !filePath.empty() && std::filesystem::exists(filePath)
        && std::filesystem::is_regular_file(filePath);

    ImGui::Spacing();
    if (!filePath.empty() && !fileExists) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::ERROR_COLOR);
        ImGuiControls::textWrapped("This file does not exist. Choose one that does.");
        ImGui::PopStyleColor();
    }

    auto* gameList = ImGuiGameList::instance();
    if (gameList == nullptr) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::ERROR_COLOR);
        ImGuiControls::textWrapped("The Pgn view is not available, the games cannot be loaded.");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginDisabled(!fileExists || gameList == nullptr);
    if (ImGuiControls::textButton("Load Games")) {
        // The Pgn tab comes to the front with the games: the next step filters what is shown
        // there, and a filter over a table nobody can see would be guesswork.
        StaticCallbacks::message().invokeAll("switch_to_pgn_view");
        gameList->loadPgnFile(filePath);
        analysisData.recentInputFiles().add(filePath);
        analysisData.updateConfiguration();
        state_ = State::Loading;
    }
    ImGuiControls::hooverTooltip("Load the file into the Pgn view and continue.");
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        summary_ = "No file selected, the analysis was not started.";
        return "stop";
    }

    return "";
}

void ChatbotStepAnalysisInput::drawLoading() {
    auto* gameList = ImGuiGameList::instance();
    if (gameList == nullptr) {
        summary_ = "The Pgn view went away while the file was being read.";
        finished_ = true;
        return;
    }
    if (gameList->isLoading()) {
        ImGuiControls::textWrapped("Loading the games, please wait...");
        return;
    }

    const auto count = gameList->getLoadedGameCount();
    summary_ = std::format("Loaded {} game{} from {}.", count, count == 1 ? "" : "s",
        gameList->getLoadedFileName());
    finished_ = true;
}

} // namespace QaplaWindows::ChatBot
