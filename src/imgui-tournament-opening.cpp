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

#include "imgui-tournament-opening.h"
#include "imgui-controls.h"
#include "configuration.h"

#include "qapla-tester/string-helper.h"

#include <imgui.h>
#include <string>

using namespace QaplaWindows;

bool ImGuiTournamentOpening::draw(float inputWidth, float fileInputWidth, float indent) {
    bool changed = false;

    ImGui::Indent(indent);

    changed |= ImGuiControls::existingFileInput("Opening file", openings_.file, fileInputWidth);
    
    ImGui::SetNextItemWidth(inputWidth);
    changed |= ImGuiControls::selectionBox("File format", openings_.format, { "epd", "raw", "pgn" });
    
    ImGui::SetNextItemWidth(inputWidth);
    changed |= ImGuiControls::selectionBox("Order", openings_.order, { "random", "sequential" });

    auto xPos = ImGui::GetCursorPosX();
    changed |= ImGuiControls::optionalInput<int>(
        "Set plies",
        openings_.plies,
        [&](int& plies) {
            ImGui::SetCursorPosX(xPos + 100);
            ImGui::SetNextItemWidth(inputWidth - 100);
            return ImGuiControls::inputInt<int>("Plies", plies, 0, 1000);
        }
    );
   
    ImGui::SetNextItemWidth(inputWidth);
    changed |= ImGuiControls::inputInt<uint32_t>("First opening", openings_.start, 0, 1000);
    
    ImGui::SetNextItemWidth(inputWidth);
    changed |= ImGuiControls::inputInt<uint32_t>("Random seed", openings_.seed, 0, 1000);
    
    ImGui::SetNextItemWidth(inputWidth);
    changed |= ImGuiControls::selectionBox("Switch policy", openings_.policy, { "default", "encounter", "round" });

    ImGui::Unindent(indent);

    return changed;
}

void ImGuiTournamentOpening::loadConfiguration() {
    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    auto sections = configData.getSectionList("opening", id_);
    if (!sections || sections->empty()) {
        return;
    }

    for (const auto& [key, value] : (*sections)[0].entries) {
        if (key == "file") {
            openings_.file = value;
        }
        else if (key == "format" && (value == "pgn" || value == "epd" || value == "raw")) {
            openings_.format = value;
        }
        else if (key == "order" && (value == "sequential" || value == "random")) {
            openings_.order = value;
        }
        else if (key == "seed") {
            openings_.seed = QaplaHelpers::to_uint32(value).value_or(815);
        }
        else if (key == "plies") {
            openings_.plies = QaplaHelpers::to_int(value);
        }
        else if (key == "start") {
            openings_.start = QaplaHelpers::to_uint32(value).value_or(0);
        }
        else if (key == "policy" && (value == "default" || value == "encounter" || value == "round")) {
            openings_.policy = value;
        }
    }
}

std::vector<QaplaHelpers::IniFile::Section> ImGuiTournamentOpening::getSections() const {
    QaplaHelpers::IniFile::KeyValueMap openingEntries{
        {"id", id_},
        {"file", openings_.file},
        {"format", openings_.format},
        {"order", openings_.order},
        {"seed", std::to_string(openings_.seed)},
        {"start", std::to_string(openings_.start)},
        {"policy", openings_.policy}
    };

    if (openings_.plies) {
        openingEntries.emplace_back("plies", std::to_string(*openings_.plies));
    }

    return {{
        .name = "opening",
        .entries = openingEntries
    }};
}
