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
#include "qapla-tester/sprt-manager.h"

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
    constexpr float space = 3.0F;
    constexpr float topOffset = 5.0F;
    constexpr float bottomOffset = 8.0F;
    constexpr float leftOffset = 20.0F;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = { 25.0F, 25.0F };
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    for (const std::string button : { "Run/Grace/Continue", "Stop", "Clear", "Load", "Save As" }) {
        ImGui::SetCursorScreenPos(pos);
        bool running = SprtTournamentData::instance().isRunning();
        auto label = button;
        
        if (label == "Run/Grace/Continue") {
            label = running ? "Grace" : "Run";
        } 
        if (label == "Run/Grace/Continue" && SprtTournamentData::instance().hasTasksScheduled()) {
            label = "Continue";
        }

        auto state = QaplaButton::ButtonState::Normal;
        if (button == "Run/Grace/Continue" && SprtTournamentData::instance().state() == SprtTournamentData::State::GracefulStopping) {
            state = QaplaButton::ButtonState::Active;
        }
        if ((button == "Stop") && !SprtTournamentData::instance().isRunning()) {
            state = QaplaButton::ButtonState::Disabled;
        }
        if (button == "Clear" && !SprtTournamentData::instance().hasTasksScheduled()) {
            state = QaplaButton::ButtonState::Disabled;
        }
        if ((button == "Load" || button == "Save As") && SprtTournamentData::instance().isRunning()) {
            state = QaplaButton::ButtonState::Disabled;
        }

        if (QaplaButton::drawIconButton(button, label, buttonSize, state,
                [&button, running, state](ImDrawList *drawList, ImVec2 topLeft, ImVec2 size)->void {
                    drawSingleButton(drawList, topLeft, size, button, running, state);
                }))
        {
            executeCommand(button);
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}


void SprtTournamentWindow::executeCommand(const std::string &button) {
    try {
        if (button == "Run/Grace/Continue") {
            bool running = SprtTournamentData::instance().isRunning();
            if (running) {
                SprtTournamentData::instance().stopPool(true);
            } else {
                SprtTournamentData::instance().startTournament();
            }
        } else if (button == "Stop") {
            SprtTournamentData::instance().stopPool();
        } else if (button == "Clear") {
            SprtTournamentData::instance().clear();
        } else if (button == "Load") {
            auto selectedPath = OsDialogs::openFileDialog();
            if (!selectedPath.empty() && !selectedPath[0].empty()) {
                // TODO: Implement loadTournament for SPRT
                SnackbarManager::instance().showNote("Load functionality not yet implemented for SPRT tournaments.");
            }
        } else if (button == "Save As") {
            auto selectedPath = OsDialogs::saveFileDialog({ {"Qapla SPRT Files", "qsprt"} });
            if (!selectedPath.empty()) {
                // TODO: Implement saveTournament for SPRT
                SnackbarManager::instance().showNote("Save functionality not yet implemented for SPRT tournaments.");
            }
        }
    } catch (const std::exception &e) {
        SnackbarManager::instance().showError(e.what());
    }
}


bool SprtTournamentWindow::drawInput() {
    constexpr float inputWidth = 200.0F;
    constexpr float fileInputWidth = inputWidth + 100.0F;
    constexpr int maxConcurrency = 32;
    auto& tournamentData = SprtTournamentData::instance();

    ImGui::SetNextItemWidth(inputWidth);
    ImGuiControls::sliderInt<uint32_t>("Concurrency", tournamentData.concurrency(), 1, maxConcurrency);
    tournamentData.setPoolConcurrency(tournamentData.concurrency(), true);
    drawProgress();
    
    ImGui::Spacing();
    if (tournamentData.isRunning()) {
        ImGui::Indent(10.0F);
        ImGui::Text("SPRT tournament is running");
        ImGui::Unindent(10.0F);
        return false;
    }

    bool changed = false;

    changed |= tournamentData.globalSettings().drawGlobalSettings(inputWidth, 10.0F);

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

    if (ImGui::CollapsingHeader("Pgn", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("pgn");
        changed |= tournamentData.tournamentPgn().draw(inputWidth, fileInputWidth, 10.0F);
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

    changed |= tournamentData.globalSettings().drawTimeControl(inputWidth, 10.0F, false);

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
