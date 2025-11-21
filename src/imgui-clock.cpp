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
#include "string-helper.h"
#include "game-record.h"
#include "move-record.h"
#include "time-control.h"

#include <imgui.h>

#include <sstream>
#include <string>
#include <format>
#include <memory>

using QaplaTester::GameRecord;
using QaplaTester::MoveRecord;
using QaplaTester::GoLimits;

using namespace QaplaWindows;

constexpr float MIN_ENGINE_NAME_FONT_SIZE = 10.0F;

ImGuiClock::ImGuiClock() = default;
ImGuiClock::~ImGuiClock() = default;

void ImGuiClock::setFromGameRecord(const GameRecord& gameRecord) {
    auto [modification, update] = gameRecordTracker_.checkModification(gameRecord.getChangeTracker());
    if (!update) {
        return;
    }
    gameRecordTracker_.updateFrom(gameRecord.getChangeTracker());

    const auto& wtc = gameRecord.getWhiteTimeControl();
    const auto& btc = gameRecord.getBlackTimeControl();
    if (!wtc.isValid() || !btc.isValid()) {
        return;
    }
    auto [whiteTime, blackTime] = gameRecord.timeUsed();
    auto nextMoveIndex = gameRecord.nextMoveIndex();
    auto halfMoves = gameRecord.halfmoveNoAtPly(nextMoveIndex);
    GoLimits goLimits = createGoLimits(wtc, btc,
        halfMoves, whiteTime, blackTime, gameRecord.isWhiteToMove());

    if (modification) {
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
        if (!stopped_) {
            clockData_.wTimer.start();
        } else {
            clockData_.bTimeCurMove = moveRecord.timeMs;
            // bTimeLeftMs has the time after current move.
            clockData_.bTimeLeftMs += moveRecord.timeMs;
            clockData_.bEngineName = moveRecord.engineName_;
        }
    } else {
        if (!stopped_) {
            clockData_.bTimer.start();
        } else {
            clockData_.wTimeCurMove = moveRecord.timeMs;
            // wTimeLeftMs has the time after current move.
            clockData_.wTimeLeftMs += moveRecord.timeMs;
            clockData_.wEngineName = moveRecord.engineName_;
        }
    }
}


void ImGuiClock::setFromMoveRecord(const MoveRecord& moveRecord, uint32_t playerIndex) {
    if (stopped_) {
        return;
    }
    if (!moveRecord.ponderMove.empty()) {
        // Time used from pondering is not relevant
        return;
    }
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
            if (stopped_) {
                clockData_.wTimer.reset(); 
            } else {
                clockData_.wTimer.start();
            }
        }
        clockData_.wTimeCurMove = cur;
        clockData_.wEngineName = analyze_ ? "Analyze" : moveRecord.engineName_;
    } 
    else {
        if (cur > clockData_.bTimeCurMove) {
            if (stopped_) {
                clockData_.bTimer.reset(); 
            } else {
                clockData_.bTimer.start();
            }
        }
        clockData_.bTimeCurMove = cur;
        clockData_.bEngineName = analyze_ ? "Analyze" : moveRecord.engineName_;
    }
}

namespace {

/**
 * @brief Calculates the text size at a given font size.
 */
ImVec2 textSizeAt(ImFont* font, float size, const std::string& str) {
    return font->CalcTextSizeA(size, FLT_MAX, 0.0F, str.c_str(), str.c_str() + str.size());
}

/**
 * @brief Calculates the text width at a given font size.
 */
float textWidthAt(ImFont* font, float size, const std::string& str) {
    return textSizeAt(font, size, str).x;
}

/**
 * @brief Prepares the time strings for display.
 * @param totalMs Total remaining time in milliseconds.
 * @param moveMs Time for the current move in milliseconds.
 * @param analyze Whether in analyze mode.
 * @return Pair of formatted time strings (total, move).
 */
std::pair<std::string, std::string> prepareTimeStrings(uint64_t totalMs, uint64_t moveMs, bool analyze) {
    uint64_t adjustedTotal = totalMs - std::min(totalMs, moveMs);
    if (!analyze) {
        adjustedTotal += 999; // Add 999ms to compensate for formatMs truncating to full seconds
    } else {
        adjustedTotal = moveMs;
    }
    return {
        QaplaHelpers::formatMs(adjustedTotal, 0),
        QaplaHelpers::formatMs(moveMs, 0)
    };
}

/**
 * @brief Draws the king icon if it's the side to move.
 */
void drawKingIcon(ImDrawList* drawList, const ImVec2& topLeft, bool white, bool wtm) {
    if (wtm == white) {
        FontManager::drawPiece(drawList, white ? QaplaBasics::WHITE_KING : QaplaBasics::BLACK_KING,
            ImVec2(topLeft.x + 5, topLeft.y + 5), 30);
    }
}

/**
 * @brief Draws centered time text with colon alignment.
 */
void drawCenteredTimeText(ImDrawList* drawList, ImFont* font, float size, 
                          const std::string& text, float xCenter, float& y, 
                          const ImGuiStyle& style) {
    const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);
    const size_t colonIdx = text.find_last_of(':');
    float x;
    
    if (colonIdx == std::string::npos) {
        // Fallback: center entire text
        const float w = textWidthAt(font, size, text);
        x = xCenter - w * 0.5F;
    } else {
        // Center at the colon's visual midpoint
        const float leftWidth = textWidthAt(font, size, text.substr(0, colonIdx));
        const float colonWidth = textWidthAt(font, size, ":");
        x = xCenter - (leftWidth + colonWidth * 0.5F);
    }
    
    const ImVec2 ext = textSizeAt(font, size, text);
    drawList->AddText(font, size, ImVec2(x, y), textCol,
        text.c_str(), text.c_str() + text.size());
    y += ext.y + style.ItemSpacing.y * 0.5F;
}

/**
 * @brief Draws the engine name with automatic font size adjustment and truncation.
 * 
 * This function ensures the engine name fits within the available width by:
 * 1. Reducing font size down to MIN_ENGINE_NAME_FONT_SIZE
 * 2. Truncating text from the right if it still doesn't fit
 * 
 * @param drawList ImGui draw list
 * @param font Current ImGui font
 * @param engineName Name of the chess engine
 * @param initialSize Initial font size to try
 * @param xCenter Horizontal center position
 * @param y Vertical position
 * @param availableWidth Available width for the text
 */
void drawEngineNameWithFit(ImDrawList* drawList, ImFont* font, 
                          std::string_view engineName, float initialSize,
                          float xCenter, float y, float availableWidth) {
    const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);
    constexpr float minFontSize = MIN_ENGINE_NAME_FONT_SIZE;
    
    std::string nameStr(engineName);
    float fontSize = initialSize;
    
    // Step 1: Try to fit by reducing font size
    while (fontSize >= minFontSize) {
        float textWidth = textWidthAt(font, fontSize, nameStr);
        if (textWidth <= availableWidth) {
            // Fits! Draw it centered
            const float nameX = xCenter - textWidth * 0.5F;
            drawList->AddText(font, fontSize, ImVec2(nameX, y), textCol,
                nameStr.c_str(), nameStr.c_str() + nameStr.size());
            return;
        }
        fontSize -= 1.0F;
    }
    
    // Step 2: At minimum font size, truncate text if needed
    fontSize = minFontSize;
    float textWidth = textWidthAt(font, fontSize, nameStr);
    
    if (textWidth > availableWidth) {
        // Need to truncate - binary search for the right length
        size_t left = 0;
        size_t right = nameStr.size();
        size_t bestLen = 0;
        
        while (left <= right && right > 0) {
            size_t mid = (left + right) / 2;
            std::string truncated = nameStr.substr(0, mid);
            float width = textWidthAt(font, fontSize, truncated);
            
            if (width <= availableWidth) {
                bestLen = mid;
                left = mid + 1;
            } else {
                if (mid == 0) break;
                right = mid - 1;
            }
        }
        
        nameStr = nameStr.substr(0, bestLen);
        textWidth = textWidthAt(font, fontSize, nameStr);
    }
    
    // Draw the (possibly truncated) text
    const float nameX = xCenter - textWidth * 0.5F;
    drawList->AddText(font, fontSize, ImVec2(nameX, y), textCol,
        nameStr.c_str(), nameStr.c_str() + nameStr.size());
}

} // anonymous namespace

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
 * @param analyze   True if in analyze mode.
 */
static void drawClock(const ImVec2& topLeft, const ImVec2& bottomRight, 
    uint64_t totalMs, uint64_t moveMs, std::string_view engineName, bool white, bool wtm, bool analyze)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    const ImGuiStyle& style = ImGui::GetStyle();

    const float baseSize = ImGui::GetFontSize();
    const float nameSize = baseSize * 1.3F;
    const float totalSize = baseSize * 1.6F;
    const float moveSize = baseSize * 1.0F;

    const float xCenter = topLeft.x + (bottomRight.x - topLeft.x) * 0.5F;
    const float availableWidth = bottomRight.x - topLeft.x - 10.0F; // 5px margin on each side
    float y = topLeft.y + 7.0F;

    auto [totalStr, moveStr] = prepareTimeStrings(totalMs, moveMs, analyze);

    ImGuiControls::drawBoxWithShadow(topLeft, bottomRight);
    drawKingIcon(drawList, topLeft, white, wtm);
    
    // Total time (bigger)
    drawCenteredTimeText(drawList, font, totalSize, totalStr, xCenter, y, style);
    // Current move time (smaller, colon vertically under total's colon)
    drawCenteredTimeText(drawList, font, moveSize, moveStr, xCenter, y, style);

    // Engine name (centered, with auto-sizing and truncation)
    drawEngineNameWithFit(drawList, font, engineName, nameSize, xCenter, y, availableWidth);
}

/**
 * Draws a single-side chess clock in compact mode (no engine name).
 * The MM:SS colon of both time strings is horizontally centered within the given width.
 *
 * @param topLeft   Top-left anchor of the clock area.
 * @param bottomRight Bottom-right anchor of the clock area. 
 * @param totalMs   Total remaining time in milliseconds.
 * @param moveMs    Time for the current move in milliseconds.
 * @param white     True if this is the white clock, false for black.
 * @param wtm       True if it is white's turn to move, false for black.
 * @param analyze   True if in analyze mode.
 */
static void drawSmallClock(const ImVec2& topLeft, const ImVec2& bottomRight, 
    uint64_t totalMs, uint64_t moveMs, bool white, bool wtm, bool analyze)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    const ImGuiStyle& style = ImGui::GetStyle();

    const float baseSize = ImGui::GetFontSize();
    const float totalSize = baseSize * 1.6F;
    const float moveSize = baseSize * 1.0F;

    const float xCenter = topLeft.x + (bottomRight.x - topLeft.x) * 0.5F;
    float y = topLeft.y + 7.0F;

    auto [totalStr, moveStr] = prepareTimeStrings(totalMs, moveMs, analyze);

    drawKingIcon(drawList, topLeft, white, wtm);
    
    // Total time (bigger)
    drawCenteredTimeText(drawList, font, totalSize, totalStr, xCenter, y, style);
    // Current move time (smaller, colon vertically under total's colon)
    drawCenteredTimeText(drawList, font, moveSize, moveStr, xCenter, y, style);
}

void ImGuiClock::draw() const {

    ImVec2 topLeft = ImGui::GetCursorScreenPos();
	constexpr float clockWidth = 180.0F;

    const float totalWidth = ImGui::GetContentRegionAvail().x;
	const float totalHeight = ImGui::GetContentRegionAvail().y;

    bool smallClock = totalWidth < 370.0F;
    const float clockHeight = smallClock ? 40.0F : 85.0F;

    const float spacing = 10.0F;
    const float totalContentWidth = smallClock ? clockWidth : 2 * clockWidth + spacing;
	const float topSpace = smallClock ? 0 : std::round((totalHeight - clockHeight) * 0.5F);

    // White Clock
    auto whiteMin = ImVec2(std::round(topLeft.x + (totalWidth - totalContentWidth) * 0.5F),
        std::round(topLeft.y + topSpace));
    auto whiteMax = ImVec2(std::round(whiteMin.x + clockWidth), 
        std::round(whiteMin.y + clockHeight));

    auto wCur = clockData_.wTimeCurMove + clockData_.wTimer.elapsedMs();
    auto bCur = clockData_.bTimeCurMove + clockData_.bTimer.elapsedMs();

    if (smallClock) {
        drawSmallClock(whiteMin, whiteMax, clockData_.wTimeLeftMs, wCur,
            true, clockData_.wtm, analyze_);
    }
    else {
        drawClock(whiteMin, whiteMax, clockData_.wTimeLeftMs, wCur,
            clockData_.wEngineName, true, clockData_.wtm, analyze_);
    }

    // Black Clock
    ImVec2 blackMin = smallClock ?
        ImVec2(whiteMin.x, whiteMax.y + 10.0F) :
        ImVec2(whiteMax.x + spacing, whiteMin.y);
    auto blackMax = ImVec2(blackMin.x + clockWidth, blackMin.y + clockHeight);

    if (smallClock) {
        drawSmallClock(blackMin, blackMax, clockData_.bTimeLeftMs, bCur,
            false, clockData_.wtm, analyze_);
    }
    else {
        drawClock(blackMin, blackMax, clockData_.bTimeLeftMs, bCur,
            clockData_.bEngineName, false, clockData_.wtm, analyze_);
    }

    ImGui::Dummy(ImVec2(0.0F, 0.0F));
}
