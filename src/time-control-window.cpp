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

#include "time-control-window.h"
#include "configuration.h"
#include "imgui-controls.h"
#include <imgui.h>

#include "time-control.h"
#include "string-helper.h"

#include <array>
#include <utility>
#include <limits>


using QaplaTester::TimeSegment;
using QaplaTester::TimeControl;

using namespace QaplaWindows;

constexpr uint64_t millisecondsInMinute = 60000;
constexpr int millisecondsInSecond = 1000;
constexpr float baseIndent = 10.0F;  
constexpr float inputIndent = 32.0F;  // Indentation for input fields
constexpr float inputWidth = 150.0F;  // Width for input fields
constexpr int fastStep = 10;



TimeControlWindow::TimeControlWindow() {
};

TimeControlWindow::~TimeControlWindow() = default;

void TimeControlWindow::setFromConfiguration(const std::string& id) {
    auto sections = QaplaConfiguration::Configuration::instance().
        getConfigData().getSectionList("timecontrol", id).value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (sections.empty()) {
        return; // Use default settings
    }
    
    for (const auto& section : sections) {
        auto nameOpt = section.getValue("name");
        if (!nameOpt) {
            continue;
        }
        
        const std::string& name = *nameOpt;
        
        if (name == "BlitzTime") {
            timeControlSettings_.blitzTime.fromSection(section);
        }
        else if (name == "TournamentTime") {
            timeControlSettings_.tournamentTime.fromSection(section);
        }
        else if (name == "TimePerMove") {
            timeControlSettings_.timePerMove.fromSection(section);
        }
        else if (name == "FixedDepth") {
            timeControlSettings_.fixedDepth.fromSection(section);
        }
        else if (name == "NodesPerMove") {
            timeControlSettings_.nodesPerMove.fromSection(section);
        }
    }
    
    // Load selected time control
    auto boardSections = QaplaConfiguration::Configuration::instance().
        getConfigData().getSectionList("board", id).value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!boardSections.empty()) {
        auto timeControlOpt = boardSections[0].getValue("timecontrol");
        if (timeControlOpt) {
            try {
                timeControlSettings_.setSelectionFromString(*timeControlOpt);
            } catch (const std::exception&) {
                // Ignore invalid selection, use default
            }
        }
    }
}

void TimeControlWindow::updateConfiguration(const std::string& id) const {
    // Save time control sections
    std::vector<QaplaHelpers::IniFile::Section> sections;
    sections.push_back(timeControlSettings_.blitzTime.toSection("BlitzTime"));
    sections.push_back(timeControlSettings_.tournamentTime.toSection("TournamentTime"));
    sections.push_back(timeControlSettings_.timePerMove.toSection("TimePerMove"));
    sections.push_back(timeControlSettings_.fixedDepth.toSection("FixedDepth"));
    sections.push_back(timeControlSettings_.nodesPerMove.toSection("NodesPerMove"));

    for (auto& section : sections) {
        section.insertFirst("id", id);
    }
    
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("timecontrol", id, sections);
    
    // Save selected time control
    QaplaHelpers::IniFile::Section boardSection{
        .name = "board",  // Type name, not instance id
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", id},
            {"timecontrol", timeControlSettings_.getSelectionString()}
        }
    };
    
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("board", id, { boardSection });
}

const QaplaTester::TimeControl& TimeControlWindow::getSelectedTimeControl() const {
    return timeControlSettings_.getSelectedTimeControl();
}

std::string TimeControlWindow::computeActiveButtonId() const {
    switch (timeControlSettings_.selected) {
    case SelectedTimeControl::Blitz:
        return "##blitz";
    case SelectedTimeControl::Tournament:
        return "##tournament";
    case SelectedTimeControl::TimePerMove:
        return "##timePerMove";
    case SelectedTimeControl::FixedDepth:
        return "##fixedDepth";
    case SelectedTimeControl::NodesPerMove:
        return "##nodesPerMove";
    default:
        return "##blitz";
    }
}

void TimeControlWindow::draw() {
    std::string activeButtonId = computeActiveButtonId();
    ImGui::Spacing();
    constexpr float rightBorder = 5.0F;
    ImGui::Indent(10.0F);
    auto size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("TimeControlWindow", ImVec2(size.x - rightBorder, 0), ImGuiChildFlags_None);
    
    // Lambda to handle radio buttons and collapsing headers
    auto drawSection = [&](const char* radioButtonId, const char* headerLabel,  
        TimeControl& timeControl, auto drawFunction) {
        if (ImGui::RadioButton(radioButtonId, activeButtonId == radioButtonId)) {
            if (std::string(radioButtonId) == "##blitz") {
                timeControlSettings_.selected = SelectedTimeControl::Blitz;
            }
            else if (std::string(radioButtonId) == "##tournament") {
                timeControlSettings_.selected = SelectedTimeControl::Tournament;
            }
            else if (std::string(radioButtonId) == "##timePerMove") {
                timeControlSettings_.selected = SelectedTimeControl::TimePerMove;
            }
            else if (std::string(radioButtonId) == "##fixedDepth") {
                timeControlSettings_.selected = SelectedTimeControl::FixedDepth;
            }
            else if (std::string(radioButtonId) == "##nodesPerMove") {
                timeControlSettings_.selected = SelectedTimeControl::NodesPerMove;
            }
            else {
				throw(std::runtime_error("Unknown radio button ID: " + std::string(radioButtonId)));
            }
        }
        ImGui::SameLine();
        if (ImGui::CollapsingHeader(headerLabel)) {
			ImGui::PushID(headerLabel);
            ImGui::Indent(inputIndent);
            ImGui::PushItemWidth(inputWidth);
            timeControl = drawFunction(timeControl);
            ImGui::PopItemWidth();
			ImGui::Unindent(inputIndent);
            ImGui::PopID();
        }
    };

    // Draw each section
    drawSection("##blitz", "Blitz Time", timeControlSettings_.blitzTime,
        [&](const TimeControl& timeControl) { return drawBlitzTime(timeControl); });
	drawSection("##tournament", "Tournament Time", timeControlSettings_.tournamentTime,
        [&](const TimeControl& timeControl) { return drawTournamentTime(timeControl); });
	drawSection("##timePerMove", "Time per Move", timeControlSettings_.timePerMove,
        [&](const TimeControl& timeControl) { return drawTimePerMove(timeControl); });
	drawSection("##fixedDepth", "Fixed Depth", timeControlSettings_.fixedDepth,
        [&](const TimeControl& timeControl) { return drawFixedDepth(timeControl); });
	drawSection("##nodesPerMove", "Nodes per Move", timeControlSettings_.nodesPerMove,
        [&](const TimeControl& timeControl) { return drawNodesPerMove(timeControl); });

    ImGui::EndChild();
    ImGui::Unindent(baseIndent);
}

TimeSegment TimeControlWindow::editTimeSegment(const TimeSegment& segment, bool blitz) {
    std::string timeStr = to_string(segment);
    if (ImGuiControls::timeControlInput(timeStr, blitz)) {
        return TimeSegment::fromString(timeStr);
    }
    return segment;
}

/**
 * @brief Allows the user to select predefined time values for a TimeSegment.
 *
 * @param segment The current TimeSegment to be updated.
 * @param predefinedLabels A vector of strings representing the labels for predefined values.
 * @param predefinedMinutes A vector of integers representing the corresponding predefined minute values.
 * @return TimeSegment The updated TimeSegment with the selected predefined value.
 */
static TimeSegment selectPredefinedValues(
    const TimeSegment& segment,
    const std::vector<std::string>& predefinedLabels,
    const std::vector<int>& predefinedMinutes) {

    TimeSegment updatedSegment = segment;

    // Extract the current base time in minutes and seconds
    uint64_t baseTimeMs = segment.baseTimeMs;
    int minutes = static_cast<int>(baseTimeMs / millisecondsInMinute);
    int seconds = static_cast<int>((baseTimeMs % millisecondsInMinute) / millisecondsInSecond);

    // Determine the currently selected predefined value
    int selectedPredefinedIndex = -1;
    if (seconds == 0) {
        for (size_t i = 0; i < predefinedMinutes.size(); ++i) {
            if (minutes == predefinedMinutes[i]) {
                selectedPredefinedIndex = static_cast<int>(i);
                break;
            }
        }
    }

    // Display the combo box for predefined values
    if (ImGui::BeginCombo("Predefined Times", selectedPredefinedIndex >= 0 ? 
        predefinedLabels[selectedPredefinedIndex].c_str() : "Custom")) {
        for (size_t i = 0; i < predefinedLabels.size(); ++i) {
            bool isSelected = std::cmp_equal(selectedPredefinedIndex, i);
            if (ImGui::Selectable(predefinedLabels[i].c_str(), isSelected)) {
                selectedPredefinedIndex = static_cast<int>(i);
                minutes = predefinedMinutes[i];
                seconds = 0;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Recalculate the base time in milliseconds
    updatedSegment.baseTimeMs = static_cast<uint64_t>(minutes) * millisecondsInMinute + 
        static_cast<uint64_t>(seconds) * millisecondsInSecond;

    return updatedSegment;
}

TimeControl TimeControlWindow::drawBlitzTime(const TimeControl& currentTimeControl) {

    const std::vector<int> blitzTimeMinutes = { 1, 2, 3, 5, 10, 15 };
    const std::vector<std::string> blitzTimeLabels = { 
        "1 min", "2 min", "3 min", "5 min", "10 min", "15 min" 
    };

    // Retrieve the current time segments or default to 0
    TimeSegment timeSegment;
    if (!currentTimeControl.timeSegments().empty()) {
        timeSegment = currentTimeControl.timeSegments()[0];
	}
	timeSegment = editTimeSegment(timeSegment, true);
    timeSegment = selectPredefinedValues(timeSegment, blitzTimeLabels, blitzTimeMinutes);

    TimeControl result;
    result.addTimeSegment(timeSegment);
	return result;
}

TimeControl TimeControlWindow::drawTournamentTime(const TimeControl& currentTimeControl) {

    // Predefined settings for tournament time
    const std::vector<std::string> predefinedLabels = {
        "2:30 h", "2:00 h", "1:30 h", "1:00 h", "0:45 h", "0:30 h", "0:20 h", "0:15 h", "0:10 h", "0:05 h"
    };
    const std::vector<int> predefinedMinutes = { 150, 120, 90, 60, 45, 30, 20, 15, 10, 5 };

    // Retrieve the current time segments
    std::vector<TimeSegment> segments = currentTimeControl.timeSegments();
    if (segments.empty()) {
        segments.push_back(TimeSegment{}); // Add an initial segment if none exist
    }

    // Iterate over the segments and allow editing
    for (size_t i = 0; i < segments.size(); ++i) {
        ImGui::PushID(static_cast<int>(i)); // Ensure unique IDs for ImGui widgets

        // Edit the current segment
        segments[i] = editTimeSegment(segments[i], false);

        // Apply predefined settings
        segments[i] = selectPredefinedValues(segments[i], predefinedLabels, predefinedMinutes);

        // If the current segment has "movesToPlay" set to 0, ensure a following segment exists
        if (segments[i].movesToPlay > 0 && i == segments.size() - 1) {
            segments.push_back(TimeSegment{});
        }

        ImGui::Separator(); // Visual separator between segments
        ImGui::PopID();
        if (segments[i].movesToPlay == 0) { break; }
    }

    // Update the TimeControl with the modified segments
    TimeControl result;
    for (const auto& segment : segments) {
        result.addTimeSegment(segment);
    }

    return result;
}

TimeControl TimeControlWindow::drawTimePerMove(const TimeControl& currentTimeControl) {
    TimeControl updatedTimeControl = currentTimeControl;

    uint64_t moveTimeMs = currentTimeControl.moveTimeMs().value_or(0);

    int seconds = static_cast<int>(moveTimeMs / millisecondsInSecond);
    int milliseconds = static_cast<int>(moveTimeMs % millisecondsInSecond);

	ImGuiControls::inputInt<int>("Seconds", seconds, 0, std::numeric_limits<int>::max(), 1, fastStep);
	ImGuiControls::inputInt<int>("Milliseconds", milliseconds, 0, millisecondsInSecond - 1, 1, fastStep);

    moveTimeMs = static_cast<uint64_t>(seconds) * millisecondsInSecond + static_cast<uint64_t>(milliseconds);
    updatedTimeControl.setMoveTime(moveTimeMs);

    return updatedTimeControl;
}

TimeControl TimeControlWindow::drawFixedDepth(const TimeControl& currentTimeControl) {
    static constexpr int maxDepth = 100;
    TimeControl updatedTimeControl = currentTimeControl;

    int depth = static_cast<int>(currentTimeControl.depth().value_or(0));

    if (ImGui::InputInt("Search Depth", &depth, 1, fastStep)) {
        depth = std::clamp(depth, 0, maxDepth);
        updatedTimeControl.setDepth(static_cast<uint32_t>(depth));
    }

    return updatedTimeControl;
}

TimeControl TimeControlWindow::drawNodesPerMove(const TimeControl& currentTimeControl) {
    TimeControl updatedTimeControl = currentTimeControl;

    uint32_t nodes = currentTimeControl.nodes().value_or(0);

    if (ImGui::InputScalar("Nodes per Move", ImGuiDataType_U32, &nodes, nullptr, nullptr)) {
        updatedTimeControl.setNodes(nodes);
    }

    return updatedTimeControl;
}
