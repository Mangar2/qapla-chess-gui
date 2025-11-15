/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "game-filter-window.h"
#include <imgui.h>
#include <algorithm>
#include <ranges>

namespace QaplaWindows {

GameFilterWindow::GameFilterWindow() = default;

void GameFilterWindow::init(const std::string& configId) {
    filterData_.init(configId);
}

void GameFilterWindow::draw() {
    // Get available window size
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    
    // Calculate child window size with 10px margins on left and right
    const float horizontalMargin = 10.0f;
    ImVec2 childSize(availableSize.x - 2 * horizontalMargin, availableSize.y);
    
    // Position cursor with left margin
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + horizontalMargin);
    
    // Create scrollable child window
    ImGui::BeginChild("FilterContent", childSize, false, ImGuiWindowFlags_None);
    
    ImGui::Spacing();
    bool modified = false;
    modified |= drawActiveToggle();
    ImGui::Separator();

    // Only show filter controls if active
    if (filterData_.isActive()) {
        modified |= drawPlayerSelection();
        ImGui::Separator();
        
        modified |= drawOpponentSelection();
        ImGui::Separator();
        
        modified |= drawResultSelection();
        ImGui::Separator();
        
        modified |= drawTerminationSelection();
    }
    
    ImGui::EndChild();
    
    // Central onFilterChanged callback
    if (modified && onFilterChanged_) {
        onFilterChanged_();
    }
}

bool GameFilterWindow::drawActiveToggle() {
    bool active = filterData_.isActive();
    if (ImGui::Checkbox("Enable Filter", &active)) {
        filterData_.setActive(active);
        return true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enable or disable the filter without losing your selections");
    }
    return false;
}

bool GameFilterWindow::drawPlayerSelection() {
    ImGui::PushID("Players");
    ImGui::Text("Filter by Player:");
    
    bool modified = drawNameSelection(
        "Select players (any color)",
        availablePlayers_,
        [this](const std::string& name) { return filterData_.isPlayerSelected(name); },
        [this](const std::string& name) { filterData_.togglePlayer(name); },
        [this]() { filterData_.setSelectedPlayers({}); }
    );
    ImGui::PopID();
    return modified;
}

bool GameFilterWindow::drawOpponentSelection() {
    ImGui::PushID("Opponents");
    ImGui::Text("Filter by Opponent:");

    bool modified = drawNameSelection(
        "Select opponents (any color)",
        availableOpponents_,
        [this](const std::string& name) { return filterData_.isOpponentSelected(name); },
        [this](const std::string& name) { filterData_.toggleOpponent(name); },
        [this]() { filterData_.setSelectedOpponents({}); }
    );
    ImGui::PopID();
    return modified;
}

bool GameFilterWindow::drawNameSelection(const std::string& tooltip,
                                         const std::vector<std::string>& availableNames,
                                         std::function<bool(const std::string&)> isSelected,
                                         std::function<void(const std::string&)> onToggle,
                                         std::function<void()> onClear) {
    ImGui::SameLine();
    bool modified = false;
    if (ImGui::SmallButton("Clear")) {
        onClear();
        modified = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip.c_str());
    }
    
    for (const auto& name : availableNames) {
        bool selected = isSelected(name);
        if (ImGui::Selectable(name.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
            onToggle(name);
            modified = true;
        }
    }
    return modified;
}

bool GameFilterWindow::drawResultSelection() {
    bool modified = false;
    ImGui::Text("Filter by Game Result:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear##Results")) {
        filterData_.setSelectedResults({});
        modified = true;
    }
    
    for (const auto& result : availableResults_) {
        bool selected = filterData_.isResultSelected(result);
        std::string resultStr = QaplaTester::gameResultToPgnResult(result);
        
        if (ImGui::Selectable(resultStr.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
            filterData_.toggleResult(result);
            modified = true;
        }
    }
    
    // Show count
    auto selectedCount = filterData_.getSelectedResults().size();
    if (selectedCount > 0) {
        ImGui::Text("Selected: %zu result(s)", selectedCount);
    }
    return modified;
}

bool GameFilterWindow::drawTerminationSelection() {
    bool modified = false;
    ImGui::Text("Filter by Termination:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear##Terminations")) {
        filterData_.setSelectedTerminations({});
        modified = true;
    }
    
    // Sort terminations for better UI
    std::vector<QaplaTester::GameEndCause> sortedTerminations(
        availableTerminations_.begin(), 
        availableTerminations_.end()
    );
    std::sort(sortedTerminations.begin(), sortedTerminations.end(),
        [](QaplaTester::GameEndCause a, QaplaTester::GameEndCause b) {
            return QaplaTester::gameEndCauseToPgnTermination(a) < 
                   QaplaTester::gameEndCauseToPgnTermination(b);
        });
    
    for (const auto& termination : sortedTerminations) {
        bool selected = filterData_.isTerminationSelected(termination);
        std::string terminationStr = QaplaTester::gameEndCauseToPgnTermination(termination);
        
        if (ImGui::Selectable(terminationStr.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
            filterData_.toggleTermination(termination);
            modified = true;
        }
    }
    
    // Show count
    auto selectedCount = filterData_.getSelectedTerminations().size();
    if (selectedCount > 0) {
        ImGui::Text("Selected: %zu termination(s)", selectedCount);
    }
    return modified;
}

void GameFilterWindow::updateFilterOptions(const std::vector<QaplaTester::GameRecord>& games) {
    if (games.empty()) {
        return;
    }

    // Extract unique player names (both White and Black)
    std::set<std::string> uniquePlayers;
    std::set<QaplaTester::GameResult> uniqueResults;
    std::set<QaplaTester::GameEndCause> uniqueTerminations;

    for (const auto& game : games) {
        const auto& tags = game.getTags();
        
        // Extract both White and Black player names
        auto whiteIt = tags.find("White");
        if (whiteIt != tags.end() && !whiteIt->second.empty()) {
            uniquePlayers.insert(whiteIt->second);
        }
        
        auto blackIt = tags.find("Black");
        if (blackIt != tags.end() && !blackIt->second.empty()) {
            uniquePlayers.insert(blackIt->second);
        }
        
        // Extract game result
        auto [cause, result] = game.getGameResult();
        uniqueResults.insert(result);
        
        // Extract termination cause (only if game ended)
        if (cause != QaplaTester::GameEndCause::Ongoing) {
            uniqueTerminations.insert(cause);
        }
    }

    // Convert sets to vectors for player lists
    std::vector<std::string> playerVec(uniquePlayers.begin(), uniquePlayers.end());
    
    // Sort alphabetically
    std::ranges::sort(playerVec);

    // Update filter options (same list for both players and opponents)
    availablePlayers_ = playerVec;
    availableOpponents_ = playerVec;
    availableResults_ = uniqueResults;
    availableTerminations_ = uniqueTerminations;
}

void GameFilterWindow::updateConfiguration(const std::string& configId) const {
    filterData_.updateConfiguration(configId);
}

} // namespace QaplaWindows
