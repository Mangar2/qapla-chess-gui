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
#include "board-window.h"

#include "qapla-engine/types.h"
#include "qapla-tester/game-state.h"
#include "qapla-tester/game-record.h"

#include "font.h"
#include "imgui-cut-paste.h"
#include "imgui-button.h"
#include "imgui-board.h"
#include "game-parser.h"
#include "snackbar.h"
#include "tutorial.h"

#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace QaplaWindows
{

    static bool drawBoardButton(
        const std::string& button, const std::string& label,
        const ImVec2& buttonSize,
        QaplaButton::ButtonState state)
    {
        return QaplaButton::drawIconButton(
            button, label, buttonSize, state,
            [state, &button](ImDrawList *drawList, ImVec2 topLeft, ImVec2 size)
            {
                if (button == "Stop")
                {
                    QaplaButton::drawStop(drawList, topLeft, size, state);
                }
                else if (button == "Play")
                {
                    QaplaButton::drawPlay(drawList, topLeft, size, state);
                }
                else if (button == "Analyze")
                {
                    QaplaButton::drawAnalyze(drawList, topLeft, size, state);
                }
                else if (button == "New")
                {
                    QaplaButton::drawNew(drawList, topLeft, size, state);
                }
                else if (button == "Auto")
                {
                    QaplaButton::drawAutoPlay(drawList, topLeft, size, state);
                }
                else if (button == "Invert")
                {
                    QaplaButton::drawSwapEngines(drawList, topLeft, size, state);
                }
                else if (button == "Paste")
                {
                    QaplaButton::drawPaste(drawList, topLeft, size, state);
                } 
                else if (button == "Now")
                {
                    QaplaButton::drawNow(drawList, topLeft, size, state);
                }
                else if (button == "Setup")
                {
                    QaplaButton::drawSetup(drawList, topLeft, size, state);
                }
                else if (button == "More")
                {
                    QaplaButton::drawMore(drawList, topLeft, size, state);
                }
            });
    }

    QaplaButton::ButtonState BoardWindow::getSetupButtonState(const std::string& button) const {
        auto state = QaplaButton::ButtonState::Normal;
        if (button == "Ok") {
            return isValidPosition() ? QaplaButton::ButtonState::Normal : QaplaButton::ButtonState::Disabled;
        } 
        return state;
    }

    QaplaButton::ButtonState BoardWindow::getBoardButtonState(const std::string& button, const std::string& status) const {
        auto state = QaplaButton::ButtonState::Normal;
        
        if (button == status || (button == "Invert" && isInverted())) {
            state = QaplaButton::ButtonState::Active;
        }
        
        // Tutorial highlights
        if (button == "Play" && highlightPlay_) {
            state = QaplaButton::ButtonState::Highlighted;
        }
        if (button == "Stop" && highlightStop_) {
            state = QaplaButton::ButtonState::Highlighted;
        }
        if (button == "Analyze" && highlightAnalyze_) {
            state = QaplaButton::ButtonState::Highlighted;
        }
        if (button == "Auto" && highlightAuto_) {
            state = QaplaButton::ButtonState::Highlighted;
        }
        
        return state;
    }

    std::pair<std::vector<std::string>, std::vector<QaplaButton::PopupCommand>> 
    BoardWindow::splitButtonsForResponsiveLayout(
        const std::vector<std::string>& allButtons,
        float availableWidth,
        float buttonWidth,
        const std::string& status) const
    {
        int maxVisibleButtons = static_cast<int>(availableWidth / buttonWidth);
        
        std::vector<std::string> visibleButtons;
        std::vector<QaplaButton::PopupCommand> moreCommands = {
            {"Copy PGN", QaplaButton::ButtonState::Normal},
            {"Copy FEN", QaplaButton::ButtonState::Normal}
        };
        
        if (maxVisibleButtons >= static_cast<int>(allButtons.size()) + 1) {
            visibleButtons = allButtons;
        } else {
            int numVisibleButtons = std::max(1, maxVisibleButtons - 1);
            
            visibleButtons = std::vector<std::string>(
                allButtons.begin(), 
                allButtons.begin() + numVisibleButtons
            );
            
            for (int i = static_cast<int>(allButtons.size()) - 1; i >= numVisibleButtons; --i) {
                auto state = getBoardButtonState(allButtons[i], status);
                moreCommands.insert(moreCommands.begin(), {allButtons[i], state});
            }
        }
        
        return {visibleButtons, moreCommands};
    }

    std::string BoardWindow::drawVisibleButtons(
        const std::vector<std::string>& visibleButtons,
        ImVec2 startPos,
        ImVec2 buttonSize,
        ImVec2 totalSize,
        const std::string& status)
    {
        constexpr float space = 3.0F;
        auto pos = startPos;
        std::string clickedButton;
        
        for (const auto& button : visibleButtons) {
            ImGui::SetCursorScreenPos(pos);
            auto state = getBoardButtonState(button, status);
            if (drawBoardButton(button, button, buttonSize, state)) {
                clickedButton = button;
            }
            pos.x += totalSize.x + space;
        }
        
        return clickedButton;
    }

    bool BoardWindow::hasHighlightedCommand(const std::vector<QaplaButton::PopupCommand>& moreCommands) const
    {
        return std::any_of(moreCommands.begin(), moreCommands.end(),
            [](const QaplaButton::PopupCommand& cmd) {
                return cmd.state == QaplaButton::ButtonState::Highlighted;
            });
    }

    std::string BoardWindow::drawBoardButtons(const std::string& status)
    {
        constexpr float space = 3.0F;
        constexpr float topOffset = 5.0F;
        constexpr float bottomOffset = 8.0F;
        constexpr float leftOffset = 20.0F;
        constexpr ImVec2 buttonSize = {25.0F, 25.0F};
        
        ImVec2 boardPos = ImGui::GetCursorScreenPos();
        const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
        
        std::vector<std::string> allButtons = {
            "New", "Now", "Stop", "Play", "Analyze", "Auto", "Invert", "Setup", "Paste"
        };
        
        float availableWidth = ImGui::GetContentRegionAvail().x - leftOffset;
        float buttonWidth = totalSize.x + space;
        
        auto [visibleButtons, moreCommands] = splitButtonsForResponsiveLayout(
            allButtons, availableWidth, buttonWidth, status);
        
        ImVec2 startPos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
        std::string clickedButton = drawVisibleButtons(
            visibleButtons, startPos, buttonSize, totalSize, status);
        
        ImVec2 moreButtonPos = ImVec2(
            startPos.x + visibleButtons.size() * (totalSize.x + space),
            startPos.y);
        ImGui::SetCursorScreenPos(moreButtonPos);
        
        auto moreButtonState = hasHighlightedCommand(moreCommands) 
            ? QaplaButton::ButtonState::Highlighted 
            : QaplaButton::ButtonState::Normal;
        
        if (drawBoardButton("More", "More", buttonSize, moreButtonState)) {
            ImGui::OpenPopup("MoreCommandsPopup");
        }

        auto popupCommand = QaplaButton::showCommandPopup("MoreCommandsPopup", moreCommands);
        if (!popupCommand.empty()) {
            clickedButton = popupCommand;
        }

        ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
        
        if (!clickedButton.empty()) {
            showNextBoardTutorialStep(clickedButton, status);
        }
        
        if (clickedButton == "Setup") {
            setAllowMoveInput(false);
            setSetupMode(true);
            setupMode_ = true;
            return {};
        }
        return clickedButton;
    }

    void BoardWindow::executeSetupCommand(const std::string& command)
    {
        if (command.empty()) {
            return;
        }
        if (command == "New") {
            setFromFen(true, "");
        }
        if (command == "Ok") {
            if (!isValidPosition()) {
                SnackbarManager::instance().showWarning("Invalid position");
            } else {
                setupMode_ = false;
                setAllowMoveInput(true);
                setSetupMode(false);
            }
        }
        if (command == "Copy") {
            auto fen = getFen();
            ImGuiCutPaste::setClipboardString(fen);
            SnackbarManager::instance().showNote("FEN copied to clipboard\n" + fen);
        }
        if (command == "Paste") {
            auto pasted = ImGuiCutPaste::getClipboardString();
            if (setupMode_ && pasted) {
                auto gameRecord = QaplaUtils::GameParser().parse(*pasted);
                if (gameRecord) {
                    setFromGameRecord(*gameRecord, true);
                }
            }
        }
        if (command == "Cancel") {
            setupMode_ = false;
            setAllowMoveInput(true);
            setSetupMode(false);
            // The board is updated by polling, on change. 
            // Resetting the tracker thus enforces updating with the current position, resetting all changes.
            gameRecordTracker_.clear();
        }
        if (command == "Clear") {
            setFromFen(false, "8/8/8/8/8/8/8/8 w - - 0 1");
        }
    }

    bool BoardWindow::drawSetupButton(
        const std::string& button, const std::string& label,
        const ImVec2& buttonSize) const
    {
        auto state = getSetupButtonState(button);
        return QaplaButton::drawIconButton(
            button, label, buttonSize, state,
            [state, &button](ImDrawList *drawList, ImVec2 topLeft, ImVec2 size)
            {
                if (button == "Ok") {
                    QaplaButton::drawSetup(drawList, topLeft, size, state);
                } else if (button == "New") {
                    QaplaButton::drawNew(drawList, topLeft, size, state);
                } else if (button == "Clear") {
                    QaplaButton::drawClear(drawList, topLeft, size, state);
                } else if (button == "Copy") {
                    QaplaButton::drawCopy(drawList, topLeft, size, state);
                } else if (button == "Paste") {
                    QaplaButton::drawPaste(drawList, topLeft, size, state);
                } else if (button == "Cancel") {
                    QaplaButton::drawCancel(drawList, topLeft, size, state);
                }
            });
    }

    std::string BoardWindow::drawSetupButtons()
    {
        constexpr float space = 3.0F;
        constexpr float topOffset = 5.0F;
        constexpr float bottomOffset = 8.0F;
        constexpr float leftOffset = 20.0F;
        ImVec2 boardPos = ImGui::GetCursorScreenPos();

        constexpr ImVec2 buttonSize = {25.0F, 25.0F};
        const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
        auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
        std::string clickedButton;

        for (const std::string button : {"Ok", "New", "Clear", "Copy", "Paste", "Cancel"})
        {
            ImGui::SetCursorScreenPos(pos);
            if (drawSetupButton(button, button, buttonSize))
            {
               clickedButton = button;
            }
            pos.x += totalSize.x + space;
        }
        executeSetupCommand(clickedButton);
        ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));

        // Setup here means we deactivated the setup mode and thus we inform the caller to update its fen
        return clickedButton == "Ok" ? "Position: " + getFen() : "";
    }

    std::string BoardWindow::drawButtons(const std::string& status)
    {
        if (setupMode_) {
            return drawSetupButtons();
        } 
        showNextBoardTutorialStep("", status);
        return drawBoardButtons(status);
    }

    static auto boardWindowTutorialInit = []() {
        Tutorial::instance().addEntry({
            .name = "boardwindow",
            .displayName = "Board Window",
            .dependsOn = "enginewindow",
            .messages = {
                { "Board Controls - Step 1\n\n"
                  "Welcome to the Board Window!\n"
                  "Here you can play chess and control the engines.\n\n"
                  "Click the 'Play' button to make the first engine (white) play a move.",
                  SnackbarManager::SnackbarType::Note },
                { "Board Controls - Step 2\n\n"
                  "Great! The engine played a move.\n"
                  "Now make a counter-move. You can click piece then target, or target then piece.\n"
                  "Try clicking a5 directly - if the move is unambiguous, it executes immediately.\n\n"
                  "The computer will continue playing while 'Play' is active.",
                  SnackbarManager::SnackbarType::Note },
                { "Board Controls - Step 3\n\n"
                  "You made a move, the engine responded automatically in play mode.\n"
                  "Click 'Play' again. This will make the second engine play for black\n"
                  "(if two engines are selected).",
                  SnackbarManager::SnackbarType::Note },
                { "Board Controls - Step 4\n\n"
                  "Good! Now click 'Stop' to end the engine play.\n"
                  "After that, make another move manually.",
                  SnackbarManager::SnackbarType::Note },
                { "Board Controls - Step 5\n\n"
                  "Perfect! Now both sides are manual.\n"
                  "Click 'Analyze' to make both engines analyze the position\n"
                  "without making moves.",
                  SnackbarManager::SnackbarType::Note },
                { "Board Controls - Step 6\n\n"
                  "Excellent! Both engines are analyzing.\n"
                  "Click 'Stop' again to end the analysis.",
                  SnackbarManager::SnackbarType::Note },
                { "Board Controls - Step 7\n\n"
                  "Great! Now click 'Auto' to make both engines play against each other\n"
                  "automatically.",
                  SnackbarManager::SnackbarType::Note },
                { "Board Controls - Step 8\n\n"
                  "Click 'Stop' one more time to end the auto-play.",
                  SnackbarManager::SnackbarType::Note },
                { "Board Controls Complete!\n\n"
                  "Well done! You now know Play, Stop, Analyze, and Auto.\n\n"
                  "Next tutorial: Learn Cut & Paste to save and load positions.",
                  SnackbarManager::SnackbarType::Success }
            },
            .getProgressCounter = []() -> uint32_t& {
                return BoardWindow::tutorialProgress_;
            },
            .autoStart = false
        });
        return true;
    }();

    void BoardWindow::showNextBoardTutorialStep(const std::string& clickedButton, const std::string& status) {
        static const std::string topicName = "boardwindow";
        
        switch (tutorialProgress_) {
            case 0:
            Tutorial::instance().showNextTutorialStep(topicName);
            tutorialSubStep_ = 0;
            if (tutorialProgress_ == 1) { 
                highlightPlay_ = true;
            }
            return;
            
            case 1:
            // Step 1: User clicks play and engine makes a move
            if (tutorialSubStep_ == 0 && clickedButton == "Play") {
                highlightPlay_ = false;
                tutorialSubStep_ = 1;
                return;
            }
            // We compare with > 0 and not == 1 to continue the tutorial, even if the user made more moves.
            if (tutorialSubStep_ == 1 && gameState_->getHalfmovesPlayed() > 0) {
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(topicName);
                return;
            }
            return;
            
            case 2:
            // Step 2: User makes a move and engine replies automatically. 
            if (gameState_->getHalfmovesPlayed() > 2) {
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(topicName);
                highlightPlay_ = true;
                return;
            }
            return;
            
            case 3:
            // Step 3: User clicks Play again - engine switches to the other side
            if (tutorialSubStep_ == 0 && clickedButton == "Play") {
                highlightPlay_ = false;
                tutorialSubStep_ = 1;
                return;
            }
            if (tutorialSubStep_ == 1 && gameState_->getHalfmovesPlayed() > 3) {
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(topicName);
                highlightStop_ = true;
                return;
            }
            return;
            
            case 4:
            // Step 4: User clicks Stop and makes a manual move without engine playing
            if (tutorialSubStep_ == 0 && clickedButton == "Stop") {
                highlightStop_ = false;
                tutorialSubStep_ = 1;
                return;
            }
            if (tutorialSubStep_ == 1 && gameState_->getHalfmovesPlayed() > 4) {
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(topicName);
                highlightAnalyze_ = true;
                return;
            }
            return;
            
            case 5:
            // Step 5: User clicks Analyze - both engines analyze without making moves
            if (clickedButton == "Analyze") {
                highlightAnalyze_ = false;
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(topicName);
                highlightStop_ = true;
                return;
            }
            return;
            
            case 6:
            // Step 6: User clicks Stop to end the analysis
            if (clickedButton == "Stop") {
                highlightStop_ = false;
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(topicName);
                highlightAuto_ = true;
                return;
            }
            return;
            
            case 7:
            // Step 7: User clicks Auto - both engines play against each other automatically
            if (clickedButton == "Auto") {
                highlightAuto_ = false;
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(topicName);
                highlightStop_ = true;
                return;
            }
            return;
            
            case 8:
            // Step 8: User clicks Stop to end auto-play
            if (clickedButton == "Stop") {
                highlightStop_ = false;
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(topicName);
                return;
            }
            return;
                                    
            default:
            Tutorial::instance().finishTutorial(topicName);
            return;
        }
    }
}
