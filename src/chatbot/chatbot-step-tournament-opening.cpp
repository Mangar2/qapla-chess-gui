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

#include "chatbot-step-tournament-opening.h"
#include "tournament-data.h"
#include "imgui-controls.h"
#include <imgui.h>
#include <filesystem>
#include <format>
#include <sstream>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentOpening::ChatbotStepTournamentOpening() = default;

std::string ChatbotStepTournamentOpening::draw() {

    if (!finished_) {
        QaplaWindows::ImGuiControls::textWrapped(
            "Configure the opening book settings for the tournament. "
            "You must specify an opening file (EPD, PGN, or raw FEN).");
        ImGui::Spacing();
    }

    auto& tournamentOpening = TournamentData::instance().tournamentOpening();
    
    ImGuiTournamentOpening::DrawParams drawParams{
        .inputWidth = 150.0F,
        .fileInputWidth = 500.0F,
        .indent = 10.0F,
        .alwaysOpen = true,
        .showOpeningFile = true,
        .showOrder = true,
        .showPlies = showMoreOptions_,
        .showFirstOpening = showMoreOptions_,
        .showRandomSeed = showMoreOptions_,
        .showSwitchPolicy = showMoreOptions_
    };
    
    tournamentOpening.draw(drawParams);

    // More/Less Options button
    if (!finished_) {
        ImGui::Spacing();
        const char* optionsLabel = showMoreOptions_ ? "Less Options" : "More Options";
        if (QaplaWindows::ImGuiControls::textButton(optionsLabel)) {
            showMoreOptions_ = !showMoreOptions_;
        }
    }

    ImGui::Spacing();
    
    // Show status or validation result
    if (validationState_ == ValidationState::NotValidated) {
        drawStatusMessage();
    } else {
        drawValidationResult();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (finished_) {
        return "";
    }

    return drawButtons();
}

void ChatbotStepTournamentOpening::drawStatusMessage() {
    const auto& openings = TournamentData::instance().tournamentOpening().openings();
    
    if (openings.file.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::ERROR_COLOR);
        QaplaWindows::ImGuiControls::textWrapped(
            "An opening file is required. Please select a valid EPD, PGN, or FEN file.");
        ImGui::PopStyleColor();
    } else if (!doesOpeningFileExist()) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::ERROR_COLOR);
        QaplaWindows::ImGuiControls::textWrapped(
            "The specified opening file does not exist. Please select a valid file.");
        ImGui::PopStyleColor();
    } else {
        // Automatically validate when file exists and changed
        if (openings.file != lastValidatedFile_) {
            validateOpeningFile();
        } else {
            drawValidationResult();
        }
    }
}

void ChatbotStepTournamentOpening::drawValidationResult() {
    if (!parseResult_) {
        return;
    }

    if (parseResult_->success()) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::SUCCESS_COLOR);
        std::string message = std::format(
            "Successfully parsed opening file using {} parser.\n"
            "Found {} opening position(s).",
            parseResult_->successfulParser,
            parseResult_->games.size()
        );
        QaplaWindows::ImGuiControls::textWrapped(message.c_str());
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::ERROR_COLOR);
        QaplaWindows::ImGuiControls::textWrapped(
            "Failed to parse opening file. Please check the file format and try again.");
        ImGui::PopStyleColor();
    }

    // Show/Hide Trace button
    ImGui::Spacing();
    const char* traceLabel = showTrace_ ? "Hide Trace" : "Show Trace";
    if (QaplaWindows::ImGuiControls::textButton(traceLabel)) {
        showTrace_ = !showTrace_;
    }

    if (showTrace_) {
        ImGui::Spacing();
        std::string traceText = formatTrace();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 0.7F, 1.0F));
        QaplaWindows::ImGuiControls::textWrapped(traceText.c_str());
        ImGui::PopStyleColor();
    }
}

std::string ChatbotStepTournamentOpening::drawButtons() {
    bool isValidated = validationState_ == ValidationState::Success;

    // Continue button - only enabled after successful validation
    ImGui::BeginDisabled(!isValidated);
    if (QaplaWindows::ImGuiControls::textButton("Continue")) {
        finished_ = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }

    return "";
}

bool ChatbotStepTournamentOpening::doesOpeningFileExist() const {
    const auto& openings = TournamentData::instance().tournamentOpening().openings();
    
    if (openings.file.empty()) {
        return false;
    }
    
    try {
        std::filesystem::path path(openings.file);
        return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

void ChatbotStepTournamentOpening::validateOpeningFile() {
    const auto& openings = TournamentData::instance().tournamentOpening().openings();
    
    lastValidatedFile_ = openings.file;
    
    QaplaTester::OpeningParser parser;
    parseResult_ = parser.parseWithTrace(openings.file);
    
    if (parseResult_->success()) {
        validationState_ = ValidationState::Success;
    } else {
        validationState_ = ValidationState::Error;
    }
    
    drawValidationResult();
}

std::string ChatbotStepTournamentOpening::formatTrace() const {
    if (!parseResult_) {
        return "";
    }

    std::ostringstream oss;
    oss << "Parser Trace:\n";
    oss << "File: " << parseResult_->filePath << "\n\n";

    for (const auto& entry : parseResult_->trace) {
        oss << "[" << entry.parserName << "] " 
            << (entry.success ? "SUCCESS" : "FAILED") << "\n";
        for (const auto& msg : entry.messages) {
            oss << "  " << msg << "\n";
        }
        oss << "\n";
    }

    return oss.str();
}

} // namespace QaplaWindows::ChatBot
