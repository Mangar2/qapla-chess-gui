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
#include "font.h"
#include "imgui-controls.h"
#include "qapla-engine/types.h"
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
    if (boardData_->moveInfos().size() >= 1) {
		auto halfmoveNo = boardData_->moveInfos()[0]->halfmoveNo_;
		wCur = boardData_->moveInfos()[0]->timeMs;
		if (halfmoveNo > curHalfmoveNo) goLimits.wtimeMs -= std::min(goLimits.wtimeMs, wCur);
    }
    if (boardData_->moveInfos().size() >= 2) {
        auto halfmoveNo = boardData_->moveInfos()[1]->halfmoveNo_;
        bCur = boardData_->moveInfos()[1]->timeMs;
		if (halfmoveNo > curHalfmoveNo) goLimits.btimeMs -= std::min(goLimits.btimeMs, bCur);
    }

    clockData_ = {
        .wEngineName = gameRecord.getWhiteEngineName(),
        .bEngineName = gameRecord.getBlackEngineName(),
        .wTimeLeftMs = goLimits.wtimeMs,
        .bTimeLeftMs = goLimits.btimeMs,
        .wTimeCurMove = wCur,
        .bTimeCurMove = bCur,
		.wtm = gameRecord.isWhiteToMove()
    };
    return true;
}

/**
 * Draws a single-side chess clock (engine name, total time, current move time).
 * The MM:SS colon of both time strings is horizontally centered within the given width.
 *
 * @param topLeft   Top-left anchor of the clock area.
 * @param bottomRight Bottom-right anchor of the clock area. 
 * @param totalMs   Total remaining time in milliseconds.
 * @param moveMs    Time for the current move in milliseconds.
 * @param engineName Name of the chess engine.
 * @param white     True if this is the white clock, false for black.
 * @param wtm       True if it is white's turn to move, false for black.
 */
static void drawClock(const ImVec2& topLeft, ImVec2& bottomRight, 
    uint64_t totalMs, uint64_t moveMs, std::string_view engineName, bool white, bool wtm)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);
    const ImGuiStyle& style = ImGui::GetStyle();

    const float baseSize = ImGui::GetFontSize();
    const float nameSize = baseSize * 1.3f;
    const float totalSize = baseSize * 1.6f; // slightly larger by request
    const float moveSize = baseSize * 1.0f;

    const float xCenter = topLeft.x + (bottomRight.x - topLeft.x) * 0.5f;
    float y = topLeft.y + 7.0f;

    const std::string totalStr = formatMs(totalMs, 0);
    const std::string moveStr = formatMs(moveMs, 0);

    auto textSizeAt = [&](float size, const char* begin, const char* end) -> ImVec2 {
        return font->CalcTextSizeA(size, FLT_MAX, 0.0f, begin, end);
        };
    auto textWidthAt = [&](float size, const char* begin, const char* end) -> float {
        return textSizeAt(size, begin, end).x;
        };
    auto startXForColonCenter = [&](const std::string& s, float size) -> float {
        const size_t colonIdx = s.find_last_of(':'); // aligns at MM:SS colon
        if (colonIdx == std::string::npos) {
            const float w = textWidthAt(size, s.c_str(), s.c_str() + s.size());
            return xCenter - w * 0.5f; // safety fallback
        }
        const char* begin = s.c_str();
        const char* mid = begin + colonIdx;
        const float leftWidth = textWidthAt(size, begin, mid);
        const char colonChar[] = ":";
        const float colonWidth = textWidthAt(size, colonChar, colonChar + 1);
        // Center at the colon's visual midpoint to keep different digit widths aligned.
        return xCenter - (leftWidth + colonWidth * 0.5f);
        };
    auto writeText = [&](float size, const std::string& text, float& y) {
        const float x = startXForColonCenter(text, size);
        const ImVec2 ext = textSizeAt(size, text.c_str(), text.c_str() + text.size());
        drawList->AddText(font, size, ImVec2(x, y), textCol,
            text.c_str(), text.c_str() + text.size());
        y += ext.y + style.ItemSpacing.y * 0.5f;
		};

    ImGuiControls::drawBoxWithShadow(topLeft, bottomRight);
    if (wtm == white) {
        font::drawPiece(drawList, white ? QaplaBasics::WHITE_KING : QaplaBasics::BLACK_KING,
            ImVec2(topLeft.x + 5, topLeft.y + 5), 30);
    }
    // Total time (bigger)
	writeText(totalSize, totalStr, y);
    // Current move time (smaller, colon vertically under total's colon)
	writeText(moveSize, moveStr, y);

    // Engine name (centered as label)
    {
        const ImVec2 nameExtent = textSizeAt(nameSize, engineName.data(), engineName.data() + engineName.size());
        const float nameX = xCenter - nameExtent.x * 0.5f;
        drawList->AddText(font, nameSize, ImVec2(nameX, y), textCol,
            engineName.data(), engineName.data() + engineName.size());
        y += nameExtent.y + style.ItemSpacing.y;
    }
}

void ClockWindow::draw() {
    if (!setClockData()) return;

    // Gesamtkoordination
    ImVec2 regionPos = ImGui::GetCursorScreenPos();
    constexpr float clockHeight = 85.0f;
	constexpr float clockWidth = 180.0f;

    const float totalWidth = ImGui::GetContentRegionAvail().x;
	const float totalHeight = ImGui::GetContentRegionAvail().y;
    const float spacing = 10.0f;
    const float totalContentWidth = 2 * clockWidth + spacing;
	const float topSpace = std::round((totalHeight - clockHeight) * 0.5f);

    // White Clock
    ImVec2 whiteMin = ImVec2(std::round(regionPos.x + (totalWidth - totalContentWidth) * 0.5f),
        std::round(regionPos.y + topSpace));
    ImVec2 whiteMax = ImVec2(std::round(whiteMin.x + clockWidth), 
        std::round(whiteMin.y + clockHeight));

    // Black Clock
    ImVec2 blackMin = ImVec2(whiteMax.x + spacing, whiteMin.y);
    ImVec2 blackMax = ImVec2(blackMin.x + clockWidth, whiteMax.y);

    drawClock(whiteMin, whiteMax, clockData_.wTimeLeftMs, clockData_.wTimeCurMove, 
        clockData_.wEngineName, true, clockData_.wtm);
    drawClock(blackMin, blackMax, clockData_.bTimeLeftMs, clockData_.bTimeCurMove, 
        clockData_.bEngineName, false, clockData_.wtm);

    ImGui::Dummy(ImVec2(0.0f, 0.0f));
}

