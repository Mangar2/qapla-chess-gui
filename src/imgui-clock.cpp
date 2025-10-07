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

#include "imgui-clock.h"
#include "font.h"
#include "imgui-controls.h"
#include "qapla-engine/types.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/move-record.h"
#include "qapla-tester/time-control.h"

#include <imgui.h>

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;

ImGuiClock::ImGuiClock()
{
}

ImGuiClock::~ImGuiClock() = default;

void ImGuiClock::setFromGameRecord(const GameRecord& gameRecord) {
    bool update = gameRecordTracker_.checkModification(gameRecord.getChangeTracker()).second;
    if (!update) return;
    gameRecordTracker_.updateFrom(gameRecord.getChangeTracker());

    auto wtc = gameRecord.getWhiteTimeControl();
    auto btc = gameRecord.getBlackTimeControl();
    if (!wtc.isValid() || !btc.isValid()) return;
    auto [whiteTime, blackTime] = gameRecord.timeUsed();
    auto nextMoveIndex = gameRecord.nextMoveIndex();
    auto halfMoves = gameRecord.halfmoveNoAtPly(nextMoveIndex);
    GoLimits goLimits = createGoLimits(wtc, btc,
        halfMoves, whiteTime, blackTime, gameRecord.isWhiteToMove());

    if (nextMoveIndex == 0) {
        clockData_.wEngineName = gameRecord.getWhiteEngineName();
        clockData_.bEngineName = gameRecord.getBlackEngineName();
    }
    clockData_.wTimeLeftMs = goLimits.wtimeMs;
    clockData_.bTimeLeftMs = goLimits.btimeMs;
    clockData_.wTimeCurMove = 0;
    clockData_.bTimeCurMove = 0;
    clockData_.wtm = gameRecord.isWhiteToMove();
    clockData_.wTimer.reset();
    clockData_.bTimer.reset();
    nextHalfmoveNo_ = gameRecord.halfmoveNoAtPly(nextMoveIndex);

    if (nextMoveIndex > 0) {
        setFromHistoryMove(gameRecord.history()[nextMoveIndex - 1]);
    }
}

void QaplaWindows::ImGuiClock::setFromHistoryMove(const MoveRecord& moveRecord) {
    // if wtm, then black just moved (currMove is black's move)
    if (clockData_.wtm)
    {
        if (!stopped_)
            clockData_.wTimer.start();
        else
        {
            clockData_.bTimeCurMove = moveRecord.timeMs;
            // bTimeLeftMs has the time after current move.
            clockData_.bTimeLeftMs += moveRecord.timeMs;
            clockData_.bEngineName = moveRecord.engineName_;
        }
    }
    else
    {
        if (!stopped_)
            clockData_.bTimer.start();
        else
        {
            clockData_.wTimeCurMove = moveRecord.timeMs;
            // wTimeLeftMs has the time after current move.
            clockData_.wTimeLeftMs += moveRecord.timeMs;
            clockData_.wEngineName = moveRecord.engineName_;
        }
    }
}


void ImGuiClock::setFromMoveRecord(const MoveRecord& moveRecord, uint32_t playerIndex) {
    if (stopped_) return;
    auto halfmoveNo = moveRecord.halfmoveNo_;
    if (infoCnt_.size() <= playerIndex) {
        infoCnt_.resize(playerIndex + 1, 0);
        displayedMoveNo_.resize(playerIndex + 1, 0);
    }
    if (halfmoveNo != nextHalfmoveNo_) {
        return; 
    }
    if (moveRecord.infoUpdateCount == infoCnt_[playerIndex] && halfmoveNo == displayedMoveNo_[playerIndex]) {
        return; 
    }
    infoCnt_[playerIndex] = moveRecord.infoUpdateCount;
    displayedMoveNo_[playerIndex] = halfmoveNo;

    uint64_t cur = moveRecord.timeMs;

    if (clockData_.wtm) {
        if (cur > clockData_.wTimeCurMove) {
            if (stopped_) clockData_.wTimer.reset(); 
            else clockData_.wTimer.start();
        }
        clockData_.wTimeCurMove = cur;
        clockData_.wEngineName = analyze_ ? "Analyze" : moveRecord.engineName_;
    } 
    else {
        if (cur > clockData_.bTimeCurMove) {
            if (stopped_) clockData_.bTimer.reset(); 
            else clockData_.bTimer.start();
        }
        clockData_.bTimeCurMove = cur;
        clockData_.bEngineName = analyze_ ? "Analyze" : moveRecord.engineName_;
    }
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
    uint64_t totalMs, uint64_t moveMs, std::string_view engineName, bool white, bool wtm, bool analyze)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);
    const ImGuiStyle& style = ImGui::GetStyle();

    const float baseSize = ImGui::GetFontSize();
    const float nameSize = baseSize * 1.3f;
    const float totalSize = baseSize * 1.6f; 
    const float moveSize = baseSize * 1.0F;

    const float xCenter = topLeft.x + (bottomRight.x - topLeft.x) * 0.5f;
    float y = topLeft.y + 7.0F;

    totalMs -= std::min(totalMs, moveMs);
    totalMs += 999; // Add 999ms to compensate for formatMs truncating to full seconds
    if (analyze) totalMs = moveMs;

    const std::string totalStr = QaplaHelpers::formatMs(totalMs, 0);
    const std::string moveStr = QaplaHelpers::formatMs(moveMs, 0);

    auto textSizeAt = [&](float size, const char* begin, const char* end) -> ImVec2 {
        return font->CalcTextSizeA(size, FLT_MAX, 0.0F, begin, end);
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
    }
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
static void drawSmallClock(const ImVec2& topLeft, ImVec2& bottomRight, 
    uint64_t totalMs, uint64_t moveMs, std::string_view engineName, bool white, bool wtm, bool analyze)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);
    const ImGuiStyle& style = ImGui::GetStyle();

    const float baseSize = ImGui::GetFontSize();
    const float totalSize = baseSize * 1.6f; 
    const float moveSize = baseSize * 1.0F;

    const float xCenter = topLeft.x + (bottomRight.x - topLeft.x) * 0.5f;
    float y = topLeft.y + 7.0F;

    totalMs -= std::min(totalMs, moveMs);
    if (analyze) totalMs = moveMs;

    const std::string totalStr = QaplaHelpers::formatMs(totalMs, 0);
    const std::string moveStr = QaplaHelpers::formatMs(moveMs, 0);

    auto textSizeAt = [&](float size, const char* begin, const char* end) -> ImVec2 {
        return font->CalcTextSizeA(size, FLT_MAX, 0.0F, begin, end);
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

    if (wtm == white) {
        font::drawPiece(drawList, white ? QaplaBasics::WHITE_KING : QaplaBasics::BLACK_KING,
            ImVec2(topLeft.x + 5, topLeft.y + 5), 30);
    }
    // Total time (bigger)
	writeText(totalSize, totalStr, y);
    // Current move time (smaller, colon vertically under total's colon)
	writeText(moveSize, moveStr, y);
}

void ImGuiClock::draw() {

    ImVec2 topLeft = ImGui::GetCursorScreenPos();
	constexpr float clockWidth = 180.0F;

    const float totalWidth = ImGui::GetContentRegionAvail().x;
	const float totalHeight = ImGui::GetContentRegionAvail().y;

    bool smallClock = totalWidth < 370.0F;
    const float clockHeight = smallClock ? 40.0F : 85.0F;

    const float spacing = 10.0F;
    const float totalContentWidth = smallClock ? clockWidth : 2 * clockWidth + spacing;
	const float topSpace = smallClock ? 0 : std::round((totalHeight - clockHeight) * 0.5f);

    // White Clock
    ImVec2 whiteMin = ImVec2(std::round(topLeft.x + (totalWidth - totalContentWidth) * 0.5f),
        std::round(topLeft.y + topSpace));
    ImVec2 whiteMax = ImVec2(std::round(whiteMin.x + clockWidth), 
        std::round(whiteMin.y + clockHeight));

    auto wCur = clockData_.wTimeCurMove + clockData_.wTimer.elapsedMs();
    auto bCur = clockData_.bTimeCurMove + clockData_.bTimer.elapsedMs();

    if (smallClock) {
        drawSmallClock(whiteMin, whiteMax, clockData_.wTimeLeftMs, wCur,
            clockData_.wEngineName, true, clockData_.wtm, analyze_);
    }
    else {
        drawClock(whiteMin, whiteMax, clockData_.wTimeLeftMs, wCur,
            clockData_.wEngineName, true, clockData_.wtm, analyze_);
    }

    // Black Clock
    ImVec2 blackMin = smallClock ?
        ImVec2(whiteMin.x, whiteMax.y + 10.0F) :
        ImVec2(whiteMax.x + spacing, whiteMin.y);
    ImVec2 blackMax = ImVec2(blackMin.x + clockWidth, blackMin.y + clockHeight);

    if (smallClock) {
        drawSmallClock(blackMin, blackMax, clockData_.bTimeLeftMs, bCur,
            clockData_.bEngineName, false, clockData_.wtm, analyze_);
    }
    else {
        drawClock(blackMin, blackMax, clockData_.bTimeLeftMs, bCur,
            clockData_.bEngineName, false, clockData_.wtm, analyze_);
    }

    ImGui::Dummy(ImVec2(0.0F, 0.0F));
}

