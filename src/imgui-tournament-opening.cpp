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
#include "tutorial.h"

#include <config-file/opening-config.h>
#include <base-elements/string-helper.h>

#include <imgui.h>
#include <string>

using namespace QaplaWindows;

bool ImGuiTournamentOpening::draw(const DrawParams& params, 
    const Tutorial::TutorialContext& tutorialContext) {
    bool changed = false;

    ImGuiTreeNodeFlags headerFlags = params.alwaysOpen ? 
        ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_Leaf: ImGuiTreeNodeFlags_Selected;
        
    if (!ImGuiControls::CollapsingHeaderWithDot("Opening", headerFlags, tutorialContext.highlight)) {
        return false;
    }
    
    ImGui::PushID("opening");
    ImGui::Indent(params.indent);

    if (params.showOpeningFile) {
        changed |= drawOpeningFile(params.fileInputWidth, tutorialContext);
    }
    
    if (params.showOrder) {
        changed |= drawOrder(params.inputWidth, tutorialContext);
    }

    if (params.showPlies) {
        changed |= drawPlies(params.inputWidth, tutorialContext);
    }
   
    if (params.showFirstOpening) {
        changed |= drawFirstOpening(params.inputWidth, tutorialContext);
    }
    
    if (params.showRandomSeed) {
        changed |= drawRandomSeed(params.inputWidth, tutorialContext);
    }
    
    if (params.showSwitchPolicy) {
        changed |= drawSwitchPolicy(params.inputWidth, tutorialContext);
    }

    ImGui::Unindent(params.indent);
    ImGui::PopID();

    if (changed) {
        updateConfiguration();
    }

    return changed;
}

bool ImGuiTournamentOpening::drawOpeningFile(float fileInputWidth, 
    const Tutorial::TutorialContext& tutorialContext) {
    bool changed = ImGuiControls::existingFileInput("Opening file", openings_.file, fileInputWidth);
    ImGuiControls::hooverTooltip("Path to opening file (.epd, .pgn, or raw FEN text)");
    
    auto it = tutorialContext.annotations.find("Opening file");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }
    
    return changed;
}

bool ImGuiTournamentOpening::drawOrder(float inputWidth, 
    const Tutorial::TutorialContext& tutorialContext) {
    ImGui::SetNextItemWidth(inputWidth);
    bool changed = ImGuiControls::selectionBox("Order", openings_.order, { "random", "sequential" });
    ImGuiControls::hooverTooltip(
        "How positions are picked from the file:\n"
        "  sequential - Use positions in order\n"
        "  random - Shuffle the order"
    );

    auto it = tutorialContext.annotations.find("Order");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }

    return changed;
}

bool ImGuiTournamentOpening::drawPlies(float inputWidth, 
    const Tutorial::TutorialContext& tutorialContext) {
    auto xPos = ImGui::GetCursorPosX();
    bool changed = ImGuiControls::optionalInput<int>(
        "Set plies",
        openings_.plies,
        [&](int& plies) {
            ImGui::SetCursorPosX(xPos + 100);
            ImGui::SetNextItemWidth(inputWidth - 100);
            return ImGuiControls::inputInt<int>("Plies", plies, 0, 1000);
        }
    );
    ImGuiControls::hooverTooltip(
        "Maximum plies to play from PGN opening before engines take over.\n"
        "Only applicable for PGN input.\n"
        "  all - Play full PGN sequence\n"
        "  0 - Engines start from PGN start position\n"
        "  N - Play N plies, then engines continue"
    );

    auto it = tutorialContext.annotations.find("Plies");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }

    return changed;
}

bool ImGuiTournamentOpening::drawFirstOpening(float inputWidth, 
    const Tutorial::TutorialContext& tutorialContext) {
    ImGui::SetNextItemWidth(inputWidth);
    bool changed = ImGuiControls::inputInt<uint32_t>("First opening", openings_.start, 0, 1000);
    ImGuiControls::hooverTooltip("Index of the first position to use (1-based).\nUseful for splitting test segments.");

    auto it = tutorialContext.annotations.find("First opening");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }
    
    return changed;
}

bool ImGuiTournamentOpening::drawRandomSeed(float inputWidth, 
    const Tutorial::TutorialContext& tutorialContext) {
    ImGui::SetNextItemWidth(inputWidth);
    bool changed = ImGuiControls::inputInt<uint32_t>("Random seed", openings_.seed, 0, 1000);
    ImGuiControls::hooverTooltip("Seed for random number generator when order is 'random'");

    auto it = tutorialContext.annotations.find("Random seed");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }
    
    return changed;
}

bool ImGuiTournamentOpening::drawSwitchPolicy(float inputWidth, 
    const Tutorial::TutorialContext& tutorialContext) {
    ImGui::SetNextItemWidth(inputWidth);
    bool changed = ImGuiControls::selectionBox("Switch policy", openings_.policy, { "default", "encounter", "round" });
    ImGuiControls::hooverTooltip(
        "When a new opening position is selected:\n"
        "  default - Fresh sequence per round, reused across pairings\n"
        "  encounter - New opening per engine pair (colors don't matter)\n"
        "  round - Same opening for all games in the round"
    );

    auto it = tutorialContext.annotations.find("Switch policy");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }

    return changed;
}

void ImGuiTournamentOpening::loadConfiguration() {
    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    auto openings = QaplaTester::OpeningConfig::fromConfigData(configData, id_);
    if (openings) {
        openings_ = *openings;
    }
}

std::vector<QaplaHelpers::IniFile::Section> ImGuiTournamentOpening::getSections() const {
    return QaplaTester::OpeningConfig::toSections(openings_, id_);
}

void ImGuiTournamentOpening::updateConfiguration() const {
    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    auto sections = QaplaTester::OpeningConfig::toSections(openings_, id_);
    configData.setSectionList("opening", id_, sections);
}
