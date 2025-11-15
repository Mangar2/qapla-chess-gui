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

#include "imgui-game-filter.h"
#include <algorithm>
#include <ranges>

namespace QaplaWindows {

ImGuiGameFilter::ImGuiGameFilter()
    : filterPopup_(
        ImGuiPopup<GameFilterWindow>::Config{
            .title = "Filter Games",
            .okButton = true,
            .cancelButton = true
        },
        ImVec2(550, 700)  // Size for filter popup
    )
{
}

void ImGuiGameFilter::init(const std::string& configId) {
    configId_ = configId;
    filterData_.init(configId);
    filterPopup_.content().setFilterData(&filterData_);
}

void ImGuiGameFilter::draw() {
    filterPopup_.draw();
}

void ImGuiGameFilter::open() {
    filterPopup_.open();
}

std::optional<bool> ImGuiGameFilter::confirmed() {
    return filterPopup_.confirmed();
}

void ImGuiGameFilter::resetConfirmation() {
    filterPopup_.resetConfirmation();
}

void ImGuiGameFilter::updateFilterOptions(const std::vector<QaplaTester::GameRecord>& games) {
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

    // Update filter window options (same list for both players and opponents)
    filterPopup_.content().setAvailablePlayers(playerVec);
    filterPopup_.content().setAvailableOpponents(playerVec);
    filterPopup_.content().setAvailableResults(uniqueResults);
    filterPopup_.content().setAvailableTerminations(uniqueTerminations);
}

void ImGuiGameFilter::updateConfiguration(const std::string& configId) const {
    filterData_.updateConfiguration(configId);
}

bool ImGuiGameFilter::passesFilter(const QaplaTester::GameRecord& game) const {
    return filterData_.passesFilter(game);
}

void ImGuiGameFilter::setOnFilterChangedCallback(std::function<void()> callback) {
    filterPopup_.content().setOnFilterChangedCallback(std::move(callback));
}

} // namespace QaplaWindows
