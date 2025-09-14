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
        imGuiEngineList_.setAllowInput(false);
    }

    TournamentBoardWindow::TournamentBoardWindow(TournamentBoardWindow&&) noexcept = default;
    TournamentBoardWindow& TournamentBoardWindow::operator=(TournamentBoardWindow&&) noexcept = default;

    TournamentBoardWindow::~TournamentBoardWindow() = default;

    VerticalSplitContainer& TournamentBoardWindow::getMainWindow() {
        static std::unique_ptr<VerticalSplitContainer> mainWindow;
        if (!mainWindow) {
            mainWindow = std::make_unique<VerticalSplitContainer>("main_window");
            mainWindow->setMinTopHeight(300.0f);
            mainWindow->setMinBottomHeight(184.0f);
            mainWindow->setPresetHeight(110.0f, false);
        }
        return *mainWindow;
    }

    HorizontalSplitContainer& TournamentBoardWindow::getTopWindow() {
        static HorizontalSplitContainer* topWindow;
        if (!topWindow) {
            auto window = std::make_unique<HorizontalSplitContainer>("tournament_top");
            window->setPresetWidth(400.0f, false);
            topWindow = window.get();
            getMainWindow().setTop(std::move(window));
        }
        return *topWindow;
    }

    VerticalSplitContainer& TournamentBoardWindow::getClockMovesWindow() {
        static VerticalSplitContainer* clockMovesWindow;
        if (!clockMovesWindow) {
            auto window = std::make_unique<VerticalSplitContainer>("top_right");
            window->setFixedHeight(120.0f, true);
            clockMovesWindow = window.get();
            getTopWindow().setRight(std::move(window));
        }
        return *clockMovesWindow;
    }

    VerticalSplitContainer& TournamentBoardWindow::getMovesChartWindow() {
        static VerticalSplitContainer* movesChartWindow;
        if (!movesChartWindow) {
            auto window = std::make_unique<VerticalSplitContainer>("moves_chart");
            window->setPresetHeight(180.0f, false);
            movesChartWindow = window.get();
            getClockMovesWindow().setBottom(std::move(window));
        }
        return *movesChartWindow;
    }

    void TournamentBoardWindow::setFromGameRecord(const GameRecord& gameRecord)
    {
        round_ = gameRecord.getRound();
        gameInRound_ = gameRecord.getGameInRound();
        if (!active_) return;
        imGuiBoard_.setAllowMoveInput(false);
        imGuiBoard_.setGameState(gameRecord);
        imGuiClock_.setFromGameRecord(gameRecord);
        imGuiMoveList_.setFromGameRecord(gameRecord);
        imGuiBarChart_.setFromGameRecord(gameRecord);
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
        getTopWindow().setLeftCallback([&]() {
            imGuiBoard_.draw();
        });

        getClockMovesWindow().setTop([&]() {
            imGuiClock_.draw();
        });

        getMovesChartWindow().setTop([&]() {
            imGuiMoveList_.draw();
        });
        getMovesChartWindow().setBottom([&]() {
            imGuiBarChart_.draw();
        });

        getMainWindow().setBottom([&]() {
            imGuiEngineList_.draw();
        });
        
        getMainWindow().draw();
    }

}
