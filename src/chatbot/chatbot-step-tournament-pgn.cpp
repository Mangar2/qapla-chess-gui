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

#include "chatbot-step-tournament-pgn.h"
#include "chatbot-step.h"
#include "tournament-data.h"
#include "sprt-tournament-data.h"
#include "imgui-controls.h"
#include <imgui.h>
#include <filesystem>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentPgn::ChatbotStepTournamentPgn(EngineSelectContext type)
    : type_(type) {}

ImGuiTournamentPgn& ChatbotStepTournamentPgn::getTournamentPgn() {
    if (type_ == EngineSelectContext::SPRT) {
        return SprtTournamentData::instance().tournamentPgn();
    }
    return TournamentData::instance().tournamentPgn();
}

std::string ChatbotStepTournamentPgn::draw() {

    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            "Select the PGN file where tournament results will be saved. "
            "All games played during the tournament will be recorded in this file.");
        ImGui::Spacing();
    }

    auto& tournamentPgn = getTournamentPgn();
    
    ImGuiTournamentPgn::DrawOptions options {
        .fileInputWidth = 500.0F,
        .drawDetails = showMoreOptions_,
        .showCollapsingHeader = false
    };
    tournamentPgn.draw(options);

    const auto& pgnOptions = tournamentPgn.pgnOptions();
    auto validation = validateFilePath(pgnOptions.file, pgnOptions.append);

    ImGui::Spacing();
    drawStatusMessage(validation);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (finished_) {
        return "";
    }

    return drawButtons(validation);
}

ChatbotStepTournamentPgn::ValidationResult ChatbotStepTournamentPgn::validateFilePath(
        const std::string& filePath, bool appendMode) {
    ValidationResult result;
    
    if (filePath.empty()) {
        return result;
    }
    
    try {
        std::filesystem::path path(filePath);
        auto parentPath = path.parent_path();
        if (parentPath.empty() || std::filesystem::exists(parentPath)) {
            result.isValidPath = true;
        }
        result.fileExists = std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
        result.willOverwrite = result.fileExists && !appendMode;
    } catch (const std::filesystem::filesystem_error&) {
        // Path validation failed, keep defaults
    }
    
    return result;
}

void ChatbotStepTournamentPgn::drawStatusMessage(const ValidationResult& validation) {
    if (!validation.isValidPath) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::ERROR_COLOR);
        QaplaWindows::ImGuiControls::textWrapped(
            "Please enter a valid file path. The directory must exist.");
        ImGui::PopStyleColor();
    } else if (validation.willOverwrite) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::WARNING_COLOR);
        QaplaWindows::ImGuiControls::textWrapped(
            "Warning: The file already exists and overwrite mode is enabled. "
            "The existing content will be replaced when the tournament starts.");
        ImGui::PopStyleColor();
    } else if (validation.fileExists) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::SUCCESS_COLOR);
        QaplaWindows::ImGuiControls::textWrapped(
            "The file exists. New games will be appended to the existing content.");
        ImGui::PopStyleColor();
    }
}

std::string ChatbotStepTournamentPgn::drawButtons(const ValidationResult& validation) {
    const char* continueLabel = validation.willOverwrite ? "Overwrite & Continue" : "Continue";
    
    ImGui::BeginDisabled(!validation.isValidPath);
    if (QaplaWindows::ImGuiControls::textButton(continueLabel)) {
        finished_ = true;
    }
    if (std::string(continueLabel) == "Overwrite & Continue") {
        QaplaWindows::ImGuiControls::hooverTooltip("This will overwrite the specified PGN file with new results. Make sure this is what you want before continuing.");
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    const char* optionsLabel = showMoreOptions_ ? "Less Options" : "More Options";
    if (QaplaWindows::ImGuiControls::textButton(optionsLabel)) {
        showMoreOptions_ = !showMoreOptions_;
    }
    QaplaWindows::ImGuiControls::hooverTooltip("Show or hide additional PGN options (append mode, headers, etc.).");

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }

    return "";
}

} // namespace QaplaWindows::ChatBot
