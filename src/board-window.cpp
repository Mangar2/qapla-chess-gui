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
#include "imgui-button.h"
#include "imgui-board.h"
#include "snackbar.h"

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
                else if (button == "Now")
                {
                    QaplaButton::drawNow(drawList, topLeft, size, state);
                }
                else if (button == "Setup")
                {
                    QaplaButton::drawSetup(drawList, topLeft, size, state);
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

    std::string BoardWindow::drawBoardButtons(const std::string& status)
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

        for (const std::string button : {"Setup", "New", "Now", "Stop", "Play", "Analyze", "Auto", "Invert"})
        {
            ImGui::SetCursorScreenPos(pos);
            auto state = QaplaButton::ButtonState::Normal;
            if (button == status || (button == "Invert" && isInverted())) {
                state = QaplaButton::ButtonState::Active;
            }
            if (drawBoardButton(button, button, buttonSize, state))
            {
               clickedButton = button;
            }
            pos.x += totalSize.x + space;
        }

        ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
        if (clickedButton == "Setup") {
            setAllowMoveInput(false);
            setAllowPieceInput(true);
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
                setAllowPieceInput(false);
            }
        }
        if (command == "Copy") {
            auto fen = getFen();
            ImGui::SetClipboardText(fen.c_str());
            SnackbarManager::instance().showNote("FEN copied to clipboard\n" + fen);
        }
        if (command == "Cancel") {
            setupMode_ = false;
            setAllowMoveInput(true);
            setAllowPieceInput(false);
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

        for (const std::string button : {"Ok", "New", "Clear", "Copy", "Cancel"})
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
        return drawBoardButtons(status);
    }
}
