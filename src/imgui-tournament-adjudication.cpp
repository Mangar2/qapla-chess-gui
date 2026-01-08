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

#include "base-elements/string-helper.h"

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
    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    
    // Load draw adjudication configuration
    auto drawSections = configData.getSectionList("drawadjudication", id_);
    if (drawSections && !drawSections->empty()) {
        for (const auto& [key, value] : (*drawSections)[0].entries) {
            if (key == "active") {
                drawConfig_.active = (value == "true");
            }
            else if (key == "minFullMoves") {
                drawConfig_.minFullMoves = QaplaHelpers::to_uint32(value).value_or(0);
            }
            else if (key == "requiredConsecutiveMoves") {
                drawConfig_.requiredConsecutiveMoves = QaplaHelpers::to_uint32(value).value_or(0);
            }
            else if (key == "centipawnThreshold") {
                drawConfig_.centipawnThreshold = QaplaHelpers::to_int(value).value_or(0);
            }
            else if (key == "testOnly") {
                drawConfig_.testOnly = (value == "true");
            }
        }
    }

    // Load resign adjudication configuration
    auto resignSections = configData.getSectionList("resignadjudication", id_);
    if (resignSections && !resignSections->empty()) {
        for (const auto& [key, value] : (*resignSections)[0].entries) {
            if (key == "active") {
                resignConfig_.active = (value == "true");
            }
            else if (key == "requiredConsecutiveMoves") {
                resignConfig_.requiredConsecutiveMoves = QaplaHelpers::to_uint32(value).value_or(0);
            }
            else if (key == "centipawnThreshold") {
                resignConfig_.centipawnThreshold = QaplaHelpers::to_int(value).value_or(0);
            }
            else if (key == "twoSided") {
                resignConfig_.twoSided = (value == "true");
            }
            else if (key == "testOnly") {
                resignConfig_.testOnly = (value == "true");
            }
        }
    }
}

std::vector<QaplaHelpers::IniFile::Section> ImGuiTournamentAdjudication::getSections() const {
    std::vector<QaplaHelpers::IniFile::Section> sections;

    // Draw adjudication section
    QaplaHelpers::IniFile::KeyValueMap drawEntries{
        {"id", id_},
        {"active", drawConfig_.active ? "true" : "false"},
        {"minFullMoves", std::to_string(drawConfig_.minFullMoves)},
        {"requiredConsecutiveMoves", std::to_string(drawConfig_.requiredConsecutiveMoves)},
        {"centipawnThreshold", std::to_string(drawConfig_.centipawnThreshold)},
        {"testOnly", drawConfig_.testOnly ? "true" : "false"}
    };
    sections.push_back({
        .name = "drawadjudication",
        .entries = drawEntries
    });

    // Resign adjudication section
    QaplaHelpers::IniFile::KeyValueMap resignEntries{
        {"id", id_},
        {"active", resignConfig_.active ? "true" : "false"},
        {"requiredConsecutiveMoves", std::to_string(resignConfig_.requiredConsecutiveMoves)},
        {"centipawnThreshold", std::to_string(resignConfig_.centipawnThreshold)},
        {"twoSided", resignConfig_.twoSided ? "true" : "false"},
        {"testOnly", resignConfig_.testOnly ? "true" : "false"}
    };
    sections.push_back({
        .name = "resignadjudication",
        .entries = resignEntries
    });

    return sections;
}

void ImGuiTournamentAdjudication::updateConfiguration() const {
    auto sections = getSections();
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "drawadjudication", id_, { sections[0] });
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "resignadjudication", id_, { sections[1] });
}
