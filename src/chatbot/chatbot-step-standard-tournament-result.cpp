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

#include "chatbot-step-standard-tournament-result.h"
#include "../tournament-result-view.h"
#include "../tournament-data.h"
#include "../os-helpers.h"
#include "../imgui-controls.h"

#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <format>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace QaplaWindows::ChatBot {

ChatbotStepStandardTournamentResult::ChatbotStepStandardTournamentResult(std::string title)
    : title_(std::move(title))
{
}

std::string ChatbotStepStandardTournamentResult::draw() {
    if (finished_) {
        return "";
    }

    auto& tournamentData = TournamentData::instance();
    
    // Display tournament progress
    uint32_t totalGames = tournamentData.getTotalGames();
    uint32_t playedGames = tournamentData.getPlayedGames();
    
    ImGui::Text("Tournament Progress: %u / %u games completed", playedGames, totalGames);
    ImGui::Spacing();
    
    // Display the ELO rating table (same as in tournament window)
    ImVec2 tableSize(0.0F, 300.0F);
    tournamentData.drawEloTable(tableSize);
    
    ImGui::Spacing();
    
    // Display the results matrix table
    tournamentData.drawMatrixTable(tableSize);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Button to view detailed HTML results
    if (QaplaWindows::ImGuiControls::textButton("View Detailed Results (HTML)")) {
        htmlPath_ = generateHtmlReport();
        if (!htmlPath_.empty()) {
            QaplaHelpers::OsHelpers::openInShell(htmlPath_);
        }
    }
    QaplaWindows::ImGuiControls::hooverTooltip(
        "Open a detailed HTML report with full tournament results table in your default browser.");

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Finish")) {
        finished_ = true;
        return "";
    }

    return "";
}

std::string ChatbotStepStandardTournamentResult::generateHtmlReport() {
    try {
        auto& tournamentData = TournamentData::instance();
        const auto& result = tournamentData.getTournamentResult();

        // Build metadata
        TournamentResultView::TournamentMetadata metadata;
        
        // Get current time for latest update
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        std::tm tmNow = {};
#ifdef _WIN32
        localtime_s(&tmNow, &timeT);
#else
        localtime_r(&timeT, &tmNow);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tmNow, "%Y.%m.%d, %H:%M:%S");
        metadata.latestUpdate = oss.str();
        
        // TODO: Get actual start time from tournament data if available
        metadata.startTime = metadata.latestUpdate;
        
        // Get system information
        metadata.site = QaplaHelpers::OsHelpers::getHostname();
        metadata.country = QaplaHelpers::OsHelpers::getCountry();
        metadata.hardware = QaplaHelpers::OsHelpers::getHardwareInfo();
        metadata.operatingSystem = QaplaHelpers::OsHelpers::getOperatingSystem();
        
        // Get time control from tournament settings
        const auto& timeControl = tournamentData.getGlobalSettings().getTimeControlSettings();
        metadata.level = std::format("Blitz {}", timeControl.timeControl);
        
        // Set PGN file if applicable
        // TODO: Get actual PGN filename from tournament data
        metadata.pgnFile = "";
        
        // Set table creator
        metadata.tableCreator = "Qapla Chess GUI";
        
        // Check if tournament is finished
        metadata.tournamentFinished = tournamentData.getState() == TournamentData::State::Stopped;

        // Generate HTML with metadata
        std::string html = TournamentResultView::formatHtml(result, title_, true, &metadata);

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

} // namespace QaplaWindows::ChatBot
