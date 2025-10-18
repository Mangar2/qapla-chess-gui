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

#include "sprt-tournament-window.h"
#include "sprt-tournament-data.h"
#include "imgui-table.h"
#include "imgui-button.h"
#include "snackbar.h"
#include "os-dialogs.h"
#include "imgui-controls.h"
#include "imgui-engine-global-settings.h"
#include "configuration.h"

#include "qapla-tester/move-record.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/engine-event.h"
#include "qapla-tester/tournament.h"

#include <imgui.h>

#include <sstream>
#include <string>
#include <format>
#include <memory>
#include <optional>

using namespace QaplaWindows;

static void drawSingleButton(
    ImDrawList *drawList, ImVec2 topLeft, ImVec2 size,
    const std::string &button, bool running, QaplaButton::ButtonState state)
{
    if (button == "Run/Grace/Continue")
    {
        if (running)
        {
            QaplaButton::drawGrace(drawList, topLeft, size, state);
        }
        else
        {
            QaplaButton::drawPlay(drawList, topLeft, size, state);
        }
    }
    if (button == "Stop")
    {
        QaplaButton::drawStop(drawList, topLeft, size, state);
    }
    if (button == "Clear")
    {
        QaplaButton::drawClear(drawList, topLeft, size, state);
    }
    if (button == "Load")
    {
        QaplaButton::drawOpen(drawList, topLeft, size, state);
    }
    if (button == "Save As")
    {
        QaplaButton::drawSave(drawList, topLeft, size, state);
    }
}


void SprtTournamentWindow::drawButtons() {
    
}


void SprtTournamentWindow::executeCommand(const std::string &button) {
}

bool SprtTournamentWindow::drawInput() {
    constexpr float inputWidth = 200.0F;
    constexpr float fileInputWidth = inputWidth + 100.0F;
    auto& tournamentData = SprtTournamentData::instance();

    bool changed = false;

    if (ImGui::CollapsingHeader("Engines", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("engineSettings");
        ImGui::Indent(10.0F);
        changed |= tournamentData.engineSelect().draw();
        ImGui::Unindent(10.0F);
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Opening", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("opening");
        changed |= tournamentData.tournamentOpening().draw(inputWidth, fileInputWidth, 10.0F);
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("SPRT Configuration", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("sprtConfig");
        ImGui::Indent(10.0F);

        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<int>("Elo Lower (H0)", tournamentData.sprtConfig().eloLower, -1000, 1000);
        
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<int>("Elo Upper (H1)", tournamentData.sprtConfig().eloUpper, -1000, 1000);
        
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputPromille("Alpha (‰)", tournamentData.sprtConfig().alpha, 0.001, 0.5, 0.001);
        ImGui::SameLine();
        ImGui::Text("(%.3f)", tournamentData.sprtConfig().alpha);
        
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputPromille("Beta (‰)", tournamentData.sprtConfig().beta, 0.001, 0.5, 0.001);
        ImGui::SameLine();
        ImGui::Text("(%.3f)", tournamentData.sprtConfig().beta);
        
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Max Games", tournamentData.sprtConfig().maxGames, 1, 1000000);

        ImGui::Unindent(10.0F);
        ImGui::PopID();
    }

    ImGui::Spacing();

    return changed;
}

void SprtTournamentWindow::drawProgress() {
}

void SprtTournamentWindow::draw() {
    constexpr float rightBorder = 5.0F;
    auto& tournamentData = SprtTournamentData::instance();
    drawButtons();

    ImGui::Indent(10.0F);
    auto size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("InputArea", ImVec2(size.x - rightBorder, 0), ImGuiChildFlags_None);
    if (drawInput()) {
        tournamentData.updateConfiguration();
    }

    ImGui::EndChild();
    ImGui::Unindent(10.0F);
}
