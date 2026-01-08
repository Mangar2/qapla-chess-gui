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

#include <chess-game/move-record.h>
#include <chess-game/game-record.h>
#include <base-elements/string-helper.h>
#include <chess-game/engine-event.h>
#include <tournament/tournament.h>

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
    if (button == "RunGraceContinue")
    {
        if (running)
        {
            QaplaButton::drawGrace(drawList, topLeft, size, state);
            ImGuiControls::hooverTooltip("Stop tournament gracefully after current games finish");
        }
        else
        {
            QaplaButton::drawPlay(drawList, topLeft, size, state);
            ImGuiControls::hooverTooltip(TournamentData::instance().hasTasksScheduled() 
                ? "Continue tournament with current configuration" 
                : "Start new tournament with current configuration");
        }
    }
    if (button == "Stop")
    {
        QaplaButton::drawStop(drawList, topLeft, size, state);
        ImGuiControls::hooverTooltip("Stop tournament immediately, aborting running games");
    }
    if (button == "Clear")
    {
        QaplaButton::drawClear(drawList, topLeft, size, state);
        ImGuiControls::hooverTooltip("Clear all tournament data and results");
    }
    if (button == "Load")
    {
        QaplaButton::drawOpen(drawList, topLeft, size, state);
        ImGuiControls::hooverTooltip("Load tournament configuration and results from file");
    }
    if (button == "Save As")
    {
        QaplaButton::drawSave(drawList, topLeft, size, state);
        ImGuiControls::hooverTooltip("Save tournament configuration and results to file");
    }
}

static QaplaButton::ButtonState getButtonState(const std::string& button) {
    // Tutorial highlighting
    if (!TournamentWindow::highlightedButton_.empty() && button == TournamentWindow::highlightedButton_) {
        return QaplaButton::ButtonState::Highlighted;
    }
    auto state = TournamentData::instance().getState();
    
    if (button == "RunGraceContinue" && state == TournamentData::State::GracefulStopping) {
        return QaplaButton::ButtonState::Active;
    }
    
    if (button == "RunGraceContinue" && TournamentData::instance().isFinished()) {
        return QaplaButton::ButtonState::Disabled;
    }

    if (button == "RunGraceContinue" && state == TournamentData::State::Stopping) {
        return QaplaButton::ButtonState::Disabled;
    }
    
    if ((button == "Stop") && !TournamentData::instance().isRunning()) {
        return QaplaButton::ButtonState::Disabled;
    }

    if ((button == "Stop") && state == TournamentData::State::Stopping) {
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
    const auto totalSize = QaplaButton::calcIconButtonsTotalSize(buttonSize, 
        { "Continue", "Stop", "Clear", "Load", "Save As" });
        
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    
    std::string clickedButton;
    
    for (const std::string button : { "RunGraceContinue", "Stop", "Clear", "Load", "Save As" }) {
        ImGui::SetCursorScreenPos(pos);
        bool running = TournamentData::instance().isRunning();
        auto label = button;
        
        if (label == "RunGraceContinue") {
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
            if (button == "RunGraceContinue")
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
                auto selectedPath = OsDialogs::openFileDialog(false, 
                    { {"Qapla Tournament Files", "*.qtour"}, {"All Files", "*.*"} });
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
    auto concurrency = tournamentData.getExternalConcurrency();
    ImGuiControls::sliderInt<uint32_t>("Concurrency", concurrency, 1, maxConcurrency);
    ImGuiControls::hooverTooltip("Number of games running in parallel");
    tournamentData.setExternalConcurrency(concurrency);
    tournamentData.setPoolConcurrency(concurrency, true);
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
    changed |= tournamentData.getGlobalSettings().drawGlobalSettings(
        { .controlWidth = inputWidth, .controlIndent = 10.0F }, {}, globalSettingsTutorial_);
    
    const bool highlightEngineSelect = (highlightedSection_ == "EngineSelect");
    changed |= tournamentData.getEngineSelect().draw(highlightEngineSelect);
    
    openingTutorial_.highlight = (highlightedSection_ == "Opening");
    changed |= tournamentData.tournamentOpening().draw(
        { .inputWidth = inputWidth, .fileInputWidth = fileInputWidth, .indent = 10.0F }, openingTutorial_);

    tournamentTutorial_.highlight = (highlightedSection_ == "Tournament");
    changed |= tournamentData.tournamentConfiguration().draw({}, inputWidth, 10.0F, tournamentTutorial_);
    
    timeControlTutorial_.highlight = (highlightedSection_ == "TimeControl");
    changed |= tournamentData.getGlobalSettings().drawTimeControl(
        { .controlWidth = inputWidth, .controlIndent = 10.0F }, false, false, timeControlTutorial_);
    
    pgnTutorial_.highlight = (highlightedSection_ == "Pgn");
    changed |= tournamentData.tournamentPgn().draw(
        { .inputWidth = inputWidth, .fileInputWidth = fileInputWidth, .indent = 10.0F }, 
        pgnTutorial_);
    
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
    ImGuiControls::hooverTooltip("Tournament progress: games played / total games");
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
    drawInput();
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
    const auto& configs = TournamentData::instance().getEngineSelect().getEngineConfigurations();
    
    // Check if we have at least 2 selected engines with same originalName
    long long index = 0;
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

void TournamentWindow::clearTournamentTutorialState() {
    globalSettingsTutorial_.highlight = false;
    tutorialProgress_ = 0;
    highlightedButton_.clear();
    highlightedSection_.clear();
    globalSettingsTutorial_.clear();
    openingTutorial_.clear();
    tournamentTutorial_.clear();
    timeControlTutorial_.clear();
    pgnTutorial_.clear();
}

void TournamentWindow::applyHighlighting(const HighlightInfo& info) {
    if (Tutorial::instance().doWaitForUserInput()) {
        // Clear all highlights when waiting for user input
        highlightedButton_.clear();
        highlightedSection_.clear();
        globalSettingsTutorial_.clear();
        openingTutorial_.clear();
        tournamentTutorial_.clear();
        timeControlTutorial_.clear();
        pgnTutorial_.clear();
    } else {
        // Set highlights from info
        highlightedButton_ = info.highlightedButton;
        highlightedSection_ = info.highlightedSection;
        globalSettingsTutorial_ = info.globalSettingsTutorial;
        openingTutorial_ = info.openingTutorial;
        tournamentTutorial_ = info.tournamentTutorial;
        timeControlTutorial_ = info.timeControlTutorial;
        pgnTutorial_ = info.pgnTutorial;
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void TournamentWindow::showNextTournamentTutorialStep([[maybe_unused]] const std::string& clickedButton) {
    auto topicName = Tutorial::TutorialName::Tournament;
    
    auto& tournamentData = TournamentData::instance();
    
    switch (tutorialProgress_) {
        case 0:
        clearTournamentTutorialState();
        return;
        case 1:
        // Tutorial started, tab is highlighted
        // When draw() is called, the tab is open -> advance to next step
        Tutorial::instance().requestNextTutorialStep(topicName);
        return;
        
        case 2:
        applyHighlighting({
            .highlightedSection = "GlobalSettings",
            .globalSettingsTutorial = {
                .highlight = true,
                .annotations = {
                    {"Hash (MB)", "Set to: 64"},
                    {"Ponder", "Uncheck 'Enable global pondering'"}
                }
            }
        });
        
        // Configure global settings
        // Check if hash is 64 MB and global ponder is disabled
        if (tournamentData.getGlobalSettings().getGlobalConfiguration().hashSizeMB == 64 && 
            !tournamentData.getGlobalSettings().getGlobalConfiguration().useGlobalPonder) {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 3:
        applyHighlighting({.highlightedSection = "EngineSelect"});
        
        // Select two engines with the same originalName and ponder enabled
        if (hasTwoSameEnginesWithPonder()) {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 4:
        applyHighlighting({
            .highlightedSection = "Opening",
            .openingTutorial = {
                .highlight = true,
                .annotations = {
                    {"Opening file", "Select any opening file"}
                }
            }
        });
        
        // Configure opening file
        // Check if opening file is set (ignore format check as requested)
        if (!tournamentData.tournamentOpening().openings().file.empty()) {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 5:
        applyHighlighting({
            .highlightedSection = "Tournament",
            .tournamentTutorial = {
                .highlight = true,
                .annotations = {
                    {"Type", "Set to: round-robin"},
                    {"Rounds", "Set to: 2"},
                    {"Games per pairing", "Set to: 2"},
                    {"Same opening", "Set to: 2"}
                }
            }
        });
        
        // Configure tournament settings
        // Check: type=round-robin, rounds=2, games=2, repeat=2
        if (tournamentData.config().type == "round-robin" &&
            tournamentData.config().rounds == 2 &&
            tournamentData.config().games == 2 &&
            tournamentData.config().repeat == 2) {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 6:
        applyHighlighting({
            .highlightedSection = "TimeControl",
            .timeControlTutorial = {
                .highlight = true,
                .annotations = {
                    {"Predefined time control", "Select: 20.0+0.02"}
                }
            }
        });
        
        // Configure time control
        // Check if time control is set to "20.0+0.02"
        if (tournamentData.getGlobalSettings().getTimeControlSettings().timeControl == "20.0+0.02") {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 7:
        applyHighlighting({
            .highlightedSection = "Pgn",
            .pgnTutorial = {
                .highlight = true,
                .annotations = {
                    {"Pgn file", "Select output file"}
                }
            }
        });
        
        // Set PGN output file
        if (!tournamentData.tournamentPgn().pgnOptions().file.empty()) {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 8:
        applyHighlighting({});
        
        // Set concurrency to 4
        if (tournamentData.getExternalConcurrency() == 4) {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 9:
        applyHighlighting({.highlightedButton = "RunGraceContinue"});
        
        // Start tournament - check if running
        if (tournamentData.isRunning()) {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 10:
        applyHighlighting({});
        
        // Wait for tournament to finish - check if NOT running and has tasks scheduled
        if (!tournamentData.isRunning() && tournamentData.hasTasksScheduled()) {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 11:
        applyHighlighting({.highlightedButton = "Save As"});
        
        // Save tournament - advance when "Save As" button is clicked
        if (clickedButton == "Save As") {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 12:
        applyHighlighting({.highlightedSection = "EngineSelect"});
        
        // Add third engine - check if at least 3 engines are selected
        {
            const auto& configs = tournamentData.getEngineSelect().getEngineConfigurations();
            int selectedCount = 0;
            for (const auto& engineConfig : configs) {
                if (engineConfig.selected) {
                    selectedCount++;
                }
            }
            if (selectedCount >= 3) {
                Tutorial::instance().requestNextTutorialStep(topicName);
            }
        }
        return;
        
        case 13:
        applyHighlighting({.highlightedButton = "RunGraceContinue"});
        
        // Continue tournament - check if running
        if (tournamentData.isRunning()) {
            Tutorial::instance().requestNextTutorialStep(topicName);
        }
        return;
        
        case 14:
        applyHighlighting({});
        
        // Final step - tournament running or finished
        Tutorial::instance().requestNextTutorialStep(topicName);
        return;
        
        case 15:
        applyHighlighting({});
        
        if (!SnackbarManager::instance().isTutorialMessageVisible()) {
            clearTournamentTutorialState();
            Tutorial::instance().finishTutorial(topicName);
        }
        return;
                                
        default:
        applyHighlighting({});
        clearTournamentTutorialState();
        return;
    }
}

static auto tournamentWindowTutorialInit = []() {
    QaplaWindows::Tutorial::instance().setEntry({
        .name = QaplaWindows::Tutorial::TutorialName::Tournament,
        .displayName = "Tournament",
        .messages = {
            { .text = "Welcome to the Tournament Tutorial!\n\n"
              "This tutorial will guide you through setting up and running engine tournaments. "
              "You can compare multiple chess engines against each other in round-robin and gauntlet matches.\n\n"
              "Tip: If you prefer a simpler approach, you can also configure and start tournaments "
              "through the Chatbot - just select 'Tournament' from the Chatbot menu. "
              "This tutorial covers the expert mode with direct access to all settings.\n\n"
              "To help you through the process, we'll mark the relevant sections and buttons with a red dot as we go along.\n"
              "Hover over the options for detailed tooltips explaining each setting.\n\n"
              "Let's begin! Click on the 'Tournament' tab in the left window to open the tournament configuration.",
              .success = "Great! You've opened the Tournament tab.",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "Now we'll configure the global engine settings. These settings apply to all engines "
              "in the tournament and ensure fair, consistent conditions.\n\n"
              "Settings can be set for all engines or individually per engine. "
              "We change Hash size globally and pondering per engine:\n"
              "• Leave the checkbox near Hash checked and set 'Hash (MB)' to 64\n"
              "• Uncheck the checkbox next to Ponder - this lets us control pondering per engine later",
              .success = "Global settings are configured.",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "Now let's select the engines for our tournament. We'll demonstrate specific engine settings: "
              "compare the same engine against itself - once with pondering enabled and once without.\n\n"
              "In the 'Engine Selection' section, "
              "select the same engine twice (click the '+' button left of 'Available Engine' twice)\n"
              "Now you see the same engine twice in the 'Selected Engines' list. "
              "Expand the first one and set the check mark for 'Ponder'\n"
              "The names will automatically get a '[ponder]' suffix to distinguish them. "
              "Now you can see how pondering affects performance of the engine!",
              .success = "Engines are selected!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "Now we need opening positions. Without openings, every game would start from the "
              "standard chess position, leading to repetitive games.\n\n"
              "In the 'Opening' section:\n"
              "• Click on 'Opening file' and select a file with opening positions\n"
              "• Supported formats: .epd (EPD positions), .pgn (game moves), or raw FEN strings\n"
              "The format is auto-detected based on file extension and file content.\n\n"
              "Opening books ensure variety and test engines across different positions.",
              .success = "Opening file configured!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "Time to set up the tournament structure. In the 'Tournament' section, configure:\n\n"
              "• Type: 'round-robin' - every engine plays against every other engine\n"
              "• Rounds: 2 - the complete round-robin is played twice\n"
              "• Games per pairing: 2 - each engine pair plays 2 games per round\n"
              "• Same opening: 2 - each engine plays the same opening once with white and once with black (colors swapped)\n\n"
              "With these settings and 2 engines, you'll get: 2 rounds × 1 pairing × 2 games = 4 games total. "
              "Using the same opening twice (with swapped colors) ensures fairness - any opening advantage "
              "is balanced out.",
              .success = "Tournament structure set!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "Now configure the time control - how much time each engine gets.\n\n"
              "In the 'Time Control' section:\n"
              "• Select '20.0+0.02' from the predefined options\n\n"
              "This means: 20 seconds base time + 0.02 seconds (20 milliseconds) increment per move. "
              "The increment is added after each move, preventing sudden time losses.\n\n"
              "You can either use predefined time controls or set a custom one. Time settings synchronize automatically. "
              "When you select '20.0+0.02', the custom fields update accordingly and vice versa.",
              .success = "Time control configured!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "Now set where to save the games. In the 'PGN' section:\n"
              "• Click on 'Pgn file' and choose a location and filename\n\n"
              "All games will be saved in PGN (Portable Game Notation) format - the standard "
              "format for chess games. You can later open these files in any chess software "
              "to review the games, analyze with engines, or share them.\n\n"
              "Hover over other PGN options to see what additional information can be saved.",
              .success = "PGN output configured!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "One last setting before we start: Concurrency.\n"
              "• Set 'Concurrency' to 4 at the top of the window\n\n"
              "Concurrency determines how many games run simultaneously. With 4 concurrent games, "
              "the tournament finishes 4× faster. You may want to configure one less concurrent games "
              "than your CPU has physical cores to avoid overload. If your CPU supports hyperthreading, "
              "divide the number of shown cores by two to get the number of physical cores.\n\n"
              "During the tournament, you'll see 4 board tabs appear - one for each running game. "
              "You can click on any tab to watch that game live. You can also click a line in the "
              "running games table to jump to that game's board tab.",
              .success = "Everything is configured - time to start!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "Click the 'Run' button (play icon) in the toolbar to begin the tournament.\n\n"
              "Once running:\n"
              "• The input controls will hide to save space\n"
              "• You'll see the progress bar fill up\n"
              "• Board tabs will appear for each concurrent game\n"
              "• The running games table will show active and completed games\n"
              "• The results table will update in real-time sorted by the calculated elo of each engine\n"
              "• The third table shows game termination causes statistics, it supports sorting and searching\n\n"
              "You can click 'Grace' (same button) to stop gracefully after current games finish, "
              "or 'Stop' to abort immediately.",
              .success = "The tournament is running!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = false },
            { .text = "Watch the progress bar and click on any board tab to see a live game. "
              "The engines are now playing against each other with your configured settings.\n\n"
              "While you wait, notice:\n"
              "• The crosstable showing results as they come in\n"
              "Wait for all games to complete...",
              .success = "Tournament finished!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "Your results are automatically saved, but let's also save manually to learn the process.\n\n"
              "Click the 'Save As' button (disk icon) in the toolbar.\n\n"
              "The .qtour file format saves everything:\n"
              "• All engine configurations\n"
              "• Tournament settings\n"
              "• Complete results and statistics\n"
              "• Scheduling information for continuation\n\n"
              "You can load this file later to use the same settings for another tournament, "
              "extend or reduce the tournament by adding or removing engines, rounds or games per round "
              "or continue the tournament.",
              .success = "Tournament saved!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "Now let's extend the tournament by adding a new engine. This is powerful: "
              "you don't need to restart the tournament - new pairings are automatically added.\n\n"
              "Go to 'Engine Selection' and select a third engine (any different engine).\n\n"
              "The system will calculate which games are still needed. Only the new pairings "
              "will be played - all existing results are preserved. This is great for "
              "gradually adding engines to an ongoing comparison.",
              .success = "Third engine added!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "The tournament now includes additional pairings for the new engine. "
              "The button has changed to 'Continue' because there are pending games.\n\n"
              "Click 'Continue' (same button as Run) to resume the tournament.\n\n"
              "This continue feature is also useful if you:\n"
              "• Stopped the tournament and want to resume\n"
              "• Changed settings and want to play remaining games\n"
              "• Loaded a saved tournament",
              .success = "Extended tournament is running!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "The new games are being played. Only the pairings involving the new engine "
              "are scheduled - previous results remain intact.\n\n"
              "Let it finish, or click 'Stop' if you want to end early.\n\n"
              "Tip: You can stop gracefully with 'Grace' - current games finish normally, "
              "no new games start. This preserves all game results.",
              .success = "Congratulations! Tournament Tutorial Complete!",
              .type = SnackbarManager::SnackbarType::Note,
              .waitForUserInput = true },
            { .text = "You've learned the essentials of running engine tournaments:\n\n"
              "• Configure global settings for fair conditions\n"
              "• Select and compare multiple engines\n"
              "• Set up openings, time controls, and output\n"
              "• Run tournaments with parallel games\n"
              "• Save, load, and extend tournaments\n\n"
              "Explore the Adjudication settings to auto-end drawn or won games early. "
              "Try different tournament types like 'gauntlet' where one engine plays all others.\n\n"
              "Happy testing!",
              .type = SnackbarManager::SnackbarType::Success }
        },
        .getProgressCounter = []() -> uint32_t& {
            return TournamentWindow::tutorialProgress_;
        },
        .autoStart = false
    });
    return true;
}();

