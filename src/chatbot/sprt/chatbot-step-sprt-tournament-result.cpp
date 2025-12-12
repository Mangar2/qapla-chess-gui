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

#include "chatbot-step-sprt-tournament-result.h"
#include "../../tournament-result-view.h"
#include "../../sprt-tournament-data.h"
#include "../../os-helpers.h"
#include "../../imgui-controls.h"

#include <sprt-manager.h>

#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <format>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace QaplaWindows::ChatBot {

ChatbotStepSprtTournamentResult::ChatbotStepSprtTournamentResult(std::string title)
    : title_(std::move(title))
{
}

std::string ChatbotStepSprtTournamentResult::draw() {
    if (finished_) {
        return "";
    }

    auto& sprtData = SprtTournamentData::instance();
    
    // Explanatory text for SPRT
    QaplaWindows::ImGuiControls::textWrapped(
        "SPRT (Sequential Probability Ratio Test) compares two engines to determine "
        "if one is significantly stronger. The test continues until a decision is reached "
        "or the maximum number of games is played.");
    
    ImGui::Spacing();
    
    // Explanation of the SPRT bounds display (e.g., "-2.94 < -0.03 < 2.94")
    auto eloLower = sprtData.sprtConfig().eloLower;
    auto eloUpper = sprtData.sprtConfig().eloUpper;
    auto maxGames = sprtData.sprtConfig().maxGames;
    QaplaWindows::ImGuiControls::textWrapped(std::format(
        "The values shown are: [Lower Bound < LLR (Log-Likelihood Ratio) < Upper Bound]\n" 
        "If LLR falls below lowerbound, then the engine is not {} elo stronger (H0 accepted).\n"
        "If LLR exceeds upperbound, then the engine is at least {} elo stronger (H1 accepted).\n"
        "The test continues as long as LLR stays between the bounds and the maximum number of games ({}) is not reached.", 
        eloLower, eloUpper, maxGames));

    ImGui::Spacing();
        
    // Display SPRT test result table
    ImGui::Text("SPRT Test Result:");
    ImVec2 sprtTableSize(0.0F, 100.0F);
    sprtData.drawSprtTable(sprtTableSize);
    
    ImGui::Spacing();
    
    // Display duel result table
    ImGui::Text("Duel Result:");
    ImVec2 resultTableSize(0.0F, 100.0F);
    sprtData.drawResultTable(resultTableSize);
    
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
        "Open a detailed HTML report with full SPRT tournament results in your default browser.");

    ImGui::SameLine();

    if (QaplaWindows::ImGuiControls::textButton("Finish")) {
        finished_ = true;
        return "";
    }

    return "";
}

std::string ChatbotStepSprtTournamentResult::generateHtmlReport() {
    try {
        auto& sprtData = SprtTournamentData::instance();
        const auto& result = sprtData.getTournamentResult();

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
        
        // TODO: Get actual start time from SPRT data if available
        metadata.startTime = metadata.latestUpdate;
        
        // Get system information
        metadata.site = QaplaHelpers::OsHelpers::getHostname();
        metadata.country = QaplaHelpers::OsHelpers::getCountry();
        metadata.hardware = QaplaHelpers::OsHelpers::getHardwareInfo();
        metadata.operatingSystem = QaplaHelpers::OsHelpers::getOperatingSystem();
        
        // Get time control from SPRT settings
        const auto& timeControl = sprtData.getGlobalSettings().getTimeControlSettings();
        metadata.level = std::format("Blitz {}", timeControl.timeControl);
        
        // Set PGN file if applicable
        // TODO: Get actual PGN filename from SPRT data
        metadata.pgnFile = "";
        
        // Set table creator
        metadata.tableCreator = "Qapla Chess GUI";
        
        // Check if SPRT tournament is finished
        metadata.tournamentFinished = sprtData.state() == SprtTournamentData::State::Stopped;

        // Generate HTML with metadata
        std::string html = TournamentResultView::formatHtml(result, title_, true, &metadata);

        // Ensure config directory exists
        std::string configDir = QaplaHelpers::OsHelpers::getConfigDirectory();
        std::filesystem::create_directories(configDir);

        // Always use the same filename (overwrites previous)
        std::string htmlPath = configDir + "/sprt-tournament-result.html";

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
