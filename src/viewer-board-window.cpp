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
#include "viewer-board-window.h"

#include "imgui-engine-list.h"
#include "imgui-board.h"

#include "horizontal-split-container.h"
#include "vertical-split-container.h"

#include <qapla-engine/types.h>
#include <chess-game/game-record.h>
#include <game-manager/engine-record.h>

#include <imgui.h>
#include <memory>
#include <vector>
#include <format>

using QaplaTester::GameRecord;
using QaplaTester::EngineRecords;
using QaplaTester::MoveRecord;

namespace QaplaWindows
{

    ViewerBoardWindow::ViewerBoardWindow() {
        imGuiEngineList_.setAllowInput(false);
    }

    ViewerBoardWindow::ViewerBoardWindow(ViewerBoardWindow&&) noexcept = default;
    ViewerBoardWindow& ViewerBoardWindow::operator=(ViewerBoardWindow&&) noexcept = default;

    ViewerBoardWindow::~ViewerBoardWindow() = default;

    VerticalSplitContainer& ViewerBoardWindow::getMainWindow() {
        static std::unique_ptr<VerticalSplitContainer> mainWindow;
        if (!mainWindow) {
            mainWindow = std::make_unique<VerticalSplitContainer>("main_window");
            mainWindow->setMinTopHeight(300.0F);
            mainWindow->setMinBottomHeight(184.0F);
            mainWindow->setPresetHeight(110.0F, false);
        }
        return *mainWindow;
    }

    HorizontalSplitContainer& ViewerBoardWindow::getTopWindow() {
        static HorizontalSplitContainer* topWindow;
        if (topWindow == nullptr) {
            auto window = std::make_unique<HorizontalSplitContainer>("tournament_top");
            window->setPresetWidth(400.0F, false);
            topWindow = window.get();
            getMainWindow().setTop(std::move(window));
        }
        return *topWindow;
    }

    VerticalSplitContainer& ViewerBoardWindow::getClockMovesWindow() {
        static VerticalSplitContainer* clockMovesWindow;
        if (clockMovesWindow == nullptr) {
            auto window = std::make_unique<VerticalSplitContainer>("top_right");
            window->setFixedHeight(120.0F, true);
            clockMovesWindow = window.get();
            getTopWindow().setRight(std::move(window));
        }
        return *clockMovesWindow;
    }

    VerticalSplitContainer& ViewerBoardWindow::getMovesChartWindow() {
        static VerticalSplitContainer* movesChartWindow;
        if (movesChartWindow == nullptr) {
            auto window = std::make_unique<VerticalSplitContainer>("moves_chart");
            window->setPresetHeight(180.0F, false);
            movesChartWindow = window.get();
            getClockMovesWindow().setBottom(std::move(window));
        }
        return *movesChartWindow;
    }

    void ViewerBoardWindow::setFromGameRecord(const GameRecord& gameRecord)
    {
        round_ = gameRecord.getRound();
        gameInRound_ = gameRecord.getGameInRound();
        positionName_ = gameRecord.getPositionName();
        auto whiteEngineName = gameRecord.getWhiteEngineName();
        auto blackEngineName = gameRecord.getBlackEngineName();
        if (positionName_.empty()) {
            // Tournament game with round, game number and two engines
            tooltipText_ = std::format("Round {}, Game {}\n{} vs {}", 
                round_, gameInRound_, whiteEngineName, blackEngineName);
            windowId_ = std::format("{}.{}:{}-{}", 
                round_, gameInRound_, whiteEngineName, blackEngineName);
        } else {
            // Epd-Analysis, one engine computes a position
            tooltipText_ = std::format("{}\n{}", positionName_, whiteEngineName);
            windowId_ = std::format("{}:{}", positionName_, whiteEngineName);
        }

        if (!active_) {
            return;
        }
        imGuiBoard_.setAllowMoveInput(false);
        imGuiBoard_.setFromGameRecord(gameRecord);
        imGuiClock_.setFromGameRecord(gameRecord);
        imGuiMoveList_.setFromGameRecord(gameRecord);
        imGuiBarChart_.setFromGameRecord(gameRecord);
        imGuiEngineList_.setFromGameRecord(gameRecord);
    }

    void ViewerBoardWindow::setFromEngineRecords(const EngineRecords& engineRecords)
    {
        if (!active_) {
            return;
        }
        imGuiEngineList_.setEngineRecords(engineRecords);
    }

    void ViewerBoardWindow::setFromMoveRecord(const MoveRecord& moveRecord, uint32_t playerIndex)
    {
        if (!active_) {
            return;
        }
        imGuiEngineList_.setFromMoveRecord(moveRecord, playerIndex);
        imGuiClock_.setFromMoveRecord(moveRecord, playerIndex);
    }

    void ViewerBoardWindow::draw()
    {
        getTopWindow().setLeft([&]() {
            imGuiBoard_.draw();
        });

        getClockMovesWindow().setTop([&]() {
            imGuiClock_.draw();
        });

        getMovesChartWindow().setTop([&]() {
            imGuiMoveList_.setClickable(true);
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

    std::string formatTabTitle(const std::string& input, size_t maxTotalLength = 12) {
        const std::string separator = "-";
        // Step 1: Remove non-alphanumeric characters
        std::string cleaned;
        for (char c : input) {
            if (std::isalnum(c) != 0) {
                cleaned += c;
            }
        }
        
        // Step 2: Find the last sequence of digits (more flexible)
        std::string numbers;
        size_t lastDigitEnd = cleaned.size();
        for (size_t j = cleaned.size(); j > 0; --j) {
            if (std::isdigit(static_cast<unsigned char>(cleaned[j - 1])) != 0) {
                if (numbers.empty()) {
                    lastDigitEnd = j;
                }
                numbers.insert(numbers.begin(), cleaned[j - 1]);
            } else if (!numbers.empty()) {
                break; // Stop at the first non-digit after digits
            }
        }
        
        // Step 3: Letters are the part before the numbers
        std::string letters = cleaned.substr(0, lastDigitEnd - numbers.size());
        
        // Step 4: Calculate effective max letters based on total length
        size_t effectiveMaxLetters = maxTotalLength;
        if (!numbers.empty() && maxTotalLength > 0) {
            // Assume numbers are at least 3 digits for calculation
            size_t assumedNumbersLength = 3;
            size_t separatorLength = separator.size();
            if (assumedNumbersLength + separatorLength < maxTotalLength) {
                effectiveMaxLetters = maxTotalLength - assumedNumbersLength - separatorLength;
            } else {
                effectiveMaxLetters = 0; // Fallback if not enough space
            }
        }
        
        // Step 5: Limit letters
        if (letters.size() > effectiveMaxLetters) {
            letters = letters.substr(0, effectiveMaxLetters);
        }
        if (letters.empty()) {
            return numbers;
        }
        if (!numbers.empty()) {
            return letters + separator + numbers;
        } 
        return letters;
    }


    std::string ViewerBoardWindow::id() const {
        if (positionName_.empty()) {
            return "Game " + std::to_string(round_) + "." + std::to_string(gameInRound_);
        }
        return formatTabTitle(positionName_, 10);
    }

}