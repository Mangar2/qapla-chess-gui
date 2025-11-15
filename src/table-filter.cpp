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

#include "table-filter.h"
#include "imgui-controls.h"
#include "configuration.h"

#include <imgui.h>
#include <algorithm>

using namespace QaplaWindows;

// FullTextFilter implementation
FullTextFilter::FullTextFilter() : filterChanged_(false) {
}

bool FullTextFilter::matches(const Row& row) const {
    if (searchText_.empty()) {
        return true;
    }
    
    for (const auto& cell : row) {
        if (cell.find(searchText_) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool FullTextFilter::draw() {
    filterChanged_ = false;
    
    ImGui::PushID("FullTextFilter");
    if (ImGuiControls::inputText("Search", searchText_)) {
        filterChanged_ = true;
    }
    ImGui::PopID();
    
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

bool MetaFilter::matches(const Row& row) const {
    for (const auto& filter : filters_) {
        if (!filter->matches(row)) {
            return false;
        }
    }
    return true;
}

bool MetaFilter::draw() {
    bool anyChanged = false;
    
    for (auto& filter : filters_) {
        if (filter->draw()) {
            anyChanged = true;
        }
    }
    
    return anyChanged;
}

void MetaFilter::updateOptions(const ITableFilter::Table& table) {
    for (auto& filter : filters_) {
        filter->updateOptions(table);
    }
}
