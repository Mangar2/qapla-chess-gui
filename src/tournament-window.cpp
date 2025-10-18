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

#include "tournament-window.h"
#include "tournament-data.h"
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


void TournamentWindow::drawButtons() {
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
        bool running = TournamentData::instance().isRunning();
        auto label = button;
        
        if (label == "Run/Grace/Continue") {
            label = running ? "Grace" : "Run";
        } 
        if (label == "Run/Grace/Continue" && TournamentData::instance().hasTasksScheduled()) {
            label = "Continue";
        }

        auto state = QaplaButton::ButtonState::Normal;
        if (button == "Run/Grace/Continue" && TournamentData::instance().state() == TournamentData::State::GracefulStopping) {
            state = QaplaButton::ButtonState::Active;
        }
        if ((button == "Stop") && !TournamentData::instance().isRunning()) {
            state = QaplaButton::ButtonState::Disabled;
        }
        if (button == "Clear" && !TournamentData::instance().hasTasksScheduled()) {
            state = QaplaButton::ButtonState::Disabled;
        }
        if ((button == "Load" || button == "Save As") && TournamentData::instance().isRunning()) {
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


void QaplaWindows::TournamentWindow::executeCommand(const std::string &button)
{
    {
        try
        {
            if (button == "Run/Grace/Continue")
            {
                bool running = TournamentData::instance().isRunning();
                if (running)
                {
                    TournamentData::instance().stopPool(true);
                } else {
                    TournamentData::instance().startTournament();
                }
            } else if (button == "Stop")
            {
                TournamentData::instance().stopPool();
            } else if (button == "Clear")
            {
                TournamentData::instance().clear();
            } else if (button == "Load")
            {
                auto selectedPath = OsDialogs::openFileDialog();
                if (!selectedPath.empty() && !selectedPath[0].empty())
                {
                    TournamentData::instance().loadTournament(selectedPath[0]);
                }
            } else if (button == "Save As")
            {
                auto selectedPath = OsDialogs::saveFileDialog({ {"Qapla Tournament Files", "qtour"} });
                if (!selectedPath.empty())
                {
                    TournamentData::saveTournament(selectedPath);
                }
            }
        }
        catch (const std::exception &e)
        {
            SnackbarManager::instance().showError(e.what());
        }
    }
}



bool TournamentWindow::drawInput() {
    
	constexpr float inputWidth = 200.0F;
    constexpr float fileInputWidth = inputWidth + 100.0F;
    constexpr int maxConcurrency = 32;
	auto& tournamentData = TournamentData::instance();
    
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiControls::sliderInt<uint32_t>("Concurrency", tournamentData.concurrency(), 1, maxConcurrency);
    tournamentData.setPoolConcurrency(tournamentData.concurrency(), true);
    drawProgress();
    
    ImGui::Spacing();
    if (tournamentData.isRunning()) {
		ImGui::Indent(10.0F);
        ImGui::Text("Tournament is running");
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
    if (ImGui::CollapsingHeader("Tournament", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("tournament");
        ImGui::Indent(10.0F);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputText("Event", tournamentData.config().event);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::selectionBox("Type", tournamentData.config().type, { "gauntlet", "round-robin" });
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Rounds", tournamentData.config().rounds, 1, 1000);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Games per pairing", tournamentData.config().games, 1, 1000);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Same opening", tournamentData.config().repeat, 1, 1000);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("No color swap", tournamentData.config().noSwap);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<int>("Average Elo", tournamentData.config().averageElo, 1000, 5000);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Save interval (s)", tournamentData.config().saveInterval, 0, 1000);
        ImGui::Unindent(10.0F);
        ImGui::PopID();
    }
    
    // Draw time control and global engine settings using the new class
    changed |= tournamentData.globalSettings().drawTimeControl(inputWidth, 10.0F, false);
    
    if (ImGui::CollapsingHeader("Pgn", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("pgn");
        changed |= tournamentData.tournamentPgn().draw(inputWidth, fileInputWidth, 10.0F);
        ImGui::PopID();
	}
    if (ImGui::CollapsingHeader("Adjudicate draw", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("drawAdjudication");
        ImGui::Indent(10.0F);
        ImGui::SetNextItemWidth(inputWidth);
   		changed |= ImGuiControls::triStateInput("Active", 
            tournamentData.drawConfig().active, tournamentData.drawConfig().testOnly);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<uint32_t>("Min full moves", tournamentData.drawConfig().minFullMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<uint32_t>("Required consecutive moves", tournamentData.drawConfig().requiredConsecutiveMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<int>("Centipawn threshold", tournamentData.drawConfig().centipawnThreshold, -10000, 10000);
        ImGui::Unindent(10.0F);
        ImGui::PopID();
	}
    if (ImGui::CollapsingHeader("Adjudicate resign", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("resignAdjudication");
        ImGui::Indent(10.0F);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::triStateInput("Active", 
            tournamentData.resignConfig().active, tournamentData.resignConfig().testOnly);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<uint32_t>("Required consecutive moves", tournamentData.resignConfig().requiredConsecutiveMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<int>("Centipawn threshold", tournamentData.resignConfig().centipawnThreshold, -10000, 10000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Both side decides", tournamentData.resignConfig().twoSided);
        ImGui::Unindent(10.0F);
        ImGui::PopID();
	}
	
    ImGui::Spacing();

    return changed;
}

void TournamentWindow::drawProgress()
{
    auto& tournamentData = TournamentData::instance();
    auto totalGames = tournamentData.getTotalScheduledGames();
    auto playedGames = tournamentData.getPlayedGames();
    
    if (totalGames == 0) {
        return;
    }
    
    float progress = static_cast<float>(playedGames) / static_cast<float>(totalGames);
    ImGui::ProgressBar(progress, ImVec2(ImGui::GetContentRegionAvail().x, 0.0F), 
        std::to_string(playedGames).c_str());
}

void TournamentWindow::draw() {
    constexpr float rightBorder = 5.0F;
    auto& tournamentData = TournamentData::instance();
    drawButtons();

    ImGui::Indent(10.0F);
    auto size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("InputArea", ImVec2(size.x - rightBorder, 0), ImGuiChildFlags_None);
    if (drawInput()) {
        tournamentData.updateConfiguration();
    }
    tournamentData.drawRunningTable(ImVec2(size.x, 600.0F));
    tournamentData.drawEloTable(ImVec2(size.x, 400.0F));
    tournamentData.drawCauseTable(ImVec2(size.x, 400.0F));
    if (tournamentData.drawConfig().testOnly || tournamentData.resignConfig().testOnly) {
        tournamentData.drawAdjudicationTable(ImVec2(size.x, 200.0F));
    }

    ImGui::EndChild();
    ImGui::Unindent(10.0F);
}

