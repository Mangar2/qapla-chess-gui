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

#include "epd-window.h"
#include "imgui-table.h"
#include "imgui-button.h"
#include "snackbar.h"
#include "tutorial.h"
#include "os-dialogs.h"
#include "imgui-controls.h"
#include "imgui-epd-configuration.h"
#include "epd-data.h"
#include "configuration.h"

#include "move-record.h"
#include "game-record.h"
#include "string-helper.h"
#include "engine-event.h"
#include "game-manager-pool.h"

#include <imgui.h>

#include <sstream>
#include <string>
#include <format>
#include <memory>
#include <optional>

namespace QaplaWindows {

EpdWindow::EpdWindow() = default;
EpdWindow::~EpdWindow() = default;

static std::string getButtonText(const std::string& button, EpdData::State epdState) {
    auto& epdData = EpdData::instance();
    if (button == "Run/Stop")
    {
        if (epdState == EpdData::State::Running) {
            return "Stop";
        } 
        auto remaining = epdData.remainingTests;
        return EpdData::instance().configChanged() || remaining == 0 ? "Analyze" : "Continue";
    } 
    return button;
}

static QaplaButton::ButtonState getButtonState(const std::string& button, EpdData::State epdState) {
    // Tutorial highlighting
    if (!EpdWindow::highlightedButton_.empty() && button == EpdWindow::highlightedButton_) {
        return QaplaButton::ButtonState::Highlighted;
    }
    
    if (button == "Run/Stop")
    {
        auto& epdData = EpdData::instance();

        if (epdState == EpdData::State::Running) {
            return QaplaButton::ButtonState::Active;
        } 

        if (!epdData.mayAnalyze(false)) {
            return QaplaButton::ButtonState::Disabled;
        }
    } 
    if (button == "Grace")
    {
         if (epdState == EpdData::State::Gracefully) {
            return QaplaButton::ButtonState::Active;
        } 
        if (epdState != EpdData::State::Running) {
            return QaplaButton::ButtonState::Disabled;
        } 
    } 
    if (button == "Clear")
    {
        if (epdState != EpdData::State::Stopped) {
            return QaplaButton::ButtonState::Disabled;
        }
    }
    return QaplaButton::ButtonState::Normal;
}

std::string EpdWindow::drawButtons()
{
    constexpr float space = 3.0F;
    constexpr float paddingTop = 5.0F;
    constexpr float paddingBottom = 8.0F;
    constexpr float paddingLeft = 20.0F;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = {25.0F, 25.0F};
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
    auto pos = ImVec2(boardPos.x + paddingLeft, boardPos.y + paddingTop);
    
    std::string clickedButton;
    
    for (const std::string button : {"Run/Stop", "Grace", "Clear"})
    {
        ImGui::SetCursorScreenPos(pos);
        
        auto buttonText = getButtonText(button, EpdData::instance().state);
        auto buttonState = getButtonState(button, EpdData::instance().state);
       
        if (QaplaButton::drawIconButton(button, buttonText, buttonSize, buttonState, 
            [&button, buttonState](ImDrawList *drawList, ImVec2 topLeft, ImVec2 size)
            {
                auto epdState = EpdData::instance().state;
                if (button == "Run/Stop")
                {
                    if (epdState == EpdData::State::Running) {
                        QaplaButton::drawStop(drawList, topLeft, size, buttonState);
                        ImGuiControls::hooverTooltip("Stop EPD analysis immediately");
                    } else {
                        QaplaButton::drawPlay(drawList, topLeft, size, buttonState);
                        ImGuiControls::hooverTooltip(EpdData::instance().configChanged() || EpdData::instance().remainingTests == 0 
                            ? "Start EPD position analysis" 
                            : "Continue EPD position analysis");
                    }
                }
                if (button == "Grace")
                {
                        QaplaButton::drawGrace(drawList, topLeft, size, buttonState);
                        ImGuiControls::hooverTooltip("Stop EPD analysis gracefully after current positions finish");
                }
                if (button == "Clear")
                {
                    QaplaButton::drawClear(drawList, topLeft, size, buttonState);
                    ImGuiControls::hooverTooltip("Clear all EPD analysis results");
                } 
            }))
        {
            clickedButton = button;
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + paddingTop + paddingBottom));
    return clickedButton;
}

void QaplaWindows::EpdWindow::executeCommand(const std::string &button)
{
    {
        try
        {
            auto epdState = EpdData::instance().state;
            if (button == "Run/Stop" && epdState != EpdData::State::Running)
            {
                EpdData::instance().analyse();
            }
            else if (button == "Run/Stop" && epdState == EpdData::State::Running)
            {
                EpdData::instance().stopPool(false);
            }
            else if (button == "Grace")
            {
                EpdData::instance().stopPool(true);
            }
            else if (button == "Clear" && epdState == EpdData::State::Stopped)
            {
                EpdData::instance().clear();
            }
        }
        catch (const std::exception &e)
        {
            SnackbarManager::instance().showError(std::string("Fehler: ") + e.what());
        }
    }
}

void EpdWindow::drawInput()
{
    constexpr float inputWidth = 200.0F;
    constexpr int maxConcurrency = 32;

    auto& epdData = EpdData::instance();

    auto concurrency = epdData.getExternalConcurrency();
    ImGuiControls::sliderInt<uint32_t>("Concurrency", concurrency, 1, maxConcurrency);
    ImGuiControls::hooverTooltip("Number of positions analyzed in parallel");
    epdData.setExternalConcurrency(concurrency);
    epdData.setPoolConcurrency(concurrency, true);

    ImGui::Spacing();
    epdData.engineSelect().draw();

    ImGuiEpdConfiguration epdConfig;
    ImGuiEpdConfiguration::DrawOptions options {
        .alwaysOpen = false,
        .showSeenPlies = true,
        .showMaxTime = true,
        .showMinTime = true,
        .showFilePath = true
    };
    epdConfig.draw(options, inputWidth, 10.0F);
}

void EpdWindow::drawProgress()
{
    auto total = EpdData::instance().totalTests;
    auto remaining = EpdData::instance().remainingTests;
    if (total == 0 || remaining == 0) {
        return;
    }
    size_t testFinished = total - remaining;
    float progress = static_cast<float>(testFinished) / static_cast<float>(total);
    ImGui::ProgressBar(progress, ImVec2(ImGui::GetContentRegionAvail().x, 0.0F), 
        std::to_string(testFinished).c_str());
    ImGuiControls::hooverTooltip("EPD analysis progress: positions analyzed / total positions");
}

void EpdWindow::draw()
{
    constexpr float rightBorder = 5.0F;
    auto clickedButton = drawButtons();
    
    if (!clickedButton.empty()) {
        executeCommand(clickedButton);
    }

    ImGui::Indent(10.0F);
    auto size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("InputArea", ImVec2(size.x - rightBorder, 0), ImGuiChildFlags_None);

    drawInput();
    drawProgress();
    ImVec2 tableSize = ImGui::GetContentRegionAvail();
    auto clickedRow = EpdData::instance().drawTable(tableSize);
    
    ImGui::EndChild();
    ImGui::Unindent(10.0F);

    EpdData::instance().setSelectedIndex(clickedRow);
    
    showNextEpdTutorialStep(clickedButton);
}

bool EpdWindow::highlighted() const {
    // Show highlight after tutorial start message only
    return tutorialProgress_ == 1;
}

void EpdWindow::showNextEpdTutorialStep([[maybe_unused]] const std::string& clickedButton) {
    constexpr auto tutorialName = Tutorial::TutorialName::Epd;
    
    auto& epdData = EpdData::instance();
    auto& config = epdData.config();
    auto epdState = epdData.state;
    
    switch (tutorialProgress_) {
        case 1:
        // Step 0 (autoStart): Tutorial started, tab is highlighted
        // When draw() is called, the tab is open -> advance to next step
        Tutorial::instance().requestNextTutorialStep(tutorialName);
        highlightedButton_ = "";
        return;
        
        case 2:
        // Step 1: Tab opened - explain EPD and ask to select two engines
        if (config.engines.size() >= 2) {
            Tutorial::instance().requestNextTutorialStep(tutorialName);
            highlightedButton_ = "";
        }
        return;
        
        case 3:
        // Step 2: Two engines selected - configure parameters
        if (config.seenPlies == 3 && 
            config.maxTimeInS == 10 && 
            config.minTimeInS == 1 && 
            !config.filepath.empty()) {
            Tutorial::instance().requestNextTutorialStep(tutorialName);
            highlightedButton_ = "Run/Stop";
        }
        return;
        
        case 4:
        // Step 3: Configuration complete - wait for analysis to start
        if (epdState == EpdData::State::Running) {
            highlightedButton_ = "Run/Stop";
            Tutorial::instance().requestNextTutorialStep(tutorialName);
        }
        return;
        
        case 5:
        // Step 4: Analysis running - wait for it to stop
        if (epdState == EpdData::State::Stopped) {
            highlightedButton_ = "Run/Stop";
            Tutorial::instance().requestNextTutorialStep(tutorialName);
        }
        return;
        
        case 6:
        // Step 5: Stopped - wait for Continue (Running again)
        if (epdState == EpdData::State::Running) {
            highlightedButton_ = "Grace";
            Tutorial::instance().requestNextTutorialStep(tutorialName);
        }
        return;
        
        case 7:
        // Step 6: Running again - wait for Grace (Stopping state)
        if (epdState == EpdData::State::Gracefully) {
            highlightedButton_ = "";
            Tutorial::instance().requestNextTutorialStep(tutorialName);
        }
        return;
        
        case 8:
        // Step 7: Grace active - wait for fully stopped, then highlight Clear
        if (epdState == EpdData::State::Stopped) {
            highlightedButton_ = "Clear";
        }
        if (epdState == EpdData::State::Cleared) {
            highlightedButton_ = "Run/Stop";
            Tutorial::instance().requestNextTutorialStep(tutorialName);
        }
        return;
        
        case 9:
        // Step 8: Cleared - wait for Analyze again (Running)
        if (epdState == EpdData::State::Running) {
            highlightedButton_ = "";
            Tutorial::instance().requestNextTutorialStep(tutorialName);
        }
        return;
        
        case 10:
        // Step 9: Running - wait for concurrency to be set to 10
        if (epdData.getExternalConcurrency() == 10) {
            highlightedButton_ = "";
            Tutorial::instance().requestNextTutorialStep(tutorialName);
        }
        return;
        
        case 11:
        if (!SnackbarManager::instance().isTutorialMessageVisible()) {
            highlightedButton_ = "";
            Tutorial::instance().finishTutorial(tutorialName);
        }
        return;
                                
        default:
        return;
    }
}

static auto epdWindowTutorialInit = []() {
    Tutorial::instance().setEntry({
        .name = Tutorial::TutorialName::Epd,
        .displayName = "EPD Analysis",
        .messages = {
            { .text = "Welcome to the EPD Analysis Tutorial!\n\n"
              "EPD (Extended Position Description) analysis lets you test how well chess engines solve tactical positions. "
              "This is different from tournaments - instead of playing full games, engines analyze predefined positions "
              "and try to find the correct move or evaluation.\n\n"
              "EPD files contain positions with expected solutions. Engines score points when they find the correct move "
              "within the time limit. This helps you:\n"
              "• Evaluate tactical strength across different engines\n"
              "• Test engines on specific position types (tactics, endgames, etc.)\n"
              "• Compare analysis quality rather than playing strength\n\n"
              "To help you through the process, we'll mark relevant buttons with a red dot as needed.\n\n"
              "Let's begin! Click on the 'EPD' tab in the left window to open the analysis configuration.",
              .success = "Great! The EPD Analysis window is now open.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Now let's select the engines to test. You can compare multiple engines to see which "
              "performs better on tactical positions.\n\n"
              "In the 'Engines' section:\n"
              "• Select at least two engines from your configured engines\n"
              "• Click the '+' button next to each engine you want to test\n\n"
              "The more engines you select, the better you can compare their tactical abilities. "
              "All selected engines will analyze the same positions under identical conditions for a fair comparison.",
              .success = "Engines selected! Let's configure the analysis parameters.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Time to configure how the analysis should run. In the 'Configuration' section, set:\n\n"
              "• 'Seen plies': 3 - engines must find the solution within 3 search iterations\n"
              "• 'Max time': 10 seconds - maximum time per position\n"
              "• 'Min time': 1 second - minimum analysis time even if solution is found earlier\n"
              "• 'EPD file': Click and select a test suite file (.epd or .raw format)\n\n"
              "The seen plies setting is important: engines get points when they find the correct move "
              "and keep it as their top choice for at least 3 consecutive search depths. This filters out "
              "lucky guesses and ensures the engine truly understands the position.\n\n"
              "Popular test suites include: WAC (Win At Chess), ERET, or custom tactical position sets.",
              .success = "Configuration is complete!",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Everything is configured - time to start the analysis!\n\n"
              "Click the 'Analyze' button (play icon) in the toolbar to begin.\n\n"
              "Once started:\n"
              "• Engines will analyze each position from the EPD file sequentially\n"
              "• The progress bar shows how many positions have been completed\n"
              "• Board tabs will appear showing current analysis (one per concurrent analysis)\n"
              "• The results table updates in real time, showing solved/total positions for each engine\n"
              "• Results are automatically saved at each position\n\n"
              "You can stop and continue anytime - progress is never lost!",
              .success = "Analysis is running! Watch the engines solve positions.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "The analysis is now active! You can see the engines working through the position set.\n\n"
              "Notice the board tabs at the top - click any tab to watch that engine's current analysis live. "
              "You'll see the principal variation, evaluation, and search depth updating in real time.\n\n"
              "Let's practice stopping and resuming:\n"
              "• Click the 'Stop' button (same location as Analyze) to pause the analysis\n\n"
              "Stopping is instant - current positions are saved, and you can continue exactly where you left off. "
              "This is useful if you need your CPU for other tasks or want to review results so far.",
              .success = "Analysis paused. Notice the button now shows 'Continue'.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "The analysis is paused, and all progress is safely saved. The results table shows "
              "what has been analyzed so far.\n\n"
              "Now let's resume:\n"
              "• Click the 'Continue' button (same button as before)\n\n"
              "The analysis will pick up exactly where it stopped - same position, same progress. "
              "No work is repeated, no results are lost. This makes it easy to run long test suites "
              "over multiple sessions.\n\n"
              "You can stop and continue as many times as needed!",
              .success = "Analysis resumed!",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "The analysis is running again. Now let's learn about graceful stopping - "
              "a feature that's useful when you want to stop cleanly.\n\n"
              "Click the 'Grace' button in the toolbar.\n\n"
              "Graceful stop is different from regular stop:\n"
              "• Regular Stop: Interrupts analysis immediately, current position may be incomplete\n"
              "• Grace: Lets current positions finish completely, then stops\n\n"
              "This ensures all results are complete and valid. When multiple positions run in parallel "
              "(with concurrency > 1), Grace waits for all active positions to finish analyzing before stopping. "
              "No new positions are started during Grace mode.",
              .success = "Grace mode activated - current analyses are finishing.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Grace mode is active! Watch the progress bar - you'll see it's not starting new positions, "
              "but current ones are still completing.\n\n"
              "If you have concurrency set higher than 1, you'll see multiple board tabs. Each tab shows "
              "one engine analyzing one position in parallel. This speeds up the entire test suite significantly.\n\n"
              "Wait for the analysis to fully stop (all current positions complete).\n\n"
              "Once stopped, let's try the 'Clear' button:\n"
              "• Click 'Clear' to delete all results and start fresh\n\n"
              "Clear is useful when you want to re-run the test with different settings or engines. "
              "It removes all progress but keeps your configuration.",
              .success = "Analysis stopped gracefully.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Results have been cleared! The results table is empty, and the progress bar is reset.\n\n"
              "Your configuration (engines, time limits, EPD file) is still intact - only the results were removed. "
              "This makes it easy to quickly re-run tests with the same settings.\n\n"
              "Let's start again:\n"
              "• Click 'Analyze' to begin a fresh analysis\n\n"
              "The engines will start analyzing from the first position again. All previous results are gone, "
              "giving you a clean slate.",
              .success = "Fresh analysis started!",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "The analysis is running again with fresh results. Now let's explore concurrency - "
              "one of the most powerful features for speeding up analysis.\n\n"
              "Move the 'Concurrency' slider at the top to 10.\n\n"
              "Concurrency determines how many positions are analyzed simultaneously:\n"
              "• Concurrency 1: Positions analyzed one at a time (slowest, least CPU usage)\n"
              "• Concurrency 10: 10 positions analyzed in parallel (10× faster!)\n\n"
              "With concurrency 10, you'll see 10 board tabs appear - one for each parallel analysis. "
              "Each tab shows a different engine working on a different position. You can click any tab "
              "to watch that specific analysis.\n\n"
              "Note: Set concurrency based on your CPU cores. Too high may slow down your system.",
              .success = "Concurrency increased - analysis is now much faster!",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Congratulations! You've completed the EPD Analysis Tutorial!\n\n"
              "You've learned how to:\n"
              "• Select engines for tactical testing\n"
              "• Configure analysis parameters (time limits, seen plies, test files)\n"
              "• Start, stop, and continue analyses\n"
              "• Use graceful stopping for clean exits\n"
              "• Clear and restart tests\n"
              "• Adjust concurrency for faster results\n\n"
              "Important notes:\n"
              "• Results are auto-saved after each position\n"
              "• You can stop/continue anytime without losing progress\n"
              "• Results cannot be resumed after application restart (config is saved, but progress is reset)\n"
              "• Higher concurrency = faster completion, but more CPU usage\n"
              "• The progress bar and results table update in real time\n\n"
              "Try different EPD test suites to evaluate your engines on various tactical themes. Happy analyzing!",
              .type = SnackbarManager::SnackbarType::Success }
        },
        .getProgressCounter = []() -> uint32_t& {
            return EpdWindow::tutorialProgress_;
        },
        .autoStart = true
    });
    return true;
}();

}