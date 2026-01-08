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

#include "imgui-tournament-configuration.h"
#include "imgui-controls.h"
#include "configuration.h"
#include <base-elements/string-helper.h>
#include <tournament/tournament.h>
#include <config-file/tournament-config-file.h>

#include <imgui.h>
#include <string>

using namespace QaplaWindows;

bool ImGuiTournamentConfiguration::draw(const DrawOptions& options, float inputWidth, float indent,
    const Tutorial::TutorialContext& tutorialContext) {
    
    if (config_ == nullptr) {
        return false;
    }

    bool changed = false;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
    if (options.alwaysOpen) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (!ImGuiControls::CollapsingHeaderWithDot("Tournament", flags, tutorialContext.highlight)) {
        return false;
    }

    ImGui::PushID("tournament");
    ImGui::Indent(indent);

    if (options.showEvent) {
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputText("Event", config_->event);
        ImGuiControls::hooverTooltip("Optional event name for PGN or logging");
    }

    if (options.showType) {
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::selectionBox("Type", config_->type, { "gauntlet", "round-robin" });
        //changed |= ImGuiControls::inputText("Type", config_->type);
        ImGuiControls::hooverTooltip(
            "Tournament type:\n"
            "  gauntlet - One engine plays against all others\n"
            "  round-robin - Every engine plays against every other engine"
        );
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("Type");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
    }

    if (options.showRounds) {
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Rounds", config_->rounds, 1, 1000);
        ImGuiControls::hooverTooltip("Repeat all pairings this many times");
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("Rounds");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
    }

    if (options.showGamesPerPairing) {
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Games per pairing", config_->games, 1, 1000);
        ImGuiControls::hooverTooltip("Number of games per pairing.\nTotal games = games × rounds");
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("Games per pairing");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
    }

    if (options.showSameOpening) {
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Same opening", config_->repeat, 1, 1000);
        ImGuiControls::hooverTooltip(
            "Number of consecutive games played per opening.\n"
            "Commonly set to 2 to alternate colors with the same line"
        );
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("Same opening");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
    }

    if (options.showNoColorSwap) {
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("No color swap", config_->noSwap);
        ImGuiControls::hooverTooltip("Disable automatic color swap after each game");
    }

    if (options.showAverageElo) {
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<int>("Average Elo", config_->averageElo, 1000, 5000);
        ImGuiControls::hooverTooltip("Average Elo level for scaling rating output");
    }

    ImGui::Unindent(indent);
    ImGui::PopID();

    if (changed) {
        updateConfiguration();
    }

    return changed;
}

void ImGuiTournamentConfiguration::loadConfiguration() {
    if (config_ == nullptr) {
        return;
    }

    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    QaplaTester::TournamentConfigFile::loadFromConfigData(configData, *config_, id_);
}

std::vector<QaplaHelpers::IniFile::Section> ImGuiTournamentConfiguration::getSections() const {
    if (config_ == nullptr) {
        return {};
    }

    return QaplaTester::TournamentConfigFile::getSections(*config_, id_);
}

void ImGuiTournamentConfiguration::updateConfiguration() const {
    if (config_ == nullptr) {
        return;
    }

    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    QaplaTester::TournamentConfigFile::saveToConfigData(configData, *config_, id_);
}
