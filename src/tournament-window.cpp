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

#include "qapla-tester/move-record.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/engine-event.h"
#include "qapla-tester/tournament.h"

#include "imgui.h"

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;


TournamentWindow::TournamentWindow()
	: tournamentData_(std::make_unique<TournamentData>())
{
}

TournamentWindow::~TournamentWindow() = default;

void TournamentWindow::drawButtons() {
    constexpr float space = 3.0f;
    constexpr float topOffset = 5.0f;
    constexpr float bottomOffset = 8.0f;
    constexpr float leftOffset = 20.0f;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = { 25.0f, 25.0f };
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    for (const std::string button : { "Run", "Stop", "Clear" }) {
        ImGui::SetCursorScreenPos(pos);
        if (QaplaButton::drawIconButton(button, button, buttonSize, false,
            [&button](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
                if (button == "Run") {
                    QaplaButton::drawPlay(drawList, topLeft, size);
                }
                if (button == "Stop") {
                    QaplaButton::drawStop(drawList, topLeft, size);
				}
                if (button == "Clear") {
                    QaplaButton::drawText("X", drawList, topLeft, size);
                }
            }))
        {
            try {
                if (button == "Run") {
					tournamentData_->startTournament();
                    SnackbarManager::instance().showSuccess("Tournament started");
                } 
                else if (button == "Stop") {
			    } 
                else if (button == "Clear") {
                }
            } 
            catch (const std::exception& e) {
                SnackbarManager::instance().showError(std::string("Fehler: ") + e.what());
            }
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}

void TournamentWindow::drawInput() {
    
	constexpr float inputWidth = 300.0f;
    constexpr int maxConcurrency = 32;
    
    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::sliderInt<uint32_t>("Concurrency",
        tournamentData_->concurrency(), 1, maxConcurrency)) {
        tournamentData_->setPoolConcurrency(tournamentData_->concurrency(), true, true);
    }
    ImGui::Spacing();
    ImGuiControls::fileInput("Tournament file", tournamentData_->config().tournamentFilename, 2 * inputWidth);
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Opening", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
        ImGuiControls::fileInput("Opening file", tournamentData_->config().openings.file, 2 * inputWidth);
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::selectionBox("File format", tournamentData_->config().openings.format, { "epd", "raw", "pgn" });
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::selectionBox("Order", tournamentData_->config().openings.order, { "random", "sequential" });
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::inputInt<uint32_t>("First opening", tournamentData_->config().openings.start, 0, 1000);
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::inputInt<uint32_t>("Random seed", tournamentData_->config().openings.seed, 0, 1000);
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::selectionBox("Switch policy", tournamentData_->config().openings.policy, { "default", "encounter", "round" });
        ImGui::Unindent(10.0f);
    }
    if (ImGui::CollapsingHeader("Tournament", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
        ImGuiControls::inputText("Event", tournamentData_->config().event, inputWidth);
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::selectionBox("Type", tournamentData_->config().type, { "gauntlet", "round-robin" });
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::inputInt<uint32_t>("Rounds", tournamentData_->config().rounds, 1, 1000);
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::inputInt<uint32_t>("Games per pairing", tournamentData_->config().games, 1, 1000);
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::inputInt<uint32_t>("Same opening", tournamentData_->config().repeat, 1, 1000);
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::booleanInput("No color swap", tournamentData_->config().noSwap);
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::inputInt<int>("Average Elo", tournamentData_->config().averageElo, 1000, 5000);
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::inputInt<uint32_t>("Save interval (s)", tournamentData_->config().saveInterval, 0, 1000);
        ImGui::Unindent(10.0f);
    }
    if (ImGui::CollapsingHeader("Pgn", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
        ImGuiControls::fileInput("Pgn file", tournamentData_->pgnConfig().file, 2 * inputWidth);

        ImGui::SetNextItemWidth(inputWidth);
		std::string append = tournamentData_->pgnConfig().append ? "Append" : "Overwrite";
		ImGuiControls::selectionBox("Append mode", append, { "Append", "Overwrite" });
		tournamentData_->pgnConfig().append = (append == "Append");

		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::booleanInput("Save only finished games", tournamentData_->pgnConfig().onlyFinishedGames);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::booleanInput("Minimal tags", tournamentData_->pgnConfig().minimalTags);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::booleanInput("Save after each move", tournamentData_->pgnConfig().saveAfterMove);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::booleanInput("Include clock", tournamentData_->pgnConfig().includeClock);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::booleanInput("Include eval", tournamentData_->pgnConfig().includeEval);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::booleanInput("Include PV", tournamentData_->pgnConfig().includePv);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::booleanInput("Include depth", tournamentData_->pgnConfig().includeDepth);

        ImGui::Unindent(10.0f);
	}
    if (ImGui::CollapsingHeader("Adjudicate draw", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
        ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::inputInt<uint32_t>("Min full moves", tournamentData_->drawConfig().minFullMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::inputInt<uint32_t>("Required consecutive moves", tournamentData_->drawConfig().requiredConsecutiveMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::inputInt<int>("Centipawn threshold", tournamentData_->drawConfig().centipawnThreshold, -10000, 10000);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::booleanInput("Only test adjucation", tournamentData_->drawConfig().testOnly);
        ImGui::Unindent(10.0f);
	}
    if (ImGui::CollapsingHeader("Adjudicate resign", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::inputInt<uint32_t>("Required consecutive moves", tournamentData_->resignConfig().requiredConsecutiveMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::inputInt<int>("Centipawn threshold", tournamentData_->resignConfig().centipawnThreshold, -10000, 10000);
		ImGui::SetNextItemWidth(inputWidth);
		ImGuiControls::booleanInput("Both side decides", tournamentData_->resignConfig().twoSided);
		ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::booleanInput("Only test adjucation", tournamentData_->resignConfig().testOnly);
        ImGui::Unindent(10.0f);
	}
	
    ImGui::Spacing();
}

void TournamentWindow::draw() {
    drawButtons();
    drawInput();
    ImVec2 size = ImGui::GetContentRegionAvail();
    auto clickedRow = tournamentData_->drawTable(size);
}

