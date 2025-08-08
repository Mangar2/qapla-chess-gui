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

#include "clock-window.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/time-control.h"

#include "imgui.h"

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;

ClockWindow::ClockWindow(std::shared_ptr<BoardData> boardData)
    : boardData_(std::move(boardData))
{
}

ClockWindow::~ClockWindow() = default;


bool ClockWindow::setClockData() {
    auto& gameRecord = boardData_->gameRecord();
    auto wtc = gameRecord.getWhiteTimeControl();
    auto btc = gameRecord.getBlackTimeControl();
    if (!wtc.isValid() || !btc.isValid()) return false;
    auto [whiteTime, blackTime] = gameRecord.timeUsed();
    GoLimits goLimits = createGoLimits(wtc, btc,
        gameRecord.nextMoveIndex(), whiteTime, blackTime, gameRecord.isWhiteToMove());
    uint64_t wCur = 0;
	uint64_t bCur = 0;

	auto nextMoveIndex = gameRecord.nextMoveIndex();
    uint32_t curHalfmoveNo = 0;
    if (nextMoveIndex > 0 && nextMoveIndex <= gameRecord.history().size()) {
        curHalfmoveNo =gameRecord.history()[nextMoveIndex - 1].halfmoveNo_;
    }

	// We need to adjust the current time for the engines currently calculating a move,
    if (boardData_->engineRecords().size() >= 1) {
		auto halfmoveNo = boardData_->engineRecords()[0].curMoveRecord->halfmoveNo_;
		wCur = boardData_->engineRecords()[0].curMoveRecord->timeMs;
		if (halfmoveNo > curHalfmoveNo) goLimits.wtimeMs -= wCur;
    }
    if (boardData_->engineRecords().size() >= 2) {
        auto halfmoveNo = boardData_->engineRecords()[1].curMoveRecord->halfmoveNo_;
        bCur = boardData_->engineRecords()[1].curMoveRecord->timeMs;
		if (halfmoveNo > curHalfmoveNo) goLimits.btimeMs -= bCur;
    }

    clockData_ = {
        .wEngineName = gameRecord.getWhiteEngineName(),
        .bEngineName = gameRecord.getBlackEngineName(),
        .wTimeLeftMs = goLimits.wtimeMs,
        .bTimeLeftMs = goLimits.btimeMs,
        .wTimeCurMove = wCur,
        .bTimeCurMove = bCur
    };
    return true;
}

void ClockWindow::draw() {
    if (!setClockData()) return;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Gesamtkoordination
    ImVec2 regionPos = ImGui::GetCursorScreenPos();
    constexpr float clockHeight = 80.0f;
	constexpr float clockWidth = 200.0f;

    const float totalWidth = ImGui::GetContentRegionAvail().x;
	const float totalHeight = ImGui::GetContentRegionAvail().y;
    const float spacing = totalWidth * 0.05f;
    const float totalContentWidth = 2 * clockWidth + spacing;
	const float topSpace = (totalHeight - clockHeight) * 0.5f;

    // White Clock
    ImVec2 whiteMin = ImVec2(regionPos.x + (totalWidth - totalContentWidth) * 0.5f,
        regionPos.y + topSpace);
    ImVec2 whiteMax = ImVec2(whiteMin.x + clockWidth, whiteMin.y + clockHeight);

    // Black Clock
    ImVec2 blackMin = ImVec2(whiteMax.x + spacing, whiteMin.y);
    ImVec2 blackMax = ImVec2(blackMin.x + clockWidth, whiteMax.y);

    auto drawClockText = [&](ImVec2 boxMin, ImVec2 boxMax,
        const std::string& engineName,
        std::uint64_t timeLeftMs,
        std::uint64_t moveTimeMs) {

            drawList->AddRect(boxMin, boxMax, IM_COL32(200, 200, 200, 255), 0.0f, 0, 1.0f);

            float centerX = (boxMin.x + boxMax.x) * 0.5f;
            float y = boxMin.y + 5.0f;

            ImFont* font = ImGui::GetFont();
            auto clock = formatMs(timeLeftMs, 0);
            ImGui::PushFont(font, 28.0f);
            drawList->AddText(font, 32.0f, ImVec2(centerX - ImGui::CalcTextSize(clock.c_str()).x * 0.5f, y),
                IM_COL32(255, 255, 255, 255), clock.c_str());
            y += ImGui::GetFontSize() + 2.0f;
            ImGui::PopFont();

            ImGui::PushFont(font, 18.0f);
            auto time = formatMs(moveTimeMs, 0);
            drawList->AddText(ImVec2(centerX - ImGui::CalcTextSize(time.c_str()).x * 0.5f, y),
                IM_COL32(200, 200, 200, 255), time.c_str());
            y += ImGui::GetFontSize() + 2.0f;
            ImGui::PopFont();
            
            ImGui::PushFont(font, 24.0f);
            drawList->AddText(ImVec2(centerX - ImGui::CalcTextSize(engineName.c_str()).x * 0.5f, y),
                IM_COL32(200, 200, 50, 255), engineName.c_str());
			ImGui::PopFont();
        };

    drawClockText(whiteMin, whiteMax, clockData_.wEngineName, clockData_.wTimeLeftMs, clockData_.wTimeCurMove);
    drawClockText(blackMin, blackMax, clockData_.bEngineName, clockData_.bTimeLeftMs, clockData_.bTimeCurMove);

    ImGui::Dummy(ImVec2(0.0f, 0.0f));
}

