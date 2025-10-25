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
#include "engine-setup-window.h"
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

    for (int tutorial = 0; tutorial < Tutorial::instance().getEntries().size(); ++tutorial) {
        auto& entry = Tutorial::instance().getEntries()[tutorial];
        bool completed = entry.completed();
        if (ImGui::Checkbox(entry.displayName.c_str(), &completed)) {
            if (completed) {
                entry.finish();
            } else {
                entry.reset();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", completed ? "Tutorial completed - uncheck to restart" : "Tutorial not completed - check to mark as complete");
        }
    }
}
