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
#include "tournament-board-window.h"
#include "tournament-data.h"

#include "imgui-engine-list.h"
#include "imgui-board.h"

#include "horizontal-split-container.h"
#include "vertical-split-container.h"

#include "qapla-engine/types.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/engine-record.h"

#include "imgui-board.h"

#include <imgui.h>
#include <memory>
#include <vector>

namespace QaplaWindows
{

    TournamentBoardWindow::TournamentBoardWindow() {
        mainWindow_ = std::make_unique<VerticalSplitContainer>();
        mainWindow_->setMinTopHeight(300.0f);
        mainWindow_->setMinBottomHeight(110.0f);

        auto topWindow = std::make_unique<HorizontalSplitContainer>();
        topWindow_ = topWindow.get();
        topWindow_->setRightWidth(400.0f);
        mainWindow_->setTop(std::move(topWindow));

        auto topRightWindow = std::make_unique<VerticalSplitContainer>();
        topRightWindow_ = topRightWindow.get();
        topRightWindow_->setFixedTopHeight(120.0f);
        topWindow_->setRight(std::move(topRightWindow));

        imGuiEngineList_.setAllowInput(false);
    }

    TournamentBoardWindow::TournamentBoardWindow(TournamentBoardWindow&&) noexcept = default;
    TournamentBoardWindow& TournamentBoardWindow::operator=(TournamentBoardWindow&&) noexcept = default;

    TournamentBoardWindow::~TournamentBoardWindow() = default;

    void TournamentBoardWindow::setFromGameRecord(const GameRecord& gameRecord)
    {
        round_ = gameRecord.getRound();
        gameInRound_ = gameRecord.getGameInRound();
        if (!active_) return;
        imGuiBoard_.setAllowMoveInput(false);
        imGuiBoard_.setGameState(gameRecord);
        imGuiClock_.setFromGameRecord(gameRecord);
        imGuiMoveList_.setFromGameRecord(gameRecord);
    }

    void TournamentBoardWindow::setFromEngineRecords(const EngineRecords& engineRecords)
    {
        if (!active_) return;
        imGuiEngineList_.setEngineRecords(engineRecords);
    }

    void TournamentBoardWindow::setFromMoveRecord(const MoveRecord& moveRecord, uint32_t playerIndex)
    {
        if (!active_) return;
        imGuiEngineList_.setFromMoveRecord(moveRecord, playerIndex);
        imGuiClock_.setFromMoveRecord(moveRecord, playerIndex);
    }

    void TournamentBoardWindow::draw()
    {
        topWindow_->setLeft(std::make_unique<LambdaEmbeddedWindowWrapper>([&]() {
            imGuiBoard_.draw();
        }));

        topRightWindow_->setTop(std::make_unique<LambdaEmbeddedWindowWrapper>([&]() {
            imGuiClock_.draw();
        }));

        topRightWindow_->setBottom(std::make_unique<LambdaEmbeddedWindowWrapper>([&]() {
            imGuiMoveList_.draw();
        }));

        mainWindow_->setBottom(
            std::make_unique<LambdaEmbeddedWindowWrapper>([&]() {
                imGuiEngineList_.draw();
            })
        );
        mainWindow_->draw();
    }

}
