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

#include "chatbot-step-tournament-result.h"
#include "../tournament-result-view.h"
#include "../tournament-data.h"
#include "../sprt-tournament-data.h"
#include "../os-helpers.h"
#include "../imgui-controls.h"

#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <format>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentResult::ChatbotStepTournamentResult(
    EngineSelectContext context,
    std::string title)
    : context_(context)
    , title_(std::move(title))
{
}

QaplaTester::TournamentResult ChatbotStepTournamentResult::getTournamentResult() const {
    switch (context_) {
        case EngineSelectContext::Standard:
            return TournamentData::instance().getTournamentResult();
        case EngineSelectContext::SPRT:
            return SprtTournamentData::instance().getTournamentResult();
        default:
            return TournamentData::instance().getTournamentResult();
    }
}

std::string ChatbotStepTournamentResult::draw() {
    if (finished_) {
        return "";
    }

    // Show text summary
    std::string summary = formatTextSummary();
    QaplaWindows::ImGuiControls::textWrapped(summary.c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Button to view detailed HTML results
    if (QaplaWindows::ImGuiControls::textButton("View Detailed Results (HTML)")) {
        if (!htmlGenerated_) {
            htmlPath_ = generateHtmlReport();
            htmlGenerated_ = true;
        }
        if (!htmlPath_.empty()) {
            QaplaHelpers::OsHelpers::openInShell(htmlPath_);
        }
    }
    QaplaWindows::ImGuiControls::hooverTooltip(
        "Open a detailed HTML report with full tournament results table in your default browser.");

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Continue")) {
        finished_ = true;
    }
    QaplaWindows::ImGuiControls::hooverTooltip("Continue to the next step.");

    return "";
}

std::string ChatbotStepTournamentResult::generateHtmlReport() {
    try {
        // Get tournament result
        const auto& result = getTournamentResult();

        // Generate HTML
        std::string html = TournamentResultView::formatHtml(result, title_, true);

        // Ensure config directory exists
        std::string configDir = QaplaHelpers::OsHelpers::getConfigDirectory();
        std::filesystem::create_directories(configDir);

        // Always use the same filename (overwrites previous)
        std::string htmlPath = configDir + "/tournament-result.html";

        // Write to file
        std::ofstream file(htmlPath);
        if (!file) {
            return "";
        }
        file << html;
        file.close();

        return htmlPath;
    }
    catch (...) {
        return "";
    }
}

std::string ChatbotStepTournamentResult::formatTextSummary() const {
    try {
        const auto& result = getTournamentResult();
        
        // Get top 3 engines from the result
        auto scoredEngines = result.scoredEngines();
        
        if (scoredEngines.empty()) {
            return "No tournament results available yet.";
        }

        // Count total games
        int totalGames = 0;
        for (const auto& scored : scoredEngines) {
            totalGames += static_cast<int>(scored.total);
        }
        totalGames /= 2; // Each game is counted twice

        std::string summary = std::format("Tournament Results ({})\n\n", title_);
        summary += std::format("Total games played: {}\n", totalGames);
        summary += std::format("Engines: {}\n\n", scoredEngines.size());
        summary += "Top 3:\n";

        int rank = 1;
        for (size_t idx = 0; idx < std::min<size_t>(3, scoredEngines.size()); ++idx) {
            const auto& scored = scoredEngines[idx];
            summary += std::format("{}. {} - {:.1f}% ({:.0f} Elo ±{})\n",
                rank++,
                scored.engineName,
                scored.score * 100.0,
                scored.elo,
                static_cast<int>(scored.error));
        }

        return summary;
    }
    catch (...) {
        return "Error retrieving tournament results.";
    }
}

} // namespace QaplaWindows::ChatBot
