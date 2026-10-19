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
#include <qapla-engine/types.h>

#include "ui/qapla-ui.h"

#include <cmath>
#include <string>
#include <string_view>

using namespace QaplaWindows;

constexpr float MIN_ENGINE_NAME_FONT_SIZE = 10.0F;

ImGuiClock::ImGuiClock() = default;
ImGuiClock::~ImGuiClock() = default;

namespace {

/**
 * @brief Calculates the text size at a given font size.
 */
QaplaUi::Vec2 textSizeAt(QaplaUi::Font font, float size, const std::string& str) {
    return QaplaUi::calcTextSize(font, size, str);
}

/**
 * @brief Calculates the text width at a given font size.
 */
float textWidthAt(QaplaUi::Font font, float size, const std::string& str) {
    return textSizeAt(font, size, str).x;
}

/**
 * @brief Draws the king icon if it's the side to move.
 */
void drawKingIcon(QaplaUi::DrawList* drawList, const QaplaUi::Vec2& topLeft, bool white, bool wtm) {
    if (wtm == white) {
        FontManager::drawPiece(drawList, white ? QaplaBasics::WHITE_KING : QaplaBasics::BLACK_KING,
            QaplaUi::Vec2(topLeft.x + 5, topLeft.y + 5), 30);
    }
}

/**
 * @brief Draws centered time text with colon alignment.
 */
void drawCenteredTimeText(QaplaUi::DrawList* drawList, QaplaUi::Font font, float size, 
                          const std::string& text, float xCenter, float& y, 
                          const QaplaUi::Style& style) {
    const QaplaUi::Color32 textCol = QaplaUi::getColorU32(QaplaUi::StyleColor::Text);
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
    
    const QaplaUi::Vec2 ext = textSizeAt(font, size, text);
    drawList->addText(font, size, QaplaUi::Vec2(x, y), textCol,
        text);
    y += ext.y + style.itemSpacing.y * 0.5F;
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
void drawEngineNameWithFit(QaplaUi::DrawList* drawList, QaplaUi::Font font, 
                          std::string_view engineName, float initialSize,
                          float xCenter, float y, float availableWidth) {
    const QaplaUi::Color32 textCol = QaplaUi::getColorU32(QaplaUi::StyleColor::Text);
    constexpr float minFontSize = MIN_ENGINE_NAME_FONT_SIZE;
    
    std::string nameStr(engineName);
    float fontSize = initialSize;
    
    // Step 1: Try to fit by reducing font size
    while (fontSize >= minFontSize) {
        float textWidth = textWidthAt(font, fontSize, nameStr);
        if (textWidth <= availableWidth) {
            // Fits! Draw it centered
            const float nameX = xCenter - textWidth * 0.5F;
            drawList->addText(font, fontSize, QaplaUi::Vec2(nameX, y), textCol,
                nameStr);
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
    drawList->addText(font, fontSize, QaplaUi::Vec2(nameX, y), textCol,
        nameStr);
}

/**
 * Draws a single-side chess clock (engine name, total time, current move time).
 * The MM:SS colon of both time strings is horizontally centered within the given width.
 *
 * @param topLeft   Top-left anchor of the clock area.
 * @param bottomRight Bottom-right anchor of the clock area. 
 * @param totalStr  Formatted remaining time, as decided by ClockModel.
 * @param moveStr   Formatted time for the current move, as decided by ClockModel.
 * @param engineName Name of the chess engine.
 * @param white     True if this is the white clock, false for black.
 * @param wtm       True if it is white's turn to move, false for black.
 */
void drawClock(const QaplaUi::Vec2& topLeft, const QaplaUi::Vec2& bottomRight, 
    const std::string& totalStr, const std::string& moveStr,
    std::string_view engineName, bool white, bool wtm)
{
    QaplaUi::DrawList* drawList = &QaplaUi::windowDrawList();
    QaplaUi::Font font = QaplaUi::getFont();
    const QaplaUi::Style& style = QaplaUi::getStyle();

    const float baseSize = QaplaUi::getFontSize();
    const float nameSize = baseSize * 1.3F;
    const float totalSize = baseSize * 1.6F;
    const float moveSize = baseSize * 1.0F;

    const float xCenter = topLeft.x + (bottomRight.x - topLeft.x) * 0.5F;
    const float availableWidth = bottomRight.x - topLeft.x - 10.0F; // 5px margin on each side
    float y = topLeft.y + 7.0F;

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
 * @param totalStr  Formatted remaining time, as decided by ClockModel.
 * @param moveStr   Formatted time for the current move, as decided by ClockModel.
 * @param white     True if this is the white clock, false for black.
 * @param wtm       True if it is white's turn to move, false for black.
 */
void drawSmallClock(const QaplaUi::Vec2& topLeft, const QaplaUi::Vec2& bottomRight, 
    const std::string& totalStr, const std::string& moveStr, bool white, bool wtm)
{
    QaplaUi::DrawList* drawList = &QaplaUi::windowDrawList();
    QaplaUi::Font font = QaplaUi::getFont();
    const QaplaUi::Style& style = QaplaUi::getStyle();

    const float baseSize = QaplaUi::getFontSize();
    const float totalSize = baseSize * 1.6F;
    const float moveSize = baseSize * 1.0F;

    const float xCenter = topLeft.x + (bottomRight.x - topLeft.x) * 0.5F;
    float y = topLeft.y + 7.0F;

    drawKingIcon(drawList, topLeft, white, wtm);
    
    // Total time (bigger)
    drawCenteredTimeText(drawList, font, totalSize, totalStr, xCenter, y, style);
    // Current move time (smaller, colon vertically under total's colon)
    drawCenteredTimeText(drawList, font, moveSize, moveStr, xCenter, y, style);
}

} // anonymous namespace

void ImGuiClock::draw() const {

    const ClockView view = model_.view();

    QaplaUi::Vec2 topLeft = QaplaUi::getCursorScreenPos();
	constexpr float clockWidth = 180.0F;

    const float totalWidth = QaplaUi::getContentRegionAvail().x;
	const float totalHeight = QaplaUi::getContentRegionAvail().y;

    bool smallClock = totalWidth < 370.0F;
    const float clockHeight = smallClock ? 40.0F : 85.0F;

    const float spacing = 10.0F;
    const float totalContentWidth = smallClock ? clockWidth : 2 * clockWidth + spacing;
	const float topSpace = smallClock ? 0 : std::round((totalHeight - clockHeight) * 0.5F);

    // White Clock
    auto whiteMin = QaplaUi::Vec2(std::round(topLeft.x + (totalWidth - totalContentWidth) * 0.5F),
        std::round(topLeft.y + topSpace));
    auto whiteMax = QaplaUi::Vec2(std::round(whiteMin.x + clockWidth), 
        std::round(whiteMin.y + clockHeight));

    if (smallClock) {
        drawSmallClock(whiteMin, whiteMax, view.whiteTotal, view.whiteMove,
            true, view.whiteToMove);
    }
    else {
        drawClock(whiteMin, whiteMax, view.whiteTotal, view.whiteMove,
            view.whiteEngineName, true, view.whiteToMove);
    }

    // Black Clock
    QaplaUi::Vec2 blackMin = smallClock ?
        QaplaUi::Vec2(whiteMin.x, whiteMax.y + 10.0F) :
        QaplaUi::Vec2(whiteMax.x + spacing, whiteMin.y);
    auto blackMax = QaplaUi::Vec2(blackMin.x + clockWidth, blackMin.y + clockHeight);

    if (smallClock) {
        drawSmallClock(blackMin, blackMax, view.blackTotal, view.blackMove,
            false, view.whiteToMove);
    }
    else {
        drawClock(blackMin, blackMax, view.blackTotal, view.blackMove,
            view.blackEngineName, false, view.whiteToMove);
    }

    QaplaUi::dummy(QaplaUi::Vec2(0.0F, 0.0F));
}
