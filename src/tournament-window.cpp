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


TournamentWindow::TournamentWindow()
    : engineSelect_(std::make_unique<ImGuiEngineSelect>())
    , globalSettings_(std::make_unique<ImGuiEngineGlobalSettings>())
{
    setEngineConfiguration();
    setGlobalSettingsConfiguration();
    
    ImGuiEngineSelect::Options options;
    options.allowGauntletEdit = true;
    options.allowPonderEdit = true;
    options.allowTimeControlEdit = true;
    options.allowTraceLevelEdit = true;
    options.allowRestartOptionEdit = true;
    options.allowMultipleSelection = true;
    engineSelect_->setOptions(options);
    
    // Set up callbacks to synchronize global settings with tournament data
    globalSettings_->setConfigurationChangedCallback(
        [](const ImGuiEngineGlobalSettings::GlobalSettings& settings) {
            auto& tournamentData = TournamentData::instance();
            
            // Sync hash
            if (settings.useGlobalHash) {
                tournamentData.eachEngineConfig().hash = settings.hashSizeMB;
            }
            
            // Sync restart
            if (settings.useGlobalRestart) {
                tournamentData.eachEngineConfig().restart = settings.restart;
            }
            
            // Sync trace
            if (settings.useGlobalTrace) {
                tournamentData.eachEngineConfig().traceLevel = settings.traceLevel;
            }
            
            // Sync ponder
            if (settings.useGlobalPonder) {
                tournamentData.eachEngineConfig().ponder = settings.ponder ? "on" : "off";
            }
        }
    );
    
    globalSettings_->setTimeControlChangedCallback(
        [](const ImGuiEngineGlobalSettings::TimeControlSettings& settings) {
            auto& tournamentData = TournamentData::instance();
            tournamentData.eachEngineConfig().tc = settings.timeControl;
        }
    );

    engineSelect_->setConfigurationChangedCallback(
        [](const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
            TournamentData::instance().setEngineConfigurations(configurations);
        }
    );
   
}

TournamentWindow::~TournamentWindow() = default;

void TournamentWindow::setEngineConfigurationCallback(ImGuiEngineSelect::ConfigurationChangedCallback callback) {
    engineSelect_->setConfigurationChangedCallback(std::move(callback));
}

void TournamentWindow::setEngineConfiguration() {
    auto sections = QaplaConfiguration::Configuration::instance().
            getConfigData().getSectionList("engineselection", "tournament").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    engineSelect_->setId("tournament");
    engineSelect_->setEngineConfiguration(sections);
}

void TournamentWindow::setGlobalSettingsConfiguration() {
    auto& config = QaplaConfiguration::Configuration::instance();
    globalSettings_->setId("tournament");
    // Load global engine settings
    auto globalSections = config.getConfigData().getSectionList("eachengine", "tournament")
        .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    globalSettings_->setGlobalConfiguration(globalSections);
    
    // Load time control settings
    auto timeControlSections = config.getConfigData().getSectionList("timecontroloptions", "tournament")
        .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    globalSettings_->setTimeControlConfiguration(timeControlSections);
}

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
    
    ImGui::Indent(10.0F);
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiControls::sliderInt<uint32_t>("Concurrency", tournamentData.concurrency(), 1, maxConcurrency);
    tournamentData.setPoolConcurrency(tournamentData.concurrency(), true);
    ImGui::Unindent(10.0F);

    ImGui::Spacing();
    if (tournamentData.isRunning()) {
		ImGui::Indent(10.0F);
        ImGui::Text("Tournament is running");
        ImGui::Unindent(10.0F);
        return false;
    }

    bool changed = false;
    ImGui::Indent(10.0F);

    if (ImGui::CollapsingHeader("Engines", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("engineSettings");
        ImGui::Indent(10.0F);
        changed |= engineSelect_->draw();
        ImGui::Unindent(10.0F);
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Opening", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("opening");
        ImGui::Indent(10.0F);
        changed |= ImGuiControls::existingFileInput("Opening file", tournamentData.config().openings.file, fileInputWidth);
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
        ImGui::Unindent(10.0F);
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
    changed |= globalSettings_->drawTimeControl(inputWidth, 10.0F, false);
    changed |= globalSettings_->drawGlobalSettings(inputWidth, 10.0F);
    
    if (ImGui::CollapsingHeader("Pgn", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("pgn");
        ImGui::Indent(10.0F);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::newFileInput("Pgn file", tournamentData.pgnConfig().file, 
            {{"PGN files (*.pgn)", "pgn"}}, fileInputWidth);

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
        ImGui::Unindent(10.0F);
        ImGui::PopID();
	}
    if (ImGui::CollapsingHeader("Adjudicate draw", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("drawAdjudication");
        ImGui::Indent(10.0F);
        ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<uint32_t>("Min full moves", tournamentData.drawConfig().minFullMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<uint32_t>("Required consecutive moves", tournamentData.drawConfig().requiredConsecutiveMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<int>("Centipawn threshold", tournamentData.drawConfig().centipawnThreshold, -10000, 10000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Test adjudication only", tournamentData.drawConfig().testOnly);
        ImGui::Unindent(10.0F);
        ImGui::PopID();
	}
    if (ImGui::CollapsingHeader("Adjudicate resign", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("resignAdjudication");
        ImGui::Indent(10.0F);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<uint32_t>("Required consecutive moves", tournamentData.resignConfig().requiredConsecutiveMoves, 0, 1000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::inputInt<int>("Centipawn threshold", tournamentData.resignConfig().centipawnThreshold, -10000, 10000);
		ImGui::SetNextItemWidth(inputWidth);
		changed |= ImGuiControls::booleanInput("Both side decides", tournamentData.resignConfig().twoSided);
		ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("Test adjudication only", tournamentData.resignConfig().testOnly);
        ImGui::Unindent(10.0F);
        ImGui::PopID();
	}
	
    ImGui::Spacing();
    ImGui::Unindent(10.0F);

    return changed;
}

void TournamentWindow::draw() {
    drawButtons();
    if (drawInput()) {
        TournamentData::instance().updateConfiguration();
    }
    ImVec2 size = ImGui::GetContentRegionAvail();
    /* auto clickedRow = */
    ImGui::Indent(10.0F);
    TournamentData::instance().drawRunningTable(ImVec2(size.x, 600.0F));
    TournamentData::instance().drawEloTable(ImVec2(size.x, 400.0F));
    TournamentData::instance().drawCauseTable(ImVec2(size.x, 400.0F));
    ImGui::Unindent(10.0F);
}

