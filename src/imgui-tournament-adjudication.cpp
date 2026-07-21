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

#include "imgui-tournament-adjudication.h"
#include "imgui-controls.h"
#include "configuration.h"
#include "config-group-loader.h"
#include "tournament-config-sections.h"

#include <config/adjudication-config.h>
#include <base-elements/string-helper.h>

#include <imgui.h>
#include <string>

using namespace QaplaWindows;

bool ImGuiTournamentAdjudication::draw(float inputWidth, float indent) {
    bool changed = false;

    // Draw Adjudication section
    if (ImGuiControls::CollapsingHeaderWithDot("Adjudicate draw", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("drawAdjudication");
        ImGui::Indent(indent);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::triStateInput("Active", 
            drawConfig_.active, drawConfig_.testOnly);
        ImGuiControls::hooverTooltip("Enable/disable draw adjudication or test mode (logs without adjudicating)");
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Min full moves", drawConfig_.minFullMoves, 0, 1000);
        ImGuiControls::hooverTooltip("Minimum number of full moves before draw adjudication can trigger");
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Required consecutive moves", drawConfig_.requiredConsecutiveMoves, 0, 1000);
        ImGuiControls::hooverTooltip("Number of consecutive moves within threshold needed to adjudicate draw");
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<int>("Centipawn threshold", drawConfig_.centipawnThreshold, -10000, 10000);
        ImGuiControls::hooverTooltip("Maximum absolute evaluation (in centipawns) to consider position drawn");
        ImGui::Unindent(indent);
        ImGui::PopID();
    }

    if (ImGuiControls::CollapsingHeaderWithDot("Adjudicate resign", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("resignAdjudication");
        ImGui::Indent(indent);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::triStateInput("Active", 
            resignConfig_.active, resignConfig_.testOnly);
        ImGuiControls::hooverTooltip("Enable/disable resign adjudication or test mode (logs without adjudicating)");
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Required consecutive moves", resignConfig_.requiredConsecutiveMoves, 0, 1000);
        ImGuiControls::hooverTooltip("Number of consecutive moves below threshold needed to adjudicate resign");
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<int>("Centipawn threshold", resignConfig_.centipawnThreshold, -10000, 10000);
        ImGuiControls::hooverTooltip("Evaluation threshold (in centipawns) below which position is considered lost");
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Both side decides", resignConfig_.twoSided);
        ImGuiControls::hooverTooltip("Require both engines to agree position is lost before adjudicating resign");
        ImGui::Unindent(indent);
        ImGui::PopID();
    }

    if (changed) {
        updateConfiguration();
    }

    return changed;
}

void ImGuiTournamentAdjudication::loadConfiguration() {
    // fromDrawManager/fromResignManager only report active=true when a group
    // instance is present at all, so an absent section round-trips to the
    // default-constructed (inactive) config.
    auto& drawManager = QaplaConfiguration::loadGroupIntoManager("draw", id_);
    drawConfig_ = QaplaTester::AdjudicationConfig::fromDrawManager(drawManager, "draw");

    auto& resignManager = QaplaConfiguration::loadGroupIntoManager("resign", id_);
    resignConfig_ = QaplaTester::AdjudicationConfig::fromResignManager(resignManager, "resign");
}

void ImGuiTournamentAdjudication::updateConfiguration() const {
    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();

    // Only persist a section when active: fromDrawManager/fromResignManager treat
    // "instance present" as the active signal, ignoring any stored "active" value.
    QaplaHelpers::IniFile::SectionList drawSections;
    if (drawConfig_.active) {
        drawSections.push_back(QaplaConfiguration::toDrawAdjudicationSection(drawConfig_, id_));
    }
    configData.setSectionList("draw", id_, drawSections);

    QaplaHelpers::IniFile::SectionList resignSections;
    if (resignConfig_.active) {
        resignSections.push_back(QaplaConfiguration::toResignAdjudicationSection(resignConfig_, id_));
    }
    configData.setSectionList("resign", id_, resignSections);
}
