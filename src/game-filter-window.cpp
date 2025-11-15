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

bool GameFilterWindow::drawSectionHeader(const std::string& title, size_t selectedCount, const std::string& tooltip) {
    bool cleared = false;
    
    // Show title
    ImGui::Text("%s", title.c_str());
    
    // Show selected count
    if (selectedCount > 0) {
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu selected)", selectedCount);
    }
    
    // Show clear button only if items are selected
    if (selectedCount > 0) {
        ImGui::SameLine();
        std::string buttonLabel = "Clear##" + title;
        if (ImGui::SmallButton(buttonLabel.c_str())) {
            cleared = true;
        }
    }
    
    // Show tooltip if provided
    if (!tooltip.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip.c_str());
        }
    }
    
    return cleared;
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
    
    size_t selectedCount = filterData_.getSelectedPlayers().size();
    bool modified = drawSectionHeader("Filter by Player:", selectedCount, "Select players (any color)");
    
    if (modified) {
        filterData_.setSelectedPlayers({});
    }
    
    modified |= drawNameSelection(
        filterData_.getAvailableNames(),
        [this](const std::string& name) { return filterData_.isPlayerSelected(name); },
        [this](const std::string& name) { filterData_.togglePlayer(name); }
    );
    
    ImGui::PopID();
    return modified;
}

bool GameFilterWindow::drawOpponentSelection() {
    ImGui::PushID("Opponents");
    
    size_t selectedCount = filterData_.getSelectedOpponents().size();
    bool modified = drawSectionHeader("Filter by Opponent:", selectedCount, "Select opponents (any color)");
    
    if (modified) {
        filterData_.setSelectedOpponents({});
    }

    modified |= drawNameSelection(
        filterData_.getAvailableNames(),
        [this](const std::string& name) { return filterData_.isOpponentSelected(name); },
        [this](const std::string& name) { filterData_.toggleOpponent(name); }
    );
    
    ImGui::PopID();
    return modified;
}

bool GameFilterWindow::drawNameSelection(const std::vector<std::string>& availableNames,
                                         std::function<bool(const std::string&)> isSelected,
                                         std::function<void(const std::string&)> onToggle) {
    bool modified = false;
    
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
    size_t selectedCount = filterData_.getSelectedResults().size();
    bool modified = drawSectionHeader("Filter by Game Result:", selectedCount, "Select game results");
    
    if (modified) {
        filterData_.setSelectedResults({});
    }
    
    for (const auto& result : filterData_.getAvailableResults()) {
        bool selected = filterData_.isResultSelected(result);
        std::string resultStr = QaplaTester::gameResultToPgnResult(result);
        
        if (ImGui::Selectable(resultStr.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
            filterData_.toggleResult(result);
            modified = true;
        }
    }
    
    return modified;
}

bool GameFilterWindow::drawTerminationSelection() {
    size_t selectedCount = filterData_.getSelectedTerminations().size();
    bool modified = drawSectionHeader("Filter by Termination:", selectedCount, "Select termination types from PGN Termination tag");
    
    if (modified) {
        filterData_.setSelectedTerminations({});
    }
    
    // Sort terminations alphabetically
    std::vector<std::string> sortedTerminations = filterData_.getAvailableTerminations();
    std::sort(sortedTerminations.begin(), sortedTerminations.end());
    
    for (const auto& termination : sortedTerminations) {
        bool selected = filterData_.isTerminationSelected(termination);
        
        if (ImGui::Selectable(termination.c_str(), selected, ImGuiSelectableFlags_DontClosePopups)) {
            filterData_.toggleTermination(termination);
            modified = true;
        }
    }
    
    return modified;
}

void GameFilterWindow::updateFilterOptions(const std::vector<QaplaTester::GameRecord>& games) {
    filterData_.updateAvailableOptions(games);
}

void GameFilterWindow::updateConfiguration(const std::string& configId) const {
    filterData_.updateConfiguration(configId);
}

} // namespace QaplaWindows
