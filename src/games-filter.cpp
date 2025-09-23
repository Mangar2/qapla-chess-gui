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

#include "games-filter.h"
#include "imgui-controls.h"
#include "configuration.h"

#include <imgui.h>
#include <algorithm>

using namespace QaplaWindows;

// FullTextFilter implementation
FullTextFilter::FullTextFilter() : filterChanged_(false) {
}

bool FullTextFilter::matches(const GameRecord& game) const {
    if (searchText_.empty()) {
        return true;
    }
    
    // Search in all string fields of the game
    const auto& tags = game.getTags();
    for (const auto& tag : tags) {
        if (tag.second.find(searchText_) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool FullTextFilter::draw() {
    filterChanged_ = false;
    
    if (ImGuiControls::inputText("Full Text Search", searchText_)) {
        filterChanged_ = true;
    }
    
    return filterChanged_;
}

void FullTextFilter::sendOptionsToConfiguration() {
    QaplaHelpers::IniFile::Section section;
	section.name = "gamesfilter";
	section.addEntry("id", "FullText");
	section.addEntry("text", searchText_);
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "gamesfilter", "FullText", { section }
    );
}

// MetaFilter implementation
MetaFilter::MetaFilter() {
    filters_.push_back(std::make_unique<FullTextFilter>());
}

bool MetaFilter::matches(const GameRecord& game) const {
    for (const auto& filter : filters_) {
        if (!filter->matches(game)) {
            return false;
        }
    }
    return true;
}

bool MetaFilter::draw() {
    bool anyChanged = false;
    
    ImGui::Begin("Game Filters");
    
    for (auto& filter : filters_) {
        if (filter->draw()) {
            anyChanged = true;
        }
    }
    
    ImGui::End();
    
    return anyChanged;
}

void MetaFilter::updateOptions(const std::vector<GameRecord>& games) {
    for (auto& filter : filters_) {
        filter->updateOptions(games);
    }
}