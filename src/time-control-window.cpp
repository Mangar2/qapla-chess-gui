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
#include "imgui.h"

#include "qapla-tester/time-control.h"

#include <array>


using namespace QaplaWindows;

TimeControlWindow::TimeControlWindow() 
{
}

void TimeControlWindow::draw() {
	auto timeControls = QaplaConfiguration::Configuration::instance().getTimeControlSettings();

    // Radio button state
    static int selectedMode = 0;

    ImGui::PushID("TimeControlWindow");

    // Lambda to handle radio buttons and collapsing headers
    auto drawSection = [&](const char* radioButtonId, const char* headerLabel, int mode, 
        TimeControl& timeControl, auto drawFunction) {
        if (ImGui::RadioButton(radioButtonId, selectedMode == mode)) {
            selectedMode = mode;
        }
        ImGui::SameLine();
        if (ImGui::CollapsingHeader(headerLabel)) {
			ImGui::PushID(headerLabel);
            ImGui::Indent(32.0f);
            ImGui::PushItemWidth(150.0f);
            timeControl = drawFunction(timeControl);
            ImGui::PopItemWidth();
			ImGui::Unindent(32.0f);
            ImGui::PopID();
        }
    };

    // Draw each section
    drawSection("##blitz", "Blitz Time", 0, timeControls.blitzTime,
        [&](const TimeControl& tc) { return drawBlitzTime(tc); });
	drawSection("##tournament", "Tournament Time", 1, timeControls.tournamentTime,
        [&](const TimeControl& tc) { return drawTournamentTime(tc); });
	drawSection("##timePerMove", "Time per Move", 2, timeControls.timePerMove,
        [&](const TimeControl& tc) { return drawTimePerMove(tc); });
	drawSection("##fixedDepth", "Fixed Depth", 3, timeControls.fixedDepth,
        [&](const TimeControl& tc) { return drawFixedDepth(tc); });
	drawSection("##nodesPerMove", "Nodes per Move", 4, timeControls.nodesPerMove,
        [&](const TimeControl& tc) { return drawNodesPerMove(tc); });

    ImGui::PopID();
	QaplaConfiguration::Configuration::instance().setTimeControlSettings(timeControls);

}

TimeSegment TimeControlWindow::editTimeSegment(const TimeSegment& segment, bool blitz) {
    TimeSegment updatedSegment = segment;

    // Extract base time and increment from the segment
    uint64_t baseTimeMs = segment.baseTimeMs;
    uint64_t incrementMs = segment.incrementMs;

    // Convert base time to hours, minutes, and seconds
    int hours = static_cast<int>(baseTimeMs / 3600000);
    int minutes = static_cast<int>((baseTimeMs % 3600000) / 60000);
    int seconds = static_cast<int>((baseTimeMs % 60000) / 1000);

    // Convert increment to minutes, seconds, and milliseconds
    int incrementMinutes = static_cast<int>(incrementMs / 60000);
    int incrementSeconds = static_cast<int>((incrementMs % 60000) / 1000);
    int incrementMilliseconds = static_cast<int>(incrementMs % 1000);

    // Input fields for base time
    if (!blitz) {
        if (ImGui::InputInt("Hours", &hours, 1, 10)) {
            if (hours < 0) hours = 0;
        }
    }
    if (ImGui::InputInt("Minutes", &minutes, 1, 10)) {
        if (minutes < 0) minutes = 0;
        if (minutes > 59) minutes = 59;
    }
    if (ImGui::InputInt("Seconds", &seconds, 1, 10)) {
        if (seconds < 0) seconds = 0;
        if (seconds > 59) seconds = 59;
    }

    // Input fields for increment
    if (!blitz) {
        if (ImGui::InputInt("Increment Minutes", &incrementMinutes, 1, 10)) {
            if (incrementMinutes < 0) incrementMinutes = 0;
        }
    }
    if (ImGui::InputInt("Increment Seconds", &incrementSeconds, 1, 10)) {
        if (incrementSeconds < 0) incrementSeconds = 0;
        if (incrementSeconds > 59) incrementSeconds = 59;
    }
    if (ImGui::InputInt("Increment Milliseconds", &incrementMilliseconds, 1, 100)) {
        if (incrementMilliseconds < 0) incrementMilliseconds = 0;
        if (incrementMilliseconds > 999) incrementMilliseconds = 999;
    }

	// Input fields for moves to play
    if (!blitz) {
        if (ImGui::InputInt("Moves to Play", &updatedSegment.movesToPlay, 1, 10)) {
            if (updatedSegment.movesToPlay < 0) updatedSegment.movesToPlay = 0;
        }
    } else {
        updatedSegment.movesToPlay = 0; // Blitz mode does not use moves to play
	}

    // Recalculate base time and increment in milliseconds
    baseTimeMs = static_cast<uint64_t>(hours) * 3600000 +
        static_cast<uint64_t>(minutes) * 60000 +
        static_cast<uint64_t>(seconds) * 1000;
    incrementMs = static_cast<uint64_t>(incrementMinutes) * 60000 +
        static_cast<uint64_t>(incrementSeconds) * 1000 +
        static_cast<uint64_t>(incrementMilliseconds);

    // Update the segment
    updatedSegment.baseTimeMs = baseTimeMs;
    updatedSegment.incrementMs = incrementMs;

    return updatedSegment;
}

TimeSegment TimeControlWindow::selectPredefinedValues(
    const TimeSegment& segment,
    const std::vector<std::string>& predefinedLabels,
    const std::vector<int>& predefinedMinutes) {
    TimeSegment updatedSegment = segment;

    // Extract the current base time in minutes and seconds
    uint64_t baseTimeMs = segment.baseTimeMs;
    int minutes = static_cast<int>(baseTimeMs / 60000);
    int seconds = static_cast<int>((baseTimeMs % 60000) / 1000);

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
            bool isSelected = (selectedPredefinedIndex == static_cast<int>(i));
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
    updatedSegment.baseTimeMs = static_cast<uint64_t>(minutes) * 60000 + static_cast<uint64_t>(seconds) * 1000;

    return updatedSegment;
}

TimeControl TimeControlWindow::drawBlitzTime(const TimeControl& currentTimeControl) {
    TimeControl updatedTimeControl = currentTimeControl;

    // Retrieve the current time segments or default to 0
    TimeSegment timeSegment;
    if (!currentTimeControl.timeSegments().empty()) {
        timeSegment = currentTimeControl.timeSegments()[0];
	}
	timeSegment = editTimeSegment(timeSegment, true);
    timeSegment = selectPredefinedValues(timeSegment, 
        { "1 min", "2 min", "3 min", "5 min", "10 min", "15 min" }, 
		{ 1, 2, 3, 5, 10, 15 });

    TimeControl result;
    result.addTimeSegment(timeSegment);
	return result;
}

TimeControl TimeControlWindow::drawTournamentTime(const TimeControl& currentTimeControl) {
    TimeControl updatedTimeControl = currentTimeControl;

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
        if (segments[i].movesToPlay == 0) break;
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

    int seconds = static_cast<int>(moveTimeMs / 1000);
    int milliseconds = static_cast<int>(moveTimeMs % 1000);

    if (ImGui::InputInt("Seconds", &seconds, 1, 10)) {
        if (seconds < 0) seconds = 0;
    }

    if (ImGui::InputInt("Milliseconds", &milliseconds, 1, 100)) {
        if (milliseconds < 0) milliseconds = 0;
        if (milliseconds > 999) milliseconds = 999;
    }

    moveTimeMs = static_cast<uint64_t>(seconds) * 1000 + static_cast<uint64_t>(milliseconds);
    updatedTimeControl.setMoveTime(moveTimeMs);

    return updatedTimeControl;
}

TimeControl TimeControlWindow::drawFixedDepth(const TimeControl& currentTimeControl) {
    TimeControl updatedTimeControl = currentTimeControl;

    int depth = static_cast<int>(currentTimeControl.depth().value_or(0));

    if (ImGui::InputInt("Search Depth", &depth, 1, 10)) {
        if (depth < 0) depth = 0;
        if (depth > 100) depth = 100;
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
