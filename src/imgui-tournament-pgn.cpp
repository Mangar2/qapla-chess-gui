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
#include "tutorial.h"

#include <config-file/pgn-config.h>

#include <imgui.h>

using namespace QaplaWindows;

bool ImGuiTournamentPgn::draw(const DrawOptions& options, const Tutorial::TutorialContext& tutorialContext) {
    bool changed = false;

    if (!options.showCollapsingHeader ||
        ImGuiControls::CollapsingHeaderWithDot("Pgn", ImGuiTreeNodeFlags_Selected, tutorialContext.highlight)) {
        ImGui::PushID("pgn");
        ImGui::Indent(options.indent);

        auto inputWidth = options.inputWidth;

        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::newFileInput("Pgn file", pgnOptions_.file, 
            {{"PGN files (*.pgn)", "pgn"}}, options.fileInputWidth);
        ImGuiControls::hooverTooltip(
            "Path to the PGN file where all games will be saved.\n"
            "The file will be created if it doesn't exist"
        );
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("Pgn file");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }

        ImGui::SetNextItemWidth(inputWidth);
        std::string append = pgnOptions_.append ? "Append" : "Overwrite";
        changed |= ImGuiControls::selectionBox("Append mode", append, { "Append", "Overwrite" });
        pgnOptions_.append = (append == "Append");
        ImGuiControls::hooverTooltip(
            "If enabled, new games will be appended to the existing PGN file.\n"
            "If disabled, the file is overwritten at the start of each run"
        );

        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Save only finished games", pgnOptions_.onlyFinishedGames);
        ImGuiControls::hooverTooltip(
            "Save only games that were finished (i.e. not crashed or aborted).\n"
            "If disabled, all games are written regardless of status"
        );

        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Minimal tags", pgnOptions_.minimalTags);
        ImGuiControls::hooverTooltip(
            "If enabled, saves a minimal PGN with only essential headers and moves —\n"
            "omits metadata and annotations"
        );

        /*
        Not yet supported in PGN IO
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Save after each move", pgnOptions_.saveAfterMove);
        */

        if (options.drawDetails) {
            ImGui::SetNextItemWidth(inputWidth);
            changed |= ImGuiControls::booleanInput("Include clock", pgnOptions_.includeClock);
            ImGuiControls::hooverTooltip(
                "Include time spend for the move\n"
                "(if available from engine output)"
            );

            ImGui::SetNextItemWidth(inputWidth);
            changed |= ImGuiControls::booleanInput("Include eval", pgnOptions_.includeEval);
            ImGuiControls::hooverTooltip(
                "Include the engine's evaluation score in PGN comments for each move"
            );

            ImGui::SetNextItemWidth(inputWidth);
            changed |= ImGuiControls::booleanInput("Include PV", pgnOptions_.includePv);
            ImGuiControls::hooverTooltip(
                "Include the full principal variation (PV) in PGN comments.\n"
                "Useful for debugging or engine analysis"
            );

            ImGui::SetNextItemWidth(inputWidth);
            changed |= ImGuiControls::booleanInput("Include depth", pgnOptions_.includeDepth);
            ImGuiControls::hooverTooltip(
                "Include the search depth reached when the move was selected"
            );
        }
        ImGui::Unindent(options.indent);
        ImGui::PopID();
    }

    if (changed) {
        updateConfiguration();
    }

    return changed;
}

void ImGuiTournamentPgn::loadConfiguration() {
    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    auto pgnOptions = QaplaTester::PgnConfig::loadFromConfigData(configData, id_);
    if (pgnOptions) {
        pgnOptions_ = *pgnOptions;
    }
}

std::vector<QaplaHelpers::IniFile::Section> ImGuiTournamentPgn::getSections() const {
    return QaplaTester::PgnConfig::getSections(pgnOptions_, id_);
}

void ImGuiTournamentPgn::updateConfiguration() const {
    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    auto sections = QaplaTester::PgnConfig::getSections(pgnOptions_, id_);
    configData.setSectionList("pgnoutput", id_, sections);
}
