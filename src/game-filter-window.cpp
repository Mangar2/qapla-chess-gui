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

#include <format>
#include "ui/qapla-ui.h"
#include <algorithm>

namespace QaplaWindows {

GameFilterWindow::GameFilterWindow() = default;

void GameFilterWindow::init(const std::string& configId) {
    filterData_.init(configId);
}

void GameFilterWindow::draw() {
    // Get available window size
    QaplaUi::Vec2 availableSize = QaplaUi::getContentRegionAvail();
    
    // Calculate child window size with 10px margins on left and right
    const float horizontalMargin = 10.0f;
    QaplaUi::Vec2 childSize(availableSize.x - 2 * horizontalMargin, availableSize.y);
    
    // Position cursor with left margin
    QaplaUi::setCursorPosX(QaplaUi::getCursorPosX() + horizontalMargin);
    
    // Create scrollable child window
    QaplaUi::beginChild("FilterContent", childSize, QaplaUi::ChildFlags::None, QaplaUi::WindowFlags::None);
    
    QaplaUi::spacing();
    bool modified = false;
    modified |= drawActiveToggle();
    QaplaUi::separator();

    // Only show filter controls if active
    if (filterData_.isActive()) {
        modified |= drawPlayerSelection();
        QaplaUi::separator();
        
        modified |= drawOpponentSelection();
        QaplaUi::separator();
        
        modified |= drawTopicSelection("results", "Filter by Game Result:", "Select game results");
        QaplaUi::separator();
        
        modified |= drawTopicSelection("causes", "Filter by Game Cause:", "Select game termination causes");
        QaplaUi::separator();
        
        modified |= drawTopicSelection("terminations", "Filter by Termination:", "Select termination types from PGN Termination tag");
    }
    
    QaplaUi::endChild();
    
    // Central onFilterChanged callback
    if (modified && onFilterChanged_) {
        onFilterChanged_();
    }
}

bool GameFilterWindow::drawSectionHeader(const std::string& title, size_t selectedCount, const std::string& tooltip) {
    bool cleared = false;
    
    // Show title
    QaplaUi::text(title);
    
    // Show selected count
    if (selectedCount > 0) {
        QaplaUi::sameLine();
        QaplaUi::textDisabled(std::format("({} selected)", selectedCount));
    }
    
    // Show clear button only if items are selected
    if (selectedCount > 0) {
        QaplaUi::sameLine();
        std::string buttonLabel = "Clear##" + title;
        if (QaplaUi::smallButton(buttonLabel.c_str())) {
            cleared = true;
        }
    }
    
    // Show tooltip if provided
    if (!tooltip.empty()) {
        QaplaUi::sameLine();
        QaplaUi::textDisabled("(?)");
        QaplaUi::itemTooltip(tooltip);
    }
    
    return cleared;
}

bool GameFilterWindow::drawActiveToggle() {
    bool active = filterData_.isActive();
    if (QaplaUi::checkbox("Enable Filter", active)) {
        filterData_.setActive(active);
        return true;
    }
    QaplaUi::sameLine();
    QaplaUi::textDisabled("(?)");
    QaplaUi::itemTooltip("Enable or disable the filter without losing your selections");
    return false;
}

bool GameFilterWindow::drawPlayerSelection() {
    QaplaUi::pushId("Players");
    
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
    
    QaplaUi::popId();
    return modified;
}

bool GameFilterWindow::drawOpponentSelection() {
    QaplaUi::pushId("Opponents");
    
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
    
    QaplaUi::popId();
    return modified;
}

bool GameFilterWindow::drawNameSelection(const std::vector<std::string>& availableNames,
                                         std::function<bool(const std::string&)> isSelected,
                                         std::function<void(const std::string&)> onToggle) {
    bool modified = false;
    
    for (const auto& name : availableNames) {
        bool selected = isSelected(name);
        if (QaplaUi::selectable(name.c_str(), selected, QaplaUi::SelectableFlags::DontClosePopups)) {
            onToggle(name);
            modified = true;
        }
    }
    return modified;
}

bool GameFilterWindow::drawTopicSelection(const std::string& topic, const std::string& title, const std::string& tooltip) {
    const auto& availableOptions = filterData_.getAvailableOptions(topic);
    
    // Don't show topics with less than 2 options
    if (availableOptions.size() < 2) {
        return false;
    }
    
    QaplaUi::pushId(topic.c_str());
    
    size_t selectedCount = filterData_.getSelectedOptions(topic).size();
    bool modified = drawSectionHeader(title, selectedCount, tooltip);
    
    if (modified) {
        filterData_.setSelectedOptions(topic, {});
    }
    
    for (const auto& option : availableOptions) {
        bool selected = filterData_.isOptionSelected(topic, option);
        
        if (QaplaUi::selectable(option.c_str(), selected, QaplaUi::SelectableFlags::DontClosePopups)) {
            filterData_.toggleOption(topic, option);
            modified = true;
        }
    }
    
    QaplaUi::popId();
    return modified;
}

void GameFilterWindow::updateFilterOptions(const std::vector<QaplaTester::GameRecord>& games) {
    filterData_.updateAvailableOptions(games);
}

void GameFilterWindow::updateConfiguration(const std::string& configId) const {
    filterData_.updateConfiguration(configId);
}

} // namespace QaplaWindows
