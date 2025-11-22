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

using namespace QaplaWindows;

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
         if (epdState == EpdData::State::Stopping) {
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
                    } else {
                        QaplaButton::drawPlay(drawList, topLeft, size, buttonState);
                    }
                }
                if (button == "Grace")
                {
                        QaplaButton::drawGrace(drawList, topLeft, size, buttonState);
                }
                if (button == "Clear")
                {
                    QaplaButton::drawClear(drawList, topLeft, size, buttonState);
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
    constexpr int maxSeenPlies = 32;
    bool modified = false;

    auto& config = EpdData::instance().config();

    modified |= ImGuiControls::sliderInt<uint32_t>("Concurrency", config.concurrency, 1, config.maxConcurrency);
    EpdData::instance().setPoolConcurrency(config.concurrency);

    ImGui::Spacing();
    EpdData::instance().engineSelect().draw();

    if (ImGuiControls::CollapsingHeaderWithDot("Configuration", ImGuiTreeNodeFlags_Selected))
    {
        constexpr uint64_t maxTimeInS = 3600ULL * 24ULL * 365ULL; 
        ImGui::Indent(10.0F);
        ImGui::SetNextItemWidth(inputWidth);
        modified |= ImGuiControls::inputInt<uint32_t>("Seen plies", config.seenPlies, 1, maxSeenPlies);

        ImGui::SetNextItemWidth(inputWidth);
        modified |= ImGuiControls::inputInt<uint64_t>("Max time (s)", config.maxTimeInS, 1, maxTimeInS, 1, 100);

        ImGui::SetNextItemWidth(inputWidth);
        modified |= ImGuiControls::inputInt<uint64_t>("Min time (s)", config.minTimeInS, 1, maxTimeInS, 1, 100);

        ImGui::Spacing();
        modified |= ImGuiControls::existingFileInput("Epd or RAW position file:", config.filepath, inputWidth * 2.0F);
        ImGui::Spacing();
        ImGui::Unindent(10.0F);

    }
    if (modified) {
        EpdData::instance().updateConfiguration();
    }
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
        Tutorial::instance().showNextTutorialStep(tutorialName);
        highlightedButton_ = "";
        return;
        
        case 2:
        // Step 1: Tab opened - explain EPD and ask to select two engines
        if (config.engines.size() >= 2) {
            Tutorial::instance().showNextTutorialStep(tutorialName);
            highlightedButton_ = "";
        }
        return;
        
        case 3:
        // Step 2: Two engines selected - configure parameters
        if (config.seenPlies == 3 && 
            config.maxTimeInS == 10 && 
            config.minTimeInS == 1 && 
            !config.filepath.empty()) {
            Tutorial::instance().showNextTutorialStep(tutorialName);
            highlightedButton_ = "Run/Stop";
        }
        return;
        
        case 4:
        // Step 3: Configuration complete - wait for analysis to start
        if (epdState == EpdData::State::Running) {
            highlightedButton_ = "Run/Stop";
            Tutorial::instance().showNextTutorialStep(tutorialName);
        }
        return;
        
        case 5:
        // Step 4: Analysis running - wait for it to stop
        if (epdState == EpdData::State::Stopped) {
            highlightedButton_ = "Run/Stop";
            Tutorial::instance().showNextTutorialStep(tutorialName);
        }
        return;
        
        case 6:
        // Step 5: Stopped - wait for Continue (Running again)
        if (epdState == EpdData::State::Running) {
            highlightedButton_ = "Grace";
            Tutorial::instance().showNextTutorialStep(tutorialName);
        }
        return;
        
        case 7:
        // Step 6: Running again - wait for Grace (Stopping state)
        if (epdState == EpdData::State::Stopping) {
            highlightedButton_ = "";
            Tutorial::instance().showNextTutorialStep(tutorialName);
        }
        return;
        
        case 8:
        // Step 7: Grace active - wait for fully stopped, then highlight Clear
        if (epdState == EpdData::State::Stopped) {
            highlightedButton_ = "Clear";
        }
        if (epdState == EpdData::State::Cleared) {
            highlightedButton_ = "Run/Stop";
            Tutorial::instance().showNextTutorialStep(tutorialName);
        }
        return;
        
        case 9:
        // Step 8: Cleared - wait for Analyze again (Running)
        if (epdState == EpdData::State::Running) {
            highlightedButton_ = "";
            Tutorial::instance().showNextTutorialStep(tutorialName);
        }
        return;
        
        case 10:
        // Step 9: Running - wait for concurrency to be set to 10
        if (config.concurrency == 10) {
            highlightedButton_ = "";
            Tutorial::instance().showNextTutorialStep(tutorialName);
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
            { .text = "Run engine position tests with EPD files.\n"
              "These tests evaluate engine strength on tactical positions.\n\n"
              "Click the 'EPD' tab to open this window.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "EPD window is now open.\n\n"
              "Open 'Engines' section and select two engines.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Configure test parameters in 'Configuration':\n"
              "• Seen plies: 3\n"
              "• Max time: 10s, Min time: 1s\n"
              "• Select an EPD or RAW file",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Configuration complete!\n\n"
              "Click 'Analyze' to start the test.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Analysis is running!\n\n"
              "Click 'Stop' to pause. Progress is auto-saved.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Stopped. Button now shows 'Continue'.\n\n"
              "Click 'Continue' to resume.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Analysis resumed!\n\n"
              "Click 'Grace' for graceful stop.\n"
              "Finishes current tests, skips new ones.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Grace mode! Tests finishing.\n\n"
              "Board tabs show parallel analyses.\n"
              "When stopped, click 'Clear' to delete results.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Results cleared!\n\n"
              "Click 'Analyze' to restart.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Analysis running!\n\n"
              "Move 'Concurrency' slider to 10.\n"
              "Creates 10 parallel analyses with board tabs.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "EPD Analysis Complete!\n\n"
              "Well done!\n\n"
              "• Results auto-save on exit\n"
              "• Cannot resume after restart\n"
              "• Progress bar shows completion",
              .type = SnackbarManager::SnackbarType::Success }
        },
        .getProgressCounter = []() -> uint32_t& {
            return EpdWindow::tutorialProgress_;
        },
        .autoStart = true
    });
    return true;
}();

