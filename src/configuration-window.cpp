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

#include "configuration-window.h"
#include "imgui-controls.h"
#include "snackbar.h"
#include "configuration.h"
#include "tutorial.h"

#include <imgui.h>

using namespace QaplaWindows;

ConfigurationWindow::ConfigurationWindow() = default;
ConfigurationWindow::~ConfigurationWindow() = default;

void ConfigurationWindow::draw()
{
    ImGui::Spacing();
    
    if (ImGui::CollapsingHeader("Snackbar Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0F);
        drawSnackbarSettings();
        ImGui::Unindent(10.0F);
    }
    
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Tutorial Progress", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0F);
        drawTutorialSettings();
        ImGui::Unindent(10.0F);
    }
    
    ImGui::Spacing();
}

void ConfigurationWindow::drawSnackbarSettings()
{
    constexpr float inputWidth = 200.0F;
    constexpr uint32_t minDuration = 1;
    constexpr uint32_t maxDuration = 120;
    bool modified = false;

    auto& config = SnackbarManager::instance().getConfig();

    ImGui::Text("Message Display Durations (in seconds):");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::inputInt<uint32_t>("Note Duration", config.noteDurationInS, minDuration, maxDuration))
    {
        modified = true;
    }

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::inputInt<uint32_t>("Success Duration", config.successDurationInS, minDuration, maxDuration))
    {
        modified = true;
    }

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::inputInt<uint32_t>("Warning Duration", config.warningDurationInS, minDuration, maxDuration))
    {
        modified = true;
    }

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::inputInt<uint32_t>("Error Duration", config.errorDurationInS, minDuration, maxDuration))
    {
        modified = true;
    }

    if (modified) {
        SnackbarManager::instance().updateConfiguration();
    }
}

void ConfigurationWindow::drawTutorialSettings()
{
    ImGui::Text("Tutorial Topics:");
    ImGui::Spacing();

    for (int i = 0; i < static_cast<int>(Tutorial::Topic::Count); ++i) {
        auto topic = static_cast<Tutorial::Topic>(i);
        std::string topicName = Tutorial::getTopicName(topic);
        bool completed = Tutorial::instance().isCompleted(topic);
        bool previousCompleted = completed;

        ImGui::PushID(i);
        
        // Display completion status as a checkbox
        if (ImGui::Checkbox(("##completed_" + topicName).c_str(), &completed)) {
            // If checkbox is toggled on (incomplete -> complete), set tutorial to completed
            if (!previousCompleted && completed) {
                // Mark tutorial as completed by setting counter to completion threshold
                auto threshold = Tutorial::getCompletionThreshold(topic);
                switch (topic) {
                    case Tutorial::Topic::Snackbar:
                        for (uint32_t j = SnackbarManager::instance().getTutorialCounter(); j < threshold; ++j) {
                            SnackbarManager::instance().incrementTutorialCounter();
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", completed ? "Tutorial completed - uncheck to enable restart" : "Tutorial not completed - check to mark as complete");
        }
        
        ImGui::SameLine();
        ImGui::Text("%s", topicName.c_str());
        
        // Add restart button if not completed or if checkbox is unchecked
        if (!completed || previousCompleted != completed) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Restart")) {
                Tutorial::instance().restartTopic(topic);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Restart this tutorial from the beginning");
            }
        }
        
        ImGui::PopID();
    }
}
