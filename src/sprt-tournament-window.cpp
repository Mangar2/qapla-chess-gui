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
#include "imgui-sprt-configuration.h"
#include "imgui-table.h"
#include "imgui-button.h"
#include "snackbar.h"
#include "os-dialogs.h"
#include "imgui-controls.h"
#include "imgui-engine-global-settings.h"
#include "configuration.h"

#include "move-record.h"
#include "game-record.h"
#include "string-helper.h"
#include "engine-event.h"
#include "tournament.h"
#include "sprt-manager.h"

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
    if (button == "RGC")
    {
        if (running)
        {
            QaplaButton::drawGrace(drawList, topLeft, size, state);
            ImGuiControls::hooverTooltip("Stop SPRT tournament gracefully after current games finish");
        }
        else
        {
            QaplaButton::drawPlay(drawList, topLeft, size, state);
            ImGuiControls::hooverTooltip(SprtTournamentData::instance().hasResults() 
                ? "Continue SPRT tournament with current configuration" 
                : "Start new SPRT tournament with current configuration");
        }
    }
    if (button == "Stop")
    {
        QaplaButton::drawStop(drawList, topLeft, size, state);
        ImGuiControls::hooverTooltip("Stop SPRT tournament or Monte Carlo test immediately");
    }
    if (button == "Clear")
    {
        QaplaButton::drawClear(drawList, topLeft, size, state);
        ImGuiControls::hooverTooltip("Clear all SPRT tournament data and results");
    }
    if (button == "Load")
    {
        QaplaButton::drawOpen(drawList, topLeft, size, state);
        ImGuiControls::hooverTooltip("Load SPRT tournament configuration and results from file");
    }
    if (button == "Save As")
    {
        QaplaButton::drawSave(drawList, topLeft, size, state);
        ImGuiControls::hooverTooltip("Save SPRT tournament configuration and results to file");
    }
    if (button == "Test")
    {
        QaplaButton::drawTest(drawList, topLeft, size, state);
        ImGuiControls::hooverTooltip("Run Monte Carlo test to estimate SPRT test duration");
    }
}

static QaplaButton::ButtonState getButtonState(const std::string& button) {
    auto state = SprtTournamentData::instance().state();
    
    if (button == "RGC" && state == SprtTournamentData::State::GracefulStopping) {
        return QaplaButton::ButtonState::Active;
    }
    
    if (button == "RGC" && !SprtTournamentData::instance().mayStartTournament(false)) {
        return QaplaButton::ButtonState::Disabled;
    }
    
    if ((button == "Stop") && !SprtTournamentData::instance().isAnyRunning()) {
        return QaplaButton::ButtonState::Disabled;
    }
    
    if (button == "Clear" && !SprtTournamentData::instance().hasResults()) {
        return QaplaButton::ButtonState::Disabled;
    }
    
    if (button == "Test" && SprtTournamentData::instance().isAnyRunning()) {
        return QaplaButton::ButtonState::Disabled;
    }
    
    if ((button == "Load" || button == "Save As") && SprtTournamentData::instance().isAnyRunning()) {
        return QaplaButton::ButtonState::Disabled;
    }
    
    return QaplaButton::ButtonState::Normal;
}


void SprtTournamentWindow::drawButtons() {
    constexpr float space = 3.0F;
    constexpr float topOffset = 5.0F;
    constexpr float bottomOffset = 8.0F;
    constexpr float leftOffset = 20.0F;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = { 25.0F, 25.0F };
    
    std::vector<std::string> allButtons = { "RGC", "Stop", "Clear", "Test", "Load", "Save As" };
    const auto totalSize = QaplaButton::calcIconButtonsTotalSize(buttonSize, allButtons);
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    for (const std::string& button : allButtons) {
        ImGui::SetCursorScreenPos(pos);
        bool running = SprtTournamentData::instance().isRunning();
        auto label = button;
        
        if (label == "RGC") {
            label = running ? "Grace" : "Run";
        } 
        if (label == "RGC" && SprtTournamentData::instance().hasResults()) {
            label = "Continue";
        }

        auto state = getButtonState(button);

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
        if (button == "RGC") {
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
        } else if (button == "Test") {
            SprtTournamentData::instance().runMonteCarloTest();
        } else if (button == "Load") {
            if (SprtTournamentData::instance().isAnyRunning()) {
                SnackbarManager::instance().showWarning("Cannot load tournament while running");
                return;
            }
            auto selectedPath = OsDialogs::openFileDialog(false, 
                { {"Qapla SPRT Files", "*.qsprt"}, {"All Files", "*.*"} });
            if (!selectedPath.empty() && !selectedPath[0].empty()) {
                SprtTournamentData::instance().loadTournament(selectedPath[0]);
            }
        } else if (button == "Save As") {
            if (SprtTournamentData::instance().isAnyRunning()) {
                SnackbarManager::instance().showWarning("Cannot save tournament while running");
                return;
            }
            auto selectedPath = OsDialogs::saveFileDialog({ {"Qapla SPRT Files", "qsprt"} });
            if (!selectedPath.empty()) {
                SprtTournamentData::saveTournament(selectedPath);
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
    auto concurrency = tournamentData.getExternalConcurrency();
    ImGuiControls::sliderInt<uint32_t>("Concurrency", concurrency, 1, maxConcurrency);
    ImGuiControls::hooverTooltip("Number of games running in parallel");
    tournamentData.setExternalConcurrency(concurrency);
    tournamentData.setPoolConcurrency(concurrency, true);
    drawProgress();
    
    ImGui::Spacing();
    if (tournamentData.isAnyRunning()) {
        ImGui::Indent(10.0F);
        if (tournamentData.isRunning()) {
            ImGui::Text("SPRT tournament is running");
        }
        if (tournamentData.isMonteCarloTestRunning()) {
            ImGui::Text("Monte Carlo test is running");
        }
        ImGui::Unindent(10.0F);
        return false;
    }

    bool changed = false;

    changed |= tournamentData.getGlobalSettings().drawGlobalSettings(
        { .controlWidth = inputWidth, .controlIndent = 10.0F }, {});
    changed |= tournamentData.getEngineSelect().draw();
    changed |= tournamentData.tournamentOpening().draw(
        { .inputWidth = inputWidth, .fileInputWidth = fileInputWidth, .indent = 10.0F });

    changed |= tournamentData.sprtConfiguration().draw();

    changed |= tournamentData.getGlobalSettings().drawTimeControl(
        { .controlWidth = inputWidth, .controlIndent = 10.0F }, false, false);
    changed |= tournamentData.tournamentPgn().draw(
        { .inputWidth = inputWidth, .fileInputWidth = fileInputWidth });
    changed |= tournamentData.tournamentAdjudication().draw(inputWidth, 10.0F);

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
    drawInput();

    tournamentData.drawMonteCarloTable(ImVec2(size.x, 400.0F));
    tournamentData.drawResultTable(ImVec2(size.x, 100.0F));
    tournamentData.drawSprtTable(ImVec2(size.x, 100.0F));
    tournamentData.drawCauseTable(ImVec2(size.x, 400.0F));

    ImGui::EndChild();
    ImGui::Unindent(10.0F);
}
