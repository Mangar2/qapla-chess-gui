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

#include "chatbot-step-analysis-output.h"
#include "analysis-data.h"
#include "imgui-controls.h"

#include "ui/qapla-ui.h"

#include <filesystem>
#include <format>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepAnalysisOutput::draw() {
    if (finished_) {
        ImGuiControls::textDisabled(summary_);
        return "";
    }

    ImGuiControls::textWrapped(
        "Where should the analysed games be written? Each game is written again with the "
        "evaluations the engine found, once per analysing engine.");
    QaplaUi::spacing();

    auto& analysisData = AnalysisData::instance();
    auto& outputPgn = analysisData.outputPgn();
    const auto& recent = analysisData.recentOutputFiles().get();

    ImGuiTournamentPgn::DrawOptions options {
        .fileInputWidth = 500.0F,
        .drawDetails = showMoreOptions_,
        .showCollapsingHeader = false,
        .recentFiles = &recent
    };
    outputPgn.draw(options);

    const auto& pgnOptions = outputPgn.pgnOptions();
    const auto validation = validate(pgnOptions.file, pgnOptions.append);

    QaplaUi::spacing();
    drawStatusMessage(validation);

    QaplaUi::spacing();
    QaplaUi::separator();
    QaplaUi::spacing();

    const char* continueLabel = validation.willOverwrite ? "Overwrite & Continue" : "Continue";

    QaplaUi::beginDisabled(!validation.isValidPath || validation.isInputFile);
    if (ImGuiControls::textButton(continueLabel)) {
        finished_ = true;
        summary_ = std::format("The analysed games go to {} ({}).", pgnOptions.file,
            pgnOptions.append ? "append" : "overwrite");
    }
    QaplaUi::endDisabled();

    QaplaUi::sameLine();

    const char* optionsLabel = showMoreOptions_ ? "Less Options" : "More Options";
    if (ImGuiControls::textButton(optionsLabel)) {
        showMoreOptions_ = !showMoreOptions_;
    }
    ImGuiControls::hooverTooltip("Show or hide what is written into the move comments.");

    QaplaUi::sameLine();

    if (ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        summary_ = "The analysis was not started.";
        return "stop";
    }

    return "";
}

ChatbotStepAnalysisOutput::ValidationResult ChatbotStepAnalysisOutput::validate(
        const std::string& filePath, bool appendMode) {
    ValidationResult result;

    if (filePath.empty()) {
        return result;
    }

    try {
        const std::filesystem::path path(filePath);
        const auto parentPath = path.parent_path();
        if (parentPath.empty() || std::filesystem::exists(parentPath)) {
            result.isValidPath = true;
        }
        result.fileExists = std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
        result.willOverwrite = result.fileExists && !appendMode;

        const auto& inputFile = AnalysisData::instance().config().inputFile;
        if (!inputFile.empty()) {
            result.isInputFile = std::filesystem::weakly_canonical(path)
                == std::filesystem::weakly_canonical(std::filesystem::path(inputFile));
        }
    }
    catch (const std::filesystem::filesystem_error&) {
        // Whatever could not be looked at stays as it was: the defaults say "not usable".
    }

    return result;
}

void ChatbotStepAnalysisOutput::drawStatusMessage(const ValidationResult& validation) {
    if (!validation.isValidPath) {
        QaplaUi::pushStyleColor(QaplaUi::StyleColor::Text, StepColors::ERROR_COLOR);
        ImGuiControls::textWrapped("Please enter a valid file path. The directory must exist.");
        QaplaUi::popStyleColor();
        return;
    }
    if (validation.isInputFile) {
        QaplaUi::pushStyleColor(QaplaUi::StyleColor::Text, StepColors::ERROR_COLOR);
        ImGuiControls::textWrapped(
            "This is the file the games are read from. Choose another one, so that the games "
            "being analysed stay as they are.");
        QaplaUi::popStyleColor();
        return;
    }
    if (validation.willOverwrite) {
        QaplaUi::pushStyleColor(QaplaUi::StyleColor::Text, StepColors::WARNING_COLOR);
        ImGuiControls::textWrapped(
            "Warning: the file exists and overwrite mode is on. Its content is replaced when the "
            "analysis starts.");
        QaplaUi::popStyleColor();
        return;
    }
    if (validation.fileExists) {
        QaplaUi::pushStyleColor(QaplaUi::StyleColor::Text, StepColors::SUCCESS_COLOR);
        ImGuiControls::textWrapped("The file exists. The analysed games are appended to it.");
        QaplaUi::popStyleColor();
    }
}

} // namespace QaplaWindows::ChatBot
