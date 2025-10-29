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

#include "imgui-tournament-pgn.h"
#include "imgui-controls.h"
#include "configuration.h"

#include <imgui.h>

using namespace QaplaWindows;

bool ImGuiTournamentPgn::draw(float inputWidth, float fileInputWidth, float indent) {
    bool changed = false;

    if (ImGui::CollapsingHeader("Pgn", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("pgn");
        ImGui::Indent(indent);

        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::newFileInput("Pgn file", pgnOptions_.file, 
            {{"PGN files (*.pgn)", "pgn"}}, fileInputWidth);

        ImGui::SetNextItemWidth(inputWidth);
        std::string append = pgnOptions_.append ? "Append" : "Overwrite";
        changed |= ImGuiControls::selectionBox("Append mode", append, { "Append", "Overwrite" });
        pgnOptions_.append = (append == "Append");

        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Save only finished games", pgnOptions_.onlyFinishedGames);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Minimal tags", pgnOptions_.minimalTags);
        /*
        Not yet supported in PGN IO
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Save after each move", pgnOptions_.saveAfterMove);
        */
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Include clock", pgnOptions_.includeClock);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Include eval", pgnOptions_.includeEval);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Include PV", pgnOptions_.includePv);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Include depth", pgnOptions_.includeDepth);

        ImGui::Unindent(indent);
        ImGui::PopID();
    }

    return changed;
}

void ImGuiTournamentPgn::loadConfiguration() {
    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    auto sections = configData.getSectionList("pgnoutput", id_);
    if (!sections || sections->empty()) {
        return;
    }

    for (const auto& [key, value] : (*sections)[0].entries) {
        if (key == "file") {
            pgnOptions_.file = value;
        }
        else if (key == "append") {
            pgnOptions_.append = (value == "true");
        }
        else if (key == "onlyFinishedGames") {
            pgnOptions_.onlyFinishedGames = (value == "true");
        }
        else if (key == "minimalTags") {
            pgnOptions_.minimalTags = (value == "true");
        }
        else if (key == "saveAfterMove") {
            pgnOptions_.saveAfterMove = (value == "true");
        }
        else if (key == "includeClock") {
            pgnOptions_.includeClock = (value == "true");
        }
        else if (key == "includeEval") {
            pgnOptions_.includeEval = (value == "true");
        }
        else if (key == "includePv") {
            pgnOptions_.includePv = (value == "true");
        }
        else if (key == "includeDepth") {
            pgnOptions_.includeDepth = (value == "true");
        }
    }
}

std::vector<QaplaHelpers::IniFile::Section> ImGuiTournamentPgn::getSections() const {
    QaplaHelpers::IniFile::KeyValueMap pgnEntries{
        {"id", id_},
        {"file", pgnOptions_.file},
        {"append", pgnOptions_.append ? "true" : "false"},
        {"onlyFinishedGames", pgnOptions_.onlyFinishedGames ? "true" : "false"},
        {"minimalTags", pgnOptions_.minimalTags ? "true" : "false"},
        {"saveAfterMove", pgnOptions_.saveAfterMove ? "true" : "false"},
        {"includeClock", pgnOptions_.includeClock ? "true" : "false"},
        {"includeEval", pgnOptions_.includeEval ? "true" : "false"},
        {"includePv", pgnOptions_.includePv ? "true" : "false"},
        {"includeDepth", pgnOptions_.includeDepth ? "true" : "false"}
    };

    return {{
        .name = "pgnoutput",
        .entries = pgnEntries
    }};
}
