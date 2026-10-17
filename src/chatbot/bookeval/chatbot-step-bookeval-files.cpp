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

#include "chatbot-step-bookeval-files.h"
#include "book-eval-data.h"
#include "imgui-controls.h"
#include "os-dialogs.h"

#include <imgui.h>

#include <filesystem>
#include <format>

namespace QaplaWindows::ChatBot {

std::string ChatbotStepBookEvalFiles::draw() {
    if (finished_) {
        ImGuiControls::textDisabled(summary_);
        return "";
    }

    auto& data = BookEvalData::instance();
    auto& config = data.config();

    ImGuiControls::textWrapped(
        "Select the 9 bit book whose positions are evaluated, and the file the evaluated book is "
        "written to. Every position of the book is searched once - the start position and the one after "
        "every move - and every move gets the score of the position it leads to; the scores "
        "go into the new book.");
    ImGui::Spacing();

    if (ImGuiControls::recentFileInput("Book", config.bookFile, data.recentBookFiles().get(),
            []() { const auto files = OsDialogs::openBookFile(); return files.empty() ? std::string{} : files.front(); },
            500.0F)) {
        if (config.outFile.empty()) {
            config.outFile = BookEvalData::suggestedOutFile(config.bookFile);
        }
        data.updateConfiguration();
    }
    ImGui::Spacing();
    if (ImGuiControls::recentFileInput("Evaluated book", config.outFile, data.recentOutFiles().get(),
            [&config]() { return OsDialogs::saveBookFile(BookEvalData::suggestedOutFile(config.bookFile)); },
            500.0F)) {
        data.updateConfiguration();
    }

    const bool bookExists = !config.bookFile.empty() && std::filesystem::is_regular_file(config.bookFile);
    const bool outGiven = !config.outFile.empty();
    ImGui::Spacing();
    if (!config.bookFile.empty() && !bookExists) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::ERROR_COLOR);
        ImGuiControls::textWrapped("This book does not exist. Choose one that does.");
        ImGui::PopStyleColor();
    }
    if (outGiven && std::filesystem::exists(config.outFile)) {
        ImGui::PushStyleColor(ImGuiCol_Text, StepColors::WARNING_COLOR);
        ImGuiControls::textWrapped("The evaluated book exists: the run continues it, with the positions "
            "that have no score yet. That needs the same book, engine and limit as before.");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginDisabled(!bookExists || !outGiven);
    if (ImGuiControls::textButton("Continue")) {
        data.recentBookFiles().add(config.bookFile);
        data.recentOutFiles().add(config.outFile);
        data.updateConfiguration();
        finished_ = true;
        summary_ = std::format("Book {}, evaluated book {}.", config.bookFile, config.outFile);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        summary_ = "No book selected, the evaluation was not started.";
        return "stop";
    }
    return "";
}

} // namespace QaplaWindows::ChatBot
