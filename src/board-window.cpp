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
#include "game-state.h"
#include "game-record.h"

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

    BoardWindow::BoardWindow() {
        if (tutorialInstance_ == nullptr) {
            tutorialInstance_ = this;
        } else if (secondaryTutorialInstance_ == nullptr) {
            secondaryTutorialInstance_ = this;
            // Notify primary instance that secondary instance was created
            if (tutorialInstance_ != nullptr) {
                tutorialInstance_->showNextCutPasteTutorialStep("new instance");
            }
        }
    }

    BoardWindow::~BoardWindow() {
        if (this == tutorialInstance_) {
            tutorialInstance_ = nullptr;
        } else if (this == secondaryTutorialInstance_) {
            secondaryTutorialInstance_ = nullptr;
        }
    }

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
                else if (button == "Time")
                {
                    QaplaButton::drawTimeClock(drawList, topLeft, size, state);
                }
            });
    }

    QaplaButton::ButtonState BoardWindow::getSetupButtonState(const std::string& button) const {
        auto state = QaplaButton::ButtonState::Normal;
        
        // Tutorial highlight
        if (button == highlightedButton_) {
            state = QaplaButton::ButtonState::Highlighted;
        }
        
        if (button == "Ok") {
            // Only highlight if valid position, otherwise show disabled
            return isValidPosition() ? state : QaplaButton::ButtonState::Disabled;
        }
        
        return state;
    }

    QaplaButton::ButtonState BoardWindow::getBoardButtonState(const std::string& button, const std::string& status) const {
        auto state = QaplaButton::ButtonState::Normal;
        
        if (button == status || (button == "Invert" && isInverted())) {
            state = QaplaButton::ButtonState::Active;
        }
        
        // Tutorial highlight
        if (button == highlightedButton_) {
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
        
        // More menu commands with tutorial highlights
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
        
        std::vector<std::string> allButtons = {
            "New", "Now", "Stop", "Play", "Analyze", "Auto", "Invert", "Time", "Setup", "Paste"
        };

        const auto totalSize = QaplaButton::calcIconButtonsTotalSize(buttonSize, allButtons);
        
        float availableWidth = ImGui::GetContentRegionAvail().x - leftOffset;
        float buttonWidth = totalSize.x + space;
        
        auto [visibleButtons, moreCommands] = splitButtonsForResponsiveLayout(
            allButtons, availableWidth, buttonWidth, status);
        
        auto startPos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
        std::string clickedButton = drawVisibleButtons(
            visibleButtons, startPos, buttonSize, totalSize, status);
        
        auto moreButtonPos = ImVec2(
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
            showNextBoardTutorialStep(clickedButton);
            showNextCutPasteTutorialStep(clickedButton);  
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

        std::vector<std::string> allButtons = {
            "Ok", "New", "Clear", "Copy", "Paste", "Cancel"
        };

        const auto totalSize = QaplaButton::calcIconButtonsTotalSize(buttonSize, allButtons);
        auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
        std::string clickedButton;

        for (const std::string& button : allButtons)
        {
            ImGui::SetCursorScreenPos(pos);
            if (drawSetupButton(button, button, buttonSize))
            {
               clickedButton = button;
            }
            pos.x += totalSize.x + space;
        }
        executeSetupCommand(clickedButton);
        if (!clickedButton.empty()) {
            showNextCutPasteTutorialStep(clickedButton);
        }
        ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));

        // Setup here means we deactivated the setup mode and thus we inform the caller to update its fen
        return clickedButton == "Ok" ? "Position: " + getFen() : "";
    }

    std::string BoardWindow::drawButtons(const std::string& status)
    {
        if (setupMode_) {
            return drawSetupButtons();
        } 
        showNextCutPasteTutorialStep("");
        showNextBoardTutorialStep("");
        return drawBoardButtons(status);
    }

    static auto boardWindowTutorialInit = []() {
        Tutorial::instance().setEntry({
            .name = Tutorial::TutorialName::BoardWindow,
            .displayName = "Board Window",
            .messages = {
                { .text = "Welcome to the Board Window!\n"
                  "Here you can play chess and control the engines.\n\n"
                  "Click the 'Play' button to make the first engine (white) play a move.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Great! The engine played a move.\n"
                  "Now make a counter-move. You can click piece then target, or target then piece.\n"
                  "Try clicking a5 directly - if the move is unambiguous, it executes immediately.\n\n"
                  "The computer will continue playing while 'Play' is active.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "You made a move, the engine responded automatically in play mode.\n"
                  "Click 'Play' again. This will make the second engine play for black\n"
                  "(if two engines are selected).",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Good! Now click 'Stop' to end the engine play.\n"
                  "After that, make another move manually.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Perfect! Now both sides are manual.\n"
                  "Click 'Analyze' to make both engines analyze the position\n"
                  "without making moves.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Excellent! Both engines are analyzing.\n"
                  "Click 'Stop' again to end the analysis.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Great! Now click 'Auto' to make both engines play against each other\n"
                  "automatically.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Click 'Stop' one more time to end the auto-play.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Board Controls Complete!\n\n"
                  "Well done! You now know Play, Stop, Analyze, and Auto.\n\n"
                  "Next tutorial: Learn Cut & Paste to save and load positions.",
                  .type = SnackbarManager::SnackbarType::Success }
            },
            .getProgressCounter = []() -> uint32_t& {
                return BoardWindow::tutorialProgress_;
            },
            .autoStart = false
        });
        return true;
    }();

    static auto boardWindowCutPasteTutorialInit = []() {
        Tutorial::instance().setEntry({
            .name = Tutorial::TutorialName::BoardCutPaste,
            .displayName = "Board Cut & Paste",
            .messages = {
                { .text = "Learn to manage positions and multiple boards.\n\n"
                  "First, click the 'Time' button to set the game time.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Set the time to 5 minutes and confirm with 'Apply'.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Good! Now click 'Setup' to enter setup mode.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Click 'Clear' to remove all pieces from the board.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Place two kings and a queen on the board.\n\n"
                  "Then click 'Ok' to return to play mode.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Click 'Analyze' to start engine analysis.\n"
                  "Wait a moment for the engine to calculate.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Click 'Stop' to stop engine analysis.\n",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "In the Engine List window, click on the top row.\n"
                  "This copies the position including the calculated variation (PV).",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Click the '+' tab at the top to create a second board.\n"
                  "A new tab (Board 2) will appear. You can switch between boards using the tabs.",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "Switch to the Board 2 tab and click 'Paste'.\n"
                  "The position and PV will be pasted. Navigate to see the calculated line.\n\n"
                  "Tip: Hover over Board 2 tab to see the close button (×).",
                  .type = SnackbarManager::SnackbarType::Note },
                { .text = "You've learned Cut & Paste and multi-board management!\n"
                  "You can copy FEN, PGN, PGN+PV, and paste various formats.\n\n"
                  "Use tabs to manage multiple boards. Board 1 cannot be closed.",
                  .type = SnackbarManager::SnackbarType::Success }
            },
            .getProgressCounter = []() -> uint32_t& {
                return BoardWindow::tutorialCutPasteProgress_;
            },
            .autoStart = false
        });
        return true;
    }();

    void BoardWindow::showNextBoardTutorialStep(const std::string& clickedButton) {
        if (this != tutorialInstance_) return;
        
        constexpr auto tutorialName = Tutorial::TutorialName::BoardWindow;
        
        switch (tutorialProgress_) {
            case 0:
            Tutorial::instance().showNextTutorialStep(tutorialName);
            tutorialSubStep_ = 0;
            if (tutorialProgress_ == 1) { 
                highlightedButton_ = "Play";
            }
            return;
            
            case 1:
            // Step 1: User clicks play and engine makes a move
            if (tutorialSubStep_ == 0 && clickedButton == "Play") {
                highlightedButton_ = "";
                tutorialSubStep_ = 1;
                return;
            }
            // We compare with > 0 and not == 1 to continue the tutorial, even if the user made more moves.
            if (tutorialSubStep_ == 1 && gameState_->getHalfmovesPlayed() > 0) {
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(tutorialName);
                return;
            }
            return;
            
            case 2:
            // Step 2: User makes a move and engine replies automatically. 
            if (gameState_->getHalfmovesPlayed() > 2) {
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(tutorialName);
                highlightedButton_ = "Play";
                return;
            }
            return;
            
            case 3:
            // Step 3: User clicks Play again - engine switches to the other side
            if (tutorialSubStep_ == 0 && clickedButton == "Play") {
                highlightedButton_ = "";
                tutorialSubStep_ = 1;
                return;
            }
            if (tutorialSubStep_ == 1 && gameState_->getHalfmovesPlayed() > 3) {
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(tutorialName);
                highlightedButton_ = "Stop";
                return;
            }
            return;
            
            case 4:
            // Step 4: User clicks Stop and makes a manual move without engine playing
            if (tutorialSubStep_ == 0 && clickedButton == "Stop") {
                highlightedButton_ = "";
                tutorialSubStep_ = 1;
                return;
            }
            if (tutorialSubStep_ == 1 && gameState_->getHalfmovesPlayed() > 4) {
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(tutorialName);
                highlightedButton_ = "Analyze";
                return;
            }
            return;
            
            case 5:
            // Step 5: User clicks Analyze - both engines analyze without making moves
            if (clickedButton == "Analyze") {
                highlightedButton_ = "";
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(tutorialName);
                highlightedButton_ = "Stop";
                return;
            }
            return;
            
            case 6:
            // Step 6: User clicks Stop to end the analysis
            if (clickedButton == "Stop") {
                highlightedButton_ = "";
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(tutorialName);
                highlightedButton_ = "Auto";
                return;
            }
            return;
            
            case 7:
            // Step 7: User clicks Auto - both engines play against each other automatically
            if (clickedButton == "Auto") {
                highlightedButton_ = "";
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(tutorialName);
                highlightedButton_ = "Stop";
                return;
            }
            return;
            
            case 8:
            // Step 8: User clicks Stop to end auto-play
            if (clickedButton == "Stop") {
                highlightedButton_ = "";
                tutorialSubStep_ = 0;
                Tutorial::instance().showNextTutorialStep(tutorialName);
                return;
            }
            return;
            
            case 9:
            if (!SnackbarManager::instance().isTutorialMessageVisible()) {
                Tutorial::instance().finishTutorial(tutorialName);
            }
            return;
                                    
            default:
            return;
        }
    }

    namespace {
    bool hasValidStringInClipboard() {
        auto clipboardStr = ImGuiCutPaste::getClipboardString();
        if (!clipboardStr || clipboardStr->empty()) {
            return false;
        }
        QaplaUtils::GameParser parser;
        auto parsed = parser.parse(*clipboardStr);
        return static_cast<bool>(parsed);
    }
    }

    void BoardWindow::showNextCutPasteTutorialStep(const std::string& clickedButton) {
        constexpr auto tutorialName = Tutorial::TutorialName::BoardCutPaste;
        
        bool twoBoardsStep = (tutorialCutPasteProgress_ == 9 || tutorialCutPasteProgress_ == 10);
           
        // Special handling for two board instances in steps 9 and 10
        if (this != tutorialInstance_ && (!twoBoardsStep || this != secondaryTutorialInstance_)) return;
        
        switch (tutorialCutPasteProgress_) {
            case 0:
            // Initial step - show first message
            Tutorial::instance().showNextTutorialStep(tutorialName);
            if (tutorialCutPasteProgress_ == 1) { 
                highlightedButton_ = "Time";
            }
            return;
            
            case 1:
            // Step 1: User clicks Time button
            if (clickedButton == "Time") {
                highlightedButton_ = "";
                Tutorial::instance().showNextTutorialStep(tutorialName);
                return;
            }
            return;
            
            case 2:
            // Step 2: User confirms Time Control popup
            if (clickedButton == "Time Control Confirmed") {
                Tutorial::instance().showNextTutorialStep(tutorialName);
                highlightedButton_ = "Setup";
                return;
            }
            return;
            
            case 3:
            // Step 3: User clicks Setup to enter setup mode
            if (clickedButton == "Setup") {
                highlightedButton_ = "";
                Tutorial::instance().showNextTutorialStep(tutorialName);
                highlightedButton_ = "Clear";
                return;
            }
            return;
            
            case 4:
            // Step 4: User clicks Clear to remove all pieces
            if (clickedButton == "Clear") {
                highlightedButton_ = "";
                Tutorial::instance().showNextTutorialStep(tutorialName);
                highlightedButton_ = "Ok";
                return;
            }
            return;
            
            case 5:
            // Step 5: User places pieces and clicks Ok
            if (clickedButton == "Ok" && !setupMode_) {
                highlightedButton_ = "";
                Tutorial::instance().showNextTutorialStep(tutorialName);
                highlightedButton_ = "Analyze";
                return;
            }
            return;
            
            case 6:
            // Step 6: User clicks Analyze and waits
            if (clickedButton == "Analyze") {
                highlightedButton_ = "Stop";
                Tutorial::instance().showNextTutorialStep(tutorialName);
                return;
            }
            return;

            case 7:
            // Step 7: User clicks Stop
            if (clickedButton == "Stop") {
                highlightedButton_ = "";
                Tutorial::instance().showNextTutorialStep(tutorialName);
                return;
            }
            return;
            
            case 8:
            // Step 7: User clicks on engine list row to copy PV
            if (hasValidStringInClipboard()) {
                Tutorial::instance().showNextTutorialStep(tutorialName);
                return;
            }
            return;
            
            case 9:
            // Step 8: Second board instance created (triggered from constructor)
            if (clickedButton == "new instance") {
                Tutorial::instance().showNextTutorialStep(tutorialName);
                // Highlight Paste button in the SECONDARY instance
                if (secondaryTutorialInstance_ != nullptr) {
                    secondaryTutorialInstance_->highlightedButton_ = "Paste";
                }
                return;
            }
            return;
            
            case 10:
            if (this == secondaryTutorialInstance_ && clickedButton == "Paste") {
                highlightedButton_ = "";
                Tutorial::instance().showNextTutorialStep(tutorialName);
                return;
            }
            return;
            
            case 11:
            // Letzte Message wurde angezeigt - warte bis User sie schließt
            if (!SnackbarManager::instance().isTutorialMessageVisible()) {
                Tutorial::instance().finishTutorial(tutorialName);
            }
            return;
           
            default:
            return;
        }
    }
}

