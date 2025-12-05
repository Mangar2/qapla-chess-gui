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

#include "imgui-epd-configuration.h"
#include "imgui-controls.h"
#include "epd-data.h"

#include <imgui.h>

namespace QaplaWindows {

bool ImGuiEpdConfiguration::draw(const DrawOptions& options, float inputWidth, float indent,
    const Tutorial::TutorialContext& tutorialContext) {
    
    bool changed = false;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
    if (options.alwaysOpen) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (!ImGuiControls::CollapsingHeaderWithDot("Configuration", flags, tutorialContext.highlight)) {
        return false;
    }

    ImGui::PushID("epd-config");
    ImGui::Indent(indent);

    auto& config = EpdData::instance().config();
    constexpr int maxSeenPlies = 32;
    constexpr uint64_t maxTimeInS = 3600ULL * 24ULL * 365ULL;

    if (options.showSeenPlies) {
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Seen plies", config.seenPlies, 1, maxSeenPlies);
        ImGuiControls::hooverTooltip("Number of plies to play from position before starting engine analysis");
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("Seen plies");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
    }

    if (options.showMaxTime) {
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint64_t>("Max time (s)", config.maxTimeInS, 1, maxTimeInS, 1, 100);
        ImGuiControls::hooverTooltip("Maximum analysis time per position in seconds");
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("Max time");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
    }

    if (options.showMinTime) {
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint64_t>("Min time (s)", config.minTimeInS, 1, maxTimeInS, 1, 100);
        ImGuiControls::hooverTooltip("Minimum analysis time per position in seconds");
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("Min time");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
    }

    if (options.showFilePath) {
        ImGui::Spacing();
        changed |= ImGuiControls::existingFileInput("Epd or RAW position file:", config.filepath, inputWidth * 2.0F);
        ImGuiControls::hooverTooltip("Path to EPD or RAW position file to analyze");
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("FilePath");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
    }

    ImGui::Unindent(indent);
    ImGui::PopID();

    if (changed) {
        updateConfiguration();
    }

    return changed;
}

void ImGuiEpdConfiguration::updateConfiguration() const {
    EpdData::instance().updateConfiguration();
}

}
