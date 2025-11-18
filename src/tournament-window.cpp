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
#include "tutorial.h"
#include "os-dialogs.h"
#include "imgui-controls.h"
#include "imgui-engine-global-settings.h"
#include "configuration.h"

#include "move-record.h"
#include "game-record.h"
#include "string-helper.h"
#include "engine-event.h"
#include "tournament.h"

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

static QaplaButton::ButtonState getButtonState(const std::string& button) {
    // Tutorial highlighting
    if (!TournamentWindow::highlightedButton_.empty() && button == TournamentWindow::highlightedButton_) {
        return QaplaButton::ButtonState::Highlighted;
    }
    
    if (button == "Run/Grace/Continue" && TournamentData::instance().state() == TournamentData::State::GracefulStopping) {
        return QaplaButton::ButtonState::Active;
    }
    
    if (button == "Run/Grace/Continue" && TournamentData::instance().isFinished()) {
        return QaplaButton::ButtonState::Disabled;
    }
    
    if ((button == "Stop") && !TournamentData::instance().isRunning()) {
        return QaplaButton::ButtonState::Disabled;
    }
    
    if (button == "Clear" && !TournamentData::instance().hasTasksScheduled()) {
        return QaplaButton::ButtonState::Disabled;
    }
    
    if ((button == "Load" || button == "Save As") && TournamentData::instance().isRunning()) {
        return QaplaButton::ButtonState::Disabled;
    }
    
    return QaplaButton::ButtonState::Normal;
}

std::string TournamentWindow::drawButtons() {
    constexpr float space = 3.0F;
    constexpr float topOffset = 5.0F;
    constexpr float bottomOffset = 8.0F;
    constexpr float leftOffset = 20.0F;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = { 25.0F, 25.0F };
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    
    std::string clickedButton = "";
    
    for (const std::string button : { "Run/Grace/Continue", "Stop", "Clear", "Load", "Save As" }) {
        ImGui::SetCursorScreenPos(pos);
        bool running = TournamentData::instance().isRunning();
        auto label = button;
        
        if (label == "Run/Grace/Continue") {
            label = running ? "Grace" : "Run";
            if (!running && TournamentData::instance().hasTasksScheduled()) {
                label = "Continue";
            }
        }

        auto state = getButtonState(button);

        if (QaplaButton::drawIconButton(button, label, buttonSize, state,
                [&button, running, state](ImDrawList *drawList, ImVec2 topLeft, ImVec2 size)->void {
                    drawSingleButton(drawList, topLeft, size, button, running, state);
                }))
        {
            clickedButton = button;
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
    return clickedButton;
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
                if (TournamentData::instance().isRunning())
                {
                    SnackbarManager::instance().showWarning("Cannot load tournament while running");
                    return;
                }
                auto selectedPath = OsDialogs::openFileDialog();
                if (!selectedPath.empty() && !selectedPath[0].empty())
                {
                    TournamentData::instance().loadTournament(selectedPath[0]);
                }
            } else if (button == "Save As")
            {
                if (TournamentData::instance().isRunning())
                {
                    SnackbarManager::instance().showWarning("Cannot save tournament while running");
                    return;
                }
                auto selectedPath = OsDialogs::saveFileDialog({ {"Qapla Tournament Files", "qtour"} });
                if (!selectedPath.empty())
                {
                    TournamentData::saveTournament(selectedPath);
                }
                showNextTournamentTutorialStep(button);
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

    globalSettingsTutorial_.highlight = (highlightedSection_ == "GlobalSettings");
    changed |= tournamentData.globalSettings().drawGlobalSettings(inputWidth, 10.0F, globalSettingsTutorial_);
    
    const bool highlightEngineSelect = (highlightedSection_ == "EngineSelect");
    changed |= tournamentData.engineSelect().draw(highlightEngineSelect);
    
    openingTutorial_.highlight = (highlightedSection_ == "Opening");
    changed |= tournamentData.tournamentOpening().draw(inputWidth, fileInputWidth, 10.0F, openingTutorial_);

    tournamentTutorial_.highlight = (highlightedSection_ == "Tournament");
    if (ImGuiControls::CollapsingHeaderWithDot("Tournament", ImGuiTreeNodeFlags_Selected, tournamentTutorial_.highlight)) {
        ImGui::PushID("tournament");
        ImGui::Indent(10.0F);
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputText("Event", tournamentData.config().event);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Optional event name for PGN or logging");
        }
        
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::selectionBox("Type", tournamentData.config().type, { "gauntlet", "round-robin" });
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Tournament type:\n"
                "  gauntlet - One engine plays against all others\n"
                "  round-robin - Every engine plays against every other engine"
            );
        }
        
        // Show tutorial annotation if present
        auto it = tournamentTutorial_.annotations.find("Type");
        if (it != tournamentTutorial_.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
        
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Rounds", tournamentData.config().rounds, 1, 1000);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Repeat all pairings this many times");
        }
        
        // Show tutorial annotation if present
        it = tournamentTutorial_.annotations.find("Rounds");
        if (it != tournamentTutorial_.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
        
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Games per pairing", tournamentData.config().games, 1, 1000);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Number of games per pairing.\nTotal games = games × rounds");
        }
        
        // Show tutorial annotation if present
        it = tournamentTutorial_.annotations.find("Games per pairing");
        if (it != tournamentTutorial_.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
        
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<uint32_t>("Same opening", tournamentData.config().repeat, 1, 1000);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Number of consecutive games played per opening.\n"
                "Commonly set to 2 to alternate colors with the same line"
            );
        }
        
        // Show tutorial annotation if present
        it = tournamentTutorial_.annotations.find("Same opening");
        if (it != tournamentTutorial_.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
        
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::booleanInput("No color swap", tournamentData.config().noSwap);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Disable automatic color swap after each game");
        }
        
        ImGui::SetNextItemWidth(inputWidth);
        changed |= ImGuiControls::inputInt<int>("Average Elo", tournamentData.config().averageElo, 1000, 5000);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Average Elo level for scaling rating output");
        }
        
        ImGui::Unindent(10.0F);
        ImGui::PopID();
    }
    
    timeControlTutorial_.highlight = (highlightedSection_ == "TimeControl");
    changed |= tournamentData.globalSettings().drawTimeControl(inputWidth, 10.0F, false, timeControlTutorial_);
    
    pgnTutorial_.highlight = (highlightedSection_ == "Pgn");
    changed |= tournamentData.tournamentPgn().draw(inputWidth, fileInputWidth, 10.0F, pgnTutorial_);
    
    changed |= tournamentData.tournamentAdjudication().draw(inputWidth, 10.0F);
	
    ImGui::Spacing();    return changed;
}

void TournamentWindow::drawProgress()
{
    auto& tournamentData = TournamentData::instance();
    auto totalGames = tournamentData.getTotalGames();
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
    auto clickedButton = drawButtons();
    
    if (!clickedButton.empty()) {
        executeCommand(clickedButton);
    }

    ImGui::Indent(10.0F);
    auto size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("InputArea", ImVec2(size.x - rightBorder, 0), ImGuiChildFlags_None);
    if (drawInput()) {
        tournamentData.updateConfiguration();
    }
    tournamentData.drawRunningTable(ImVec2(size.x, 800.0F));
    tournamentData.drawEloTable(ImVec2(size.x, 800.0F));
    tournamentData.drawCauseTable(ImVec2(size.x, 10000.0F));
    if (tournamentData.drawConfig().testOnly || tournamentData.resignConfig().testOnly) {
        tournamentData.drawAdjudicationTable(ImVec2(size.x, 200.0F));
    }

    ImGui::EndChild();
    ImGui::Unindent(10.0F);
    
    showNextTournamentTutorialStep(clickedButton);
}

bool TournamentWindow::highlighted() const {
    // Highlight tab when tutorial is waiting for user to open it
    return tutorialProgress_ == 1;
}

bool TournamentWindow::hasTwoSameEnginesWithPonder() {
    auto& configs = TournamentData::instance().engineSelect().getEngineConfigurations();
    
    // Check if we have at least 2 selected engines with same originalName
    size_t index = 0;
    for (const auto& config: configs) {
        if (config.selected) {
            auto it = std::ranges::find_if(configs.begin() + index + 1, configs.end(), 
                [&config](const ImGuiEngineSelect::EngineConfiguration& otherConfig) {
                    bool ponderCondition = config.config.isPonderEnabled() || otherConfig.config.isPonderEnabled();
                    return otherConfig.selected && (config.originalName == otherConfig.originalName) && 
                        ponderCondition;
                });
            
            if (it != configs.end()) {
                return true;
            }
        }
        index++;
    }
    
    return false;
}

void TournamentWindow::showNextTournamentTutorialStep([[maybe_unused]] const std::string& clickedButton) {
    auto topicName = Tutorial::TutorialName::Tournament;
    
    auto& tournamentData = TournamentData::instance();
    
    switch (tutorialProgress_) {
        case 1:
        // Step 0 (autoStart): Tutorial started, tab is highlighted
        // When draw() is called, the tab is open -> advance to next step
        Tutorial::instance().showNextTutorialStep(topicName);
        highlightedButton_ = "";
        highlightedSection_ = "GlobalSettings";
        globalSettingsTutorial_.highlight = true;
        globalSettingsTutorial_.annotations["Hash (MB)"] = "Set to: 64";
        globalSettingsTutorial_.annotations["Ponder"] = "Uncheck 'Engine decides'";
        return;
        
        case 2:
        // Step 1: Configure global settings
        // Check if hash is 64 MB and global ponder is disabled
        if (tournamentData.globalSettings().getGlobalSettings().hashSizeMB == 64 && 
            !tournamentData.globalSettings().getGlobalSettings().useGlobalPonder) {
            Tutorial::instance().showNextTutorialStep(topicName);
            globalSettingsTutorial_.clear();
            highlightedSection_ = "EngineSelect";
        }
        return;
        
        case 3:
        // Step 2: Select two engines with the same originalName and ponder enabled
        if (hasTwoSameEnginesWithPonder()) {
            Tutorial::instance().showNextTutorialStep(topicName);
            highlightedSection_ = "Opening";
            openingTutorial_.highlight = true;
            openingTutorial_.annotations["Opening file"] = "Select any opening file";
        }
        return;
        
        case 4:
        // Step 3: Configure opening file
        // Check if opening file is set (ignore format check as requested)
        if (!tournamentData.tournamentOpening().openings().file.empty()) {
            Tutorial::instance().showNextTutorialStep(topicName);
            openingTutorial_.clear();
            highlightedSection_ = "Tournament";
            tournamentTutorial_.highlight = true;
            tournamentTutorial_.annotations["Type"] = "Set to: round-robin";
            tournamentTutorial_.annotations["Rounds"] = "Set to: 2";
            tournamentTutorial_.annotations["Games per pairing"] = "Set to: 2";
            tournamentTutorial_.annotations["Same opening"] = "Set to: 2";
        }
        return;
        
        case 5:
        // Step 4: Configure tournament settings
        // Check: type=round-robin, rounds=2, games=2, repeat=2
        if (tournamentData.config().type == "round-robin" &&
            tournamentData.config().rounds == 2 &&
            tournamentData.config().games == 2 &&
            tournamentData.config().repeat == 2) {
            Tutorial::instance().showNextTutorialStep(topicName);
            tournamentTutorial_.clear();
            highlightedSection_ = "TimeControl";
            timeControlTutorial_.highlight = true;
            timeControlTutorial_.annotations["Predefined time control"] = "Select: 20.0+0.02";
        }
        return;
        
        case 6:
        // Step 5: Configure time control
        // Check if time control is set to "20.0+0.02"
        if (tournamentData.globalSettings().getTimeControlSettings().timeControl == "20.0+0.02") {
            Tutorial::instance().showNextTutorialStep(topicName);
            timeControlTutorial_.clear();
            highlightedSection_ = "Pgn";
            pgnTutorial_.highlight = true;
            pgnTutorial_.annotations["Pgn file"] = "Select output file";
        }
        return;
        
        case 7:
        // Step 6: Set PGN output file
        if (!tournamentData.tournamentPgn().pgnOptions().file.empty()) {
            Tutorial::instance().showNextTutorialStep(topicName);
            pgnTutorial_.clear();
            highlightedSection_ = "";
        }
        return;
        
        case 8:
        // Step 7: Set concurrency to 4
        if (tournamentData.concurrency() == 4) {
            Tutorial::instance().showNextTutorialStep(topicName);
            highlightedButton_ = "Run/Grace/Continue";
        }
        return;
        
        case 9:
        // Step 8: Start tournament - check if running
        if (tournamentData.isRunning()) {
            Tutorial::instance().showNextTutorialStep(topicName);
            highlightedButton_ = "";
        }
        return;
        
        case 10:
        // Step 9: Wait for tournament to finish - check if NOT running and has results
        if (!tournamentData.isRunning() && tournamentData.hasTasksScheduled()) {
            Tutorial::instance().showNextTutorialStep(topicName);
            highlightedButton_ = "Save As";
        }
        return;
        
        case 11:
        // Step 10: Save tournament - advance when "Save As" button is clicked
        if (clickedButton == "Save As") {
            Tutorial::instance().showNextTutorialStep(topicName);
            highlightedButton_ = "";
        }
        return;
        
        case 12:
        // Step 11: Add third engine - check if at least 3 engines are selected
        {
            auto& configs = tournamentData.engineSelect().getEngineConfigurations();
            int selectedCount = 0;
            for (const auto& engineConfig : configs) {
                if (engineConfig.selected) {
                    selectedCount++;
                }
            }
            if (selectedCount >= 3) {
                Tutorial::instance().showNextTutorialStep(topicName);
                highlightedButton_ = "Run/Grace/Continue";
            }
        }
        return;
        
        case 13:
        // Step 12: Continue tournament - check if running
        if (tournamentData.isRunning()) {
            Tutorial::instance().showNextTutorialStep(topicName);
            highlightedButton_ = "";
        }
        return;
        
        case 14:
        // Step 13: Final step - tournament running or finished
        Tutorial::instance().showNextTutorialStep(topicName);
        return;
        
        case 15:
        if (!SnackbarManager::instance().isTutorialMessageVisible()) {
            highlightedButton_ = "";
            highlightedSection_ = "";
            globalSettingsTutorial_.clear();
            openingTutorial_.clear();
            tournamentTutorial_.clear();
            timeControlTutorial_.clear();
            pgnTutorial_.clear();
            Tutorial::instance().finishTutorial(topicName);
        }
        return;
                                
        default:
        return;
    }
}

static auto tournamentWindowTutorialInit = []() {
    Tutorial::instance().setEntry({
        .name = Tutorial::TutorialName::Tournament,
        .displayName = "Tournament",
        .messages = {
            { "Tournament - Step 1\n\n"
              "Start engine tournaments here.\n"
              "Compare multiple engines in round-robin matches.\n\n"
              "Click the 'Tournament' tab to open.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 2\n\n"
              "Configure global engine settings.\n"
              "Set Hash to 64 MB.\n"
              "Disable 'Engine decides' for Ponder.\n\n"
              "This ensures consistent memory usage.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 3\n\n"
              "Select engines for the tournament.\n"
              "Choose the same engine twice.\n"
              "Enable 'Ponder' for one of them.\n\n"
              "Names will auto-disambiguate with [ponder].",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 4\n\n"
              "Configure opening positions.\n"
              "Select an opening file (.epd, .pgn, or raw FEN).\n"
              "Choose the appropriate file format.\n\n"
              "Hover over options for detailed tooltips.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 5\n\n"
              "Configure tournament settings.\n"
              "Type: round-robin, Rounds: 2\n"
              "Games per pairing: 2, Same opening: 2\n\n"
              "Hover over options for explanations.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 6\n\n"
              "Select time control.\n"
              "Choose '20.0+0.02' from predefined options.\n"
              "(20 seconds + 20 milliseconds per move)\n\n"
              "Hover over fields for format help.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 7\n\n"
              "Set PGN output file.\n"
              "All games will be saved to this file.\n\n"
              "Hover over options for detailed tooltips.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 8\n\n"
              "Set Concurrency to 4.\n"
              "This runs 4 games in parallel.\n\n"
              "You'll see 4 board tabs during the tournament.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 9\n\n"
              "Start your first tournament!\n"
              "Click 'Run' to begin.\n\n"
              "Input controls hide while running.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 10\n\n"
              "Tournament is running!\n"
              "Click a board tab to watch games.\n\n"
              "Wait for the tournament to finish...",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 11\n\n"
              "Tournament finished!\n"
              "Results are auto-saved.\n\n"
              "Click 'Save As' to save manually.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 12\n\n"
              "Save dialog completed!\n\n"
              "Now let's extend the tournament.\n"
              "Add a third engine to the selection.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 13\n\n"
              "Third engine added!\n"
              "Tournament will include new pairings.\n\n"
              "Click 'Continue' to resume.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament - Step 14\n\n"
              "Extended tournament is running!\n\n"
              "Let it finish or click 'Stop' to pause.",
              SnackbarManager::SnackbarType::Note },
            { "Tournament Tutorial Complete!\n\n"
              "You've mastered tournament basics!\n\n"
              "• Configure engines and settings\n"
              "• Run tournaments with concurrency\n"
              "• Save and extend tournaments",
              SnackbarManager::SnackbarType::Success }
        },
        .getProgressCounter = []() -> uint32_t& {
            return TournamentWindow::tutorialProgress_;
        },
        .autoStart = true
    });
    return true;
}();

