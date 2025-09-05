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
#include "configuration.h"

#include "qapla-tester/move-record.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/engine-event.h"
#include "qapla-tester/tournament.h"
#include "qapla-tester/engine-worker-factory.h"

#include "imgui.h"

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;


TournamentWindow::TournamentWindow()
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
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Clear");
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    for (const std::string button : { "Run", "Stop", "Clear" }) {
        ImGui::SetCursorScreenPos(pos);
        bool running = TournamentData::instance().isRunning();
        auto label = button == "Run" && running ? "Grace" : button;
        if (QaplaButton::drawIconButton(button, label, buttonSize, false,
            [&button, running](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
                if (button == "Run") {
                    if (running) {
                        QaplaButton::drawGrace(drawList, topLeft, size);
                    } else {
                        QaplaButton::drawPlay(drawList, topLeft, size);
                    }
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
                    bool running = TournamentData::instance().isRunning();
                    if (running) {
                        TournamentData::instance().stopPool(true);
                    } else {
                        TournamentData::instance().startTournament();
                    }
                } 
                else if (button == "Stop") {
                    TournamentData::instance().stopPool();
			    } 
                else if (button == "Clear") {
                    TournamentData::instance().clear();
                }
            } 
            catch (const std::exception& e) {
                SnackbarManager::instance().showError(e.what());
            }
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}

bool TournamentWindow::drawEngineList() {
    auto& configManager = EngineWorkerFactory::getConfigManagerMutable();
    auto configs = configManager.getAllConfigs();
    bool modified = false;
    int index = 0;
    for (auto& config : configs) {
        TournamentData::TournamentEngineConfig engine = {
            .config = config,
            .selected = false
        };
        auto& activeEngines = TournamentData::instance().getEngineConfigs();
        auto it = std::find_if(activeEngines.begin(), activeEngines.end(),
            [&config](const TournamentData::TournamentEngineConfig& engine) {
                return engine.config == config;
            });
        if (it != activeEngines.end()) {
            engine = *it;
        }
        auto& name = config.getName().empty() ? "(unnamed)" : config.getName();
        ImGui::PushID(index);
        ImGuiTreeNodeFlags flags = engine.selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_Leaf;
        bool changed = ImGuiControls::collapsingSelection(name, engine.selected, flags, [&engine]() -> bool {
            return ImGuiControls::checkbox("Gauntlet", engine.config.gauntlet());
        });
        if (changed) {
            modified = true;
            if (it == activeEngines.end()) {
                activeEngines.push_back(engine);
            } 
            else {
                it->selected = engine.selected;
                it->config.setGauntlet(engine.config.isGauntlet());
            }
        }
        ImGui::PopID();
        index++;
    }
    return modified;
}

bool TournamentWindow::drawInput() {
    
	constexpr float inputWidth = 300.0f;
    constexpr int maxConcurrency = 32;
	auto& tournamentData = TournamentData::instance();
    
    ImGui::Indent(10.0f);
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiControls::sliderInt<uint32_t>("Concurrency", tournamentData.concurrency(), 1, maxConcurrency);
    tournamentData.setPoolConcurrency(tournamentData.concurrency(), true);
    ImGui::Unindent(10.0f);

    ImGui::Spacing();
    if (tournamentData.isRunning()) {
		ImGui::Indent(10.0f);
        ImGui::Text("Tournament is running");
        ImGui::Unindent(10.0f);
        return false;
    }

    bool changed = false;
    ImGui::Indent(10.0f);

    ImGuiControls::fileInput("Tournament file", tournamentData.config().tournamentFilename, 2 * inputWidth);
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Engines", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
        changed |= drawEngineList();
        ImGui::Unindent(10.0f);
    }
    if (ImGui::CollapsingHeader("Opening", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
        changed |= ImGuiControls::fileInput("Opening file", tournamentData.config().openings.file, 2 * inputWidth);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::selectionBox("File format", tournamentData.config().openings.format, { "epd", "raw", "pgn" });
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::selectionBox("Order", tournamentData.config().openings.order, { "random", "sequential" });

		auto xPos = ImGui::GetCursorPosX();
        changed |= ImGuiControls::optionalInput<int>(
            "Set plies",
            tournamentData.config().openings.plies,
            [&](int& plies) {
                ImGui::SetCursorPosX(xPos + 100);
                ImGui::SetNextItemWidth(inputWidth - 100);
                return ImGuiControls::inputInt<int>("Plies", plies, 0, 1000);
            }
        );
       
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("First opening", tournamentData.config().openings.start, 0, 1000);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Random seed", tournamentData.config().openings.seed, 0, 1000);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::selectionBox("Switch policy", tournamentData.config().openings.policy, { "default", "encounter", "round" });
        ImGui::Unindent(10.0f);
    }
    if (ImGui::CollapsingHeader("Tournament", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputText("Event", tournamentData.config().event).has_value();
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
        ImGui::Unindent(10.0f);
    }
    if (ImGui::CollapsingHeader("Time Control", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);

        ImGuiControls::timeControlInput(tournamentData.eachEngineConfig().tc, false, inputWidth);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::selectionBox("Predefined time control", tournamentData.eachEngineConfig().tc, {
            "Custom", "10.0+0.02", "20.0+0.02", "50.0+0.10", "60.0+0.20"
            });
        ImGui::Unindent(10.0f);
    }
    if (ImGui::CollapsingHeader("Engine settings", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
        ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<uint32_t>("Hash (MB)", tournamentData.eachEngineConfig().hash, 1, 10000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::selectionBox("Restart", tournamentData.eachEngineConfig().restart,
            {"auto", "on", "off"});
        ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::selectionBox("Trace", tournamentData.eachEngineConfig().traceLevel,
            { "none", "all", "command" });
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Ponder", tournamentData.eachEngineConfig().ponder);

        ImGui::Unindent(10.0f);
	}
    if (ImGui::CollapsingHeader("Pgn", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
        changed |= ImGuiControls::fileInput("Pgn file", tournamentData.pgnConfig().file, 2 * inputWidth);

        ImGui::SetNextItemWidth(inputWidth);
		std::string append = tournamentData.pgnConfig().append ? "Append" : "Overwrite";
		changed |= ImGuiControls::selectionBox("Append mode", append, { "Append", "Overwrite" });
		tournamentData.pgnConfig().append = (append == "Append");

		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Save only finished games", tournamentData.pgnConfig().onlyFinishedGames);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Minimal tags", tournamentData.pgnConfig().minimalTags);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Save after each move", tournamentData.pgnConfig().saveAfterMove);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Include clock", tournamentData.pgnConfig().includeClock);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Include eval", tournamentData.pgnConfig().includeEval);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Include PV", tournamentData.pgnConfig().includePv);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Include depth", tournamentData.pgnConfig().includeDepth);

        ImGui::Unindent(10.0f);
	}
    if (ImGui::CollapsingHeader("Adjudicate draw", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
        ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<uint32_t>("Min full moves", tournamentData.drawConfig().minFullMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<uint32_t>("Required consecutive moves", tournamentData.drawConfig().requiredConsecutiveMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<int>("Centipawn threshold", tournamentData.drawConfig().centipawnThreshold, -10000, 10000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Only test adjucation", tournamentData.drawConfig().testOnly);
        ImGui::Unindent(10.0f);
	}
    if (ImGui::CollapsingHeader("Adjudicate resign", ImGuiTreeNodeFlags_Selected)) {
        ImGui::Indent(10.0f);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<uint32_t>("Required consecutive moves", tournamentData.resignConfig().requiredConsecutiveMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<int>("Centipawn threshold", tournamentData.resignConfig().centipawnThreshold, -10000, 10000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Both side decides", tournamentData.resignConfig().twoSided);
		ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Only test adjucation", tournamentData.resignConfig().testOnly);
        ImGui::Unindent(10.0f);
	}
	
    ImGui::Spacing();
    ImGui::Unindent(10.0f);

    return changed;
}

void TournamentWindow::draw() {
    drawButtons();
    if (drawInput()) {
        QaplaConfiguration::Configuration::instance().setModified();
    }
    ImVec2 size = ImGui::GetContentRegionAvail();
	constexpr float heightRatio = 0.4f;
    /* auto clickedRow = */
    ImGui::Indent(10.0f);
    TournamentData::instance().drawEloTable(ImVec2(size.x, size.y * heightRatio));
    TournamentData::instance().drawRunningTable(ImGui::GetContentRegionAvail());
    ImGui::Unindent(10.0f);
}

