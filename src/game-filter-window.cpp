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

namespace QaplaWindows {

GameFilterWindow::GameFilterWindow() = default;

void GameFilterWindow::draw() {
    if (!filterData_) return;

    drawActiveToggle();
    ImGui::Separator();

    // Only show filter controls if active
    if (filterData_->isActive()) {
        drawPlayerSelection();
        ImGui::Separator();
        
        drawOpponentSelection();
        ImGui::Separator();
        
        drawResultSelection();
        ImGui::Separator();
        
        drawTerminationSelection();
    }
}

void GameFilterWindow::drawActiveToggle() {
    bool active = filterData_->isActive();
    if (ImGui::Checkbox("Enable Filter", &active)) {
        filterData_->setActive(active);
        if (onFilterChanged_) {
            onFilterChanged_();
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enable or disable the filter without losing your selections");
    }
}

void GameFilterWindow::drawPlayerSelection() {
    ImGui::Text("Filter by Player:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear##Players")) {
        filterData_->setSelectedPlayers({});
        if (onFilterChanged_) {
            onFilterChanged_();
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Select players (any color)");
    }
    
    // Dynamic height based on number of items (min 2, max 10 visible items)
    const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
    const size_t visibleItems = std::min(std::max(availablePlayers_.size(), size_t(2)), size_t(10));
    const float listHeight = itemHeight * visibleItems + 4.0f; // +4 for border
    
    ImGui::BeginChild("PlayerList", ImVec2(0, listHeight), true, ImGuiWindowFlags_NoScrollbar);
    
    for (const auto& player : availablePlayers_) {
        bool selected = filterData_->isPlayerSelected(player);
        if (ImGui::Selectable(player.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
            filterData_->togglePlayer(player);
            if (onFilterChanged_) {
                onFilterChanged_();
            }
        }
    }
    
    ImGui::EndChild();
    
    // Show count
    auto selectedCount = filterData_->getSelectedPlayers().size();
    if (selectedCount > 0) {
        ImGui::Text("Selected: %zu player(s)", selectedCount);
    }
}

void GameFilterWindow::drawOpponentSelection() {
    ImGui::Text("Filter by Opponent:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear##Opponents")) {
        filterData_->setSelectedOpponents({});
        if (onFilterChanged_) {
            onFilterChanged_();
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Select opponents (any color)");
    }
    
    // Dynamic height based on number of items (min 2, max 10 visible items)
    const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
    const size_t visibleItems = std::min(std::max(availableOpponents_.size(), size_t(2)), size_t(10));
    const float listHeight = itemHeight * visibleItems + 4.0f; // +4 for border
    
    ImGui::BeginChild("OpponentList", ImVec2(0, listHeight), true, ImGuiWindowFlags_NoScrollbar);
    
    for (const auto& opponent : availableOpponents_) {
        bool selected = filterData_->isOpponentSelected(opponent);
        if (ImGui::Selectable(opponent.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
            filterData_->toggleOpponent(opponent);
            if (onFilterChanged_) {
                onFilterChanged_();
            }
        }
    }
    
    ImGui::EndChild();
    
    // Show count
    auto selectedCount = filterData_->getSelectedOpponents().size();
    if (selectedCount > 0) {
        ImGui::Text("Selected: %zu opponent(s)", selectedCount);
    }
}

void GameFilterWindow::drawResultSelection() {
    ImGui::Text("Filter by Game Result:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear##Results")) {
        filterData_->setSelectedResults({});
        if (onFilterChanged_) {
            onFilterChanged_();
        }
    }
    
    // Dynamic height based on number of items (usually 4: 1-0, 0-1, 1/2-1/2, *)
    const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
    const size_t visibleItems = std::min(std::max(availableResults_.size(), size_t(2)), size_t(6));
    const float listHeight = itemHeight * visibleItems + 4.0f; // +4 for border
    
    ImGui::BeginChild("ResultList", ImVec2(0, listHeight), true, ImGuiWindowFlags_NoScrollbar);
    
    for (const auto& result : availableResults_) {
        bool selected = filterData_->isResultSelected(result);
        std::string resultStr = QaplaTester::gameResultToPgnResult(result);
        
        if (ImGui::Selectable(resultStr.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
            filterData_->toggleResult(result);
            if (onFilterChanged_) {
                onFilterChanged_();
            }
        }
    }
    
    ImGui::EndChild();
    
    // Show count
    auto selectedCount = filterData_->getSelectedResults().size();
    if (selectedCount > 0) {
        ImGui::Text("Selected: %zu result(s)", selectedCount);
    }
}

void GameFilterWindow::drawTerminationSelection() {
    ImGui::Text("Filter by Termination:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear##Terminations")) {
        filterData_->setSelectedTerminations({});
        if (onFilterChanged_) {
            onFilterChanged_();
        }
    }
    
    // Dynamic height based on number of items (min 2, max 10 visible items)
    const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
    const size_t visibleItems = std::min(std::max(availableTerminations_.size(), size_t(2)), size_t(10));
    const float listHeight = itemHeight * visibleItems + 4.0f; // +4 for border
    
    ImGui::BeginChild("TerminationList", ImVec2(0, listHeight), true, ImGuiWindowFlags_NoScrollbar);
    
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
        bool selected = filterData_->isTerminationSelected(termination);
        std::string terminationStr = QaplaTester::gameEndCauseToPgnTermination(termination);
        
        if (ImGui::Selectable(terminationStr.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
            filterData_->toggleTermination(termination);
            if (onFilterChanged_) {
                onFilterChanged_();
            }
        }
    }
    
    ImGui::EndChild();
    
    // Show count
    auto selectedCount = filterData_->getSelectedTerminations().size();
    if (selectedCount > 0) {
        ImGui::Text("Selected: %zu termination(s)", selectedCount);
    }
}

} // namespace QaplaWindows
