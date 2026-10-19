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

#include "imgui-button.h"
#include "i18n.h"

#include <functional>
#include <string>
#include "ui/qapla-ui.h"

#include <cmath>
#include <numbers>

using QaplaWindows::Translator;

namespace QaplaButton {

    constexpr float BORDER = 4.0F;

    using IconDrawCallback = std::function<void(QaplaUi::DrawList*, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size)>;

    static QaplaUi::Vec4 lerpVec4(const QaplaUi::Vec4& a, const QaplaUi::Vec4& b, float t) {
        return {
            std::lerp(a.x, b.x, t),
            std::lerp(a.y, b.y, t),
            std::lerp(a.z, b.z, t),
            std::lerp(a.w, b.w, t)
        };
    }

    static auto getFgColor(ButtonState state) {
        if (state == ButtonState::Disabled) {
            return QaplaUi::getColorU32(QaplaUi::StyleColor::TextDisabled);
        }
        bool hovered = QaplaUi::isItemHovered();
        bool active = QaplaUi::isItemActive() || state == ButtonState::Active;

        if (active || hovered) {
            return QaplaUi::getColorU32(QaplaUi::StyleColor::Text);
        }

        const QaplaUi::Vec4 a = QaplaUi::getStyleColorVec4(QaplaUi::StyleColor::Text);
        const QaplaUi::Vec4 b = QaplaUi::getStyleColorVec4(QaplaUi::StyleColor::TextDisabled);
        const QaplaUi::Vec4 m = lerpVec4(a, b, 0.8F);
        return QaplaUi::getColorU32(m);
    }

    void drawNew(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, [[maybe_unused]] QaplaUi::Vec2 size, ButtonState state) {
        constexpr float FIELD_SIZE = 3;
        auto xPos = topLeft.x + BORDER;
        auto yPos = topLeft.y + BORDER;
        auto color = getFgColor(state);
        for (int y = 0; y < 6; y++) {
            for (int x = 0; x < 6; x++) {
                if ((x + y) % 2 == 1) {
					continue; // Skip every second square to create a checkerboard pattern
				}
                list->addRectFilled(
                    QaplaUi::Vec2(xPos + x * FIELD_SIZE, yPos + y * FIELD_SIZE), 
                    QaplaUi::Vec2(xPos + (x + 1) * FIELD_SIZE, yPos + (y + 1) * FIELD_SIZE),
                    color);
            }
        }
    }

    void drawNow(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, [[maybe_unused]] QaplaUi::Vec2 size, ButtonState state) {
        auto yReduce = BORDER + 5.0F;
        auto thickness = 7.0F;
        auto arrowInset = 4.0F;
        auto xPos = topLeft.x + BORDER - 1.0F;
        auto yPos = topLeft.y + yReduce;
        auto color = getFgColor(state);
        list->addRectFilled(QaplaUi::Vec2(xPos, yPos), QaplaUi::Vec2(xPos + 3.0F, yPos + thickness), color);
        xPos += 3.0F;
        for (int i = 0; i < 8; i++) {
            list->addLine(QaplaUi::Vec2(xPos + i, yPos - arrowInset + i),
                QaplaUi::Vec2(xPos + i, yPos + thickness + arrowInset - i),
                color);
        }
		xPos += 9.0F;
        for (int i = 0; i <= 3; i++) {
            list->addLine(QaplaUi::Vec2(xPos + i, yPos + 3.0F - i),
                QaplaUi::Vec2(xPos + i, yPos + 5.0F + i), color);
            list->addLine(QaplaUi::Vec2(xPos + 7.0F - i, yPos + 3.0F - i),
                QaplaUi::Vec2(xPos + 7.0F - i, yPos + 5.0F + i), color);
        }
    }

    void drawStop(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        for (int i = 1; i <= 2; i++) {
            auto reduce = BORDER + static_cast<float>(i);
            auto color = getFgColor(state);
            list->addRect(
                QaplaUi::Vec2(topLeft.x + reduce, topLeft.y + reduce),
                QaplaUi::Vec2(topLeft.x + size.x - reduce, topLeft.y + size.y - reduce),
                color,
                0.0F,
                1.0F);
        }
    }

    void drawRect(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto color = getFgColor(state);
        for (int i = 0; i <= 1; i++) {
            list->addRect(
                QaplaUi::Vec2(topLeft.x + i, topLeft.y + i),
                QaplaUi::Vec2(topLeft.x + size.x - i, topLeft.y + size.y - i),
                color);
        }
    }

    void drawGrace(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto color = getFgColor(state);
        constexpr float reduce = 4.0F;
        auto rectSize = QaplaUi::Vec2(size.x - BORDER * 2 - reduce, size.y - BORDER * 2 - reduce);
        int centeredX = static_cast<int>((size.x - rectSize.x) / 2);
        auto rectPosX = topLeft.x + static_cast<float>(centeredX);
        drawRect(list, QaplaUi::Vec2(rectPosX, topLeft.y + BORDER), rectSize, state);

        // Parameter
        const float dotSize = 2.0F;   // 2x2 Pixel
        const float gap = 2.0F;       // Abstand zwischen Punkten
        const int count = 3;

        // Referenzpunkt: linke untere Ecke des Symbols
        float startX = rectPosX + 1;
        float startY = topLeft.y + size.y - BORDER - dotSize + 1.0F;

        for (int i = 0; i < count; ++i) {
            float x0 = startX + i*(dotSize + gap);
            list->addRectFilled(
                QaplaUi::Vec2(x0, startY),
                QaplaUi::Vec2(x0 + dotSize, startY + dotSize),
                color);
        }    
    }

    void drawAdd(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto color = getFgColor(state);
        constexpr int thickness = 3;
        constexpr int lengthPercentage = 60;

        int width = static_cast<int>(size.x);
        int height = static_cast<int>(size.y);

        int horizontalLength = width * lengthPercentage / 100;
        int verticalLength = height * lengthPercentage / 100;

        // Ensure symmetry
        if ((horizontalLength - thickness) % 2 != 0) {
            horizontalLength--;
        }
        if ((verticalLength - thickness) % 2 != 0) {
            verticalLength--;
        }

        int horizontalStartX = static_cast<int>(topLeft.x) + (width - horizontalLength) / 2;
        int horizontalStartY = static_cast<int>(topLeft.y) + (height - thickness) / 2;

        int verticalStartX = static_cast<int>(topLeft.x) + (width - thickness) / 2;
        int verticalStartY = static_cast<int>(topLeft.y) + (height - verticalLength) / 2;

        list->addRectFilled(
            QaplaUi::Vec2(static_cast<float>(horizontalStartX), static_cast<float>(horizontalStartY)),
            QaplaUi::Vec2(static_cast<float>(horizontalStartX + horizontalLength),
                static_cast<float>(horizontalStartY + thickness)),
            color
        );

        list->addRectFilled(
            QaplaUi::Vec2(static_cast<float>(verticalStartX), static_cast<float>(verticalStartY)),
            QaplaUi::Vec2(static_cast<float>(verticalStartX + thickness), 
                static_cast<float>(verticalStartY + verticalLength)),
            color
        );
    }

    void drawRemove(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto color = getFgColor(state);
        constexpr int thickness = 3;
        constexpr int lengthPercentage = 60;

        int width = static_cast<int>(size.x);
        int height = static_cast<int>(size.y);

        int horizontalLength = width * lengthPercentage / 100;
        int verticalLength = height * lengthPercentage / 100;

        // Ensure symmetry
        if ((horizontalLength - thickness) % 2 != 0) {
            horizontalLength--;
        }
        if ((verticalLength - thickness) % 2 != 0) {
            verticalLength--;
        }

        int horizontalStartX = static_cast<int>(topLeft.x) + (width - horizontalLength) / 2;
        int horizontalStartY = static_cast<int>(topLeft.y) + (height - thickness) / 2;

        list->addRectFilled(
            QaplaUi::Vec2(static_cast<float>(horizontalStartX), static_cast<float>(horizontalStartY)),
            QaplaUi::Vec2(static_cast<float>(horizontalStartX + horizontalLength), 
                static_cast<float>(horizontalStartY + thickness)),
            color
        );

    }

    void drawRestart (QaplaUi::DrawList* drawList, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        QaplaUi::Vec2 center = topLeft;
        center.x += size.x / 2;
        center.y += size.y / 2;
        float radius = std::min(size.x, size.y) / 2.0F - BORDER - 1;

        constexpr float pi = std::numbers::pi_v<float>;

        auto color = getFgColor(state);

        float startAngle = 1.5F * pi;   // 270° = 3/2 Pi
        float endAngle = startAngle + 1.5F * pi;  // 270° → 45° 
        drawList->pathArcTo(center, radius, startAngle, endAngle, 20);
        drawList->pathStroke(color, false, 1.5F);

        // Pfeilspitze
        auto arrowTip = QaplaUi::Vec2(center.x + cosf(pi) * radius,
            center.y + sinf(pi) * radius);

        for (int i = 0; i < 3; ++i) {
            float width = std::max(5.0F - 2.0F * static_cast<float>(i), 1.0F);
            float yOffset = -2.0F * i;
            auto p1 = QaplaUi::Vec2(arrowTip.x - width / 2.0F, arrowTip.y + yOffset);
            auto p2 = QaplaUi::Vec2(arrowTip.x + width / 2.0F, arrowTip.y + yOffset);
            drawList->addLine(p1, p2, color, 2.0F);
        }
    }

    void drawConfig(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto color = getFgColor(state);
        auto center = QaplaUi::Vec2(topLeft.x + size.x / 2.0F, topLeft.y + size.y / 2.0F);
        float radius = std::min(size.x, size.y) / 2.0F - BORDER - 1;
        constexpr float pi = std::numbers::pi_v<float>;
        
        float innerRadius = radius - 0.5F;
        list->addCircle(center, innerRadius, color, 0, 2.0F);
        // Draw the gear shape
        for (int i = 0; i < 8; ++i) {
            float angle = i * (pi / 4.0F); // 45 degrees
            float x = center.x + cos(angle) * radius;
            float y = center.y + sin(angle) * radius;
            list->addCircleFilled(QaplaUi::Vec2(x, y), 2.0F, color);
        }
	}

    void drawText(const std::string& text, QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto color = getFgColor(state);
        QaplaUi::Vec2 textSize = QaplaUi::calcTextSize(text.c_str());
        auto textPos = QaplaUi::Vec2(
            topLeft.x + std::round((size.x - textSize.x) * 0.5F) - 2.0F,
            topLeft.y - 2.0F
        );
        float fontSize = size.y * 1.0F;
        list->addText(QaplaUi::getFont(), fontSize, textPos, color, text.c_str());
	}

    void drawPlay(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, [[maybe_unused]] QaplaUi::Vec2 size, ButtonState state) {
		auto yReduce = BORDER + 5.0F;
        auto thickness = 7.0F;
        auto arrowInset = 4.0F;
        auto xPos = topLeft.x + BORDER - 1.0F;
        auto yPos = topLeft.y + yReduce;
        auto color = getFgColor(state);
        for (auto add : { 3.0F, 4.0F, 5.0F }) {
            list->addRectFilled(QaplaUi::Vec2(xPos, yPos), QaplaUi::Vec2(xPos + add - (add == 5.0F ? 0.0F : 1.0F), yPos + thickness), color);
            xPos += add;
        }
        for (int i = 0; i < 8; i++) {
            list->addLine(QaplaUi::Vec2(xPos + i, yPos - arrowInset + i), 
                QaplaUi::Vec2(xPos + i, yPos + thickness + arrowInset - i),
				color);
        }
    }

    void drawAnalyze(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, [[maybe_unused]] QaplaUi::Vec2 size, ButtonState state) {
        auto yReduce = BORDER + 4.0F;
        auto thickness = 5.0F;
        auto arrowInset = 3.0F;
        auto xPos = topLeft.x + BORDER;
        auto yPos = topLeft.y + yReduce;
        auto color = getFgColor(state);
        for (auto add : { 3.0F, 4.0F, 5.0F }) {
            list->addRectFilled(QaplaUi::Vec2(xPos, yPos), QaplaUi::Vec2(xPos + add - (add == 5.0F ? 0.0F : 1.0F), yPos + thickness), color);
            xPos += add;
        }
        for (int i = 0; i < 6; i++) {
            list->addLine(QaplaUi::Vec2(xPos + i, yPos - arrowInset + i), 
                QaplaUi::Vec2(xPos + i, yPos + thickness + arrowInset - i),
                color);
        }
        xPos = topLeft.x + BORDER;
        for (int i = 0; i < 3; i++) {
            list->addRectFilled(QaplaUi::Vec2(xPos + i * 4, yPos + thickness + 5),
                QaplaUi::Vec2(xPos + 2 + i * 4, yPos + thickness + 7),
				color);
        }
    }

    void drawAutoPlay(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto yReduce = BORDER + 2.0F;
        auto thickness = 5.0F;
        auto arrowInset = 3.0F;
        auto xPos = topLeft.x + BORDER;
		auto x2Pos = topLeft.x + size.x - BORDER;
        auto yPos = topLeft.y + yReduce;
		auto y2Pos = topLeft.y + yReduce + thickness + 3.0F;
        auto color = getFgColor(state);
        for (auto add : { 3.0F, 4.0F, 5.0F }) {
            list->addRectFilled(QaplaUi::Vec2(xPos, yPos), QaplaUi::Vec2(xPos + add - (add == 5.0F ? 0.0F : 1.0F) , yPos + thickness), color);
            list->addRectFilled(QaplaUi::Vec2(x2Pos, y2Pos), QaplaUi::Vec2(x2Pos - add + (add == 5.0F ? 0.0F : 1.0F), y2Pos + thickness), color);
            xPos += add;
			x2Pos -= add;
		}
        for (int i = 0; i < 6; i++) {
            list->addLine(QaplaUi::Vec2(xPos + i, yPos - arrowInset + i),
                QaplaUi::Vec2(xPos + i, yPos + thickness + arrowInset - i),
                color);
			list->addLine(QaplaUi::Vec2(x2Pos - i, y2Pos - arrowInset + i),
                QaplaUi::Vec2(x2Pos - i, y2Pos + thickness + arrowInset - i),
				color);
        }
    }

    void drawManualPlay(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto yReduce = BORDER + 2.0F;
        auto thickness = 5.0F;
        auto xPos = topLeft.x + BORDER + 3.0F;
        auto x2Pos = topLeft.x + size.x - BORDER - 2.0F;
        auto yPos = topLeft.y + yReduce;
        auto y2Pos = topLeft.y + yReduce + thickness + 3.0F;
        auto color = getFgColor(state);
        for (auto add : { 3.0F, 4.0F, 5.0F }) {
            list->addRectFilled(QaplaUi::Vec2(xPos, yPos), QaplaUi::Vec2(xPos + add - (add == 5.0F ? 0.0F : 1.0F), yPos + thickness), color);
            list->addRectFilled(QaplaUi::Vec2(x2Pos, y2Pos), QaplaUi::Vec2(x2Pos - add + (add == 5.0F ? 0.0F : 1.0F), y2Pos + thickness), color);
            xPos += add;
            x2Pos -= add;
        }
    }

    void drawAutoDetect(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto color = QaplaButton::getFgColor(state);
        auto center = QaplaUi::Vec2(topLeft.x + size.x / 2, topLeft.y + size.y / 2);
        float radius = std::min(size.x, size.y) / 2 - BORDER; // Leave some padding

        // Draw outer circle
        list->addCircle(center, radius, color, 32, 1.5F);

        // Draw inner circle
        list->addCircle(center, radius * 0.6F, color, 32, 1.5F);

        // Draw scanning line
        if (state == ButtonState::Animated) {
            float angle = static_cast<float>(QaplaUi::getTime()) * 2.0F;
            auto endPoint = QaplaUi::Vec2(
                center.x + cos(angle) * (radius + 2),
                center.y + sin(angle) * (radius + 2)
            );
            list->addLine(center, endPoint, color, 2.0F);
        }
    }

    void drawSwapEngines(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto xReduce = BORDER + 2.0F;
        auto thickness = 5.0F;
        auto arrowInset = 3.0F;
        auto yPos = topLeft.y + BORDER;
        auto y2Pos = topLeft.y + size.y - BORDER;
        auto xPos = topLeft.x + xReduce;
        auto x2Pos = topLeft.x + xReduce + thickness + 3.0F;
        
        // Colors: white for up arrow, black for down arrow
        auto whiteColor = getFgColor(state);
        auto blackColor = QaplaUi::col32(0, 0, 0, 255);
        
        auto add = 11.0F;

        // Draw white arrow pointing up 
        list->addRectFilled(QaplaUi::Vec2(xPos, yPos), QaplaUi::Vec2(xPos + thickness, yPos + add), whiteColor);
        yPos += add;
        for (int i = 0; i < 6; i++) {
            list->addLine(QaplaUi::Vec2(xPos - arrowInset + i, yPos + i),
                QaplaUi::Vec2(xPos + thickness + arrowInset - i, yPos + i),
                whiteColor);
        }
        
        // Draw black arrow pointing down (right side)
        list->addRectFilled(QaplaUi::Vec2(x2Pos, y2Pos), QaplaUi::Vec2(x2Pos + thickness, y2Pos - add), blackColor);
        y2Pos -= add;
        for (int i = 0; i < 6; i++) {
            list->addLine(QaplaUi::Vec2(x2Pos - arrowInset + i, y2Pos - i),
                QaplaUi::Vec2(x2Pos + thickness + arrowInset - i, y2Pos - i),
                blackColor);
        }
    }

    void drawSetup(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        
        // Left bottom: Square (7x7)
        float squareSize = 7.0F;
        float squareX = topLeft.x + 4.0F;
        float squareY = topLeft.y + size.y - 11.0F;
        list->addRectFilled(QaplaUi::Vec2(squareX, squareY), QaplaUi::Vec2(squareX + squareSize, squareY + squareSize), fgColor);
        
        // Right bottom: Circle (Radius 4)
        float circleCenterX = topLeft.x + size.x - 7.0F;
        float circleCenterY = topLeft.y + size.y - 8.0F;
        float circleRadius = 4.0F;
        list->addCircleFilled(QaplaUi::Vec2(circleCenterX, circleCenterY), circleRadius, fgColor);
        
        // Top middle: Triangle (Height 6, Base 10)
        float triangleBaseX = topLeft.x + size.x / 2.0F - 5.0F;
        float triangleTopY = topLeft.y + 4.0F;
        float triangleHeight = 6.0F;
        float triangleBaseWidth = 10.0F;
        
        // Draw filled triangle using horizontal lines
        for (int i = 0; i < static_cast<int>(triangleHeight); ++i) {
            float y = triangleTopY + i;
            float width = triangleBaseWidth * (1.0F - static_cast<float>(i) / triangleHeight);
            float leftX = triangleBaseX + (triangleBaseWidth - width) / 2.0F;
            float rightX = leftX + width;
            list->addLine(QaplaUi::Vec2(leftX, y), QaplaUi::Vec2(rightX, y), fgColor, 1.0F);
        }
    }

    /**
     * @brief Helper function to draw a cross (X shape) within a given area.
     * 
     * @param list           The ImGui draw list to draw on
     * @param topLeft        Top-left corner of the area
     * @param size           Size of the area
     * @param color          Color of the cross
     * @param lineThickness  Thickness of the cross lines
     * @param border         Border/padding around the cross
     */
    static void drawCross(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, QaplaUi::Color32 color, 
                         float lineThickness = 2.0F, float border = 6.0F) {
        int lineSize = static_cast<int>(std::min(size.x, size.y)) + 
                      (static_cast<int>(lineThickness) % 2 == 0 ? 1 : 0) - 
                      static_cast<int>(2 * border);

        float startX1 = topLeft.x + static_cast<float>((static_cast<int>(size.x) - lineSize) / 2);
        float startY1 = topLeft.y + static_cast<float>((static_cast<int>(size.y) - lineSize) / 2);
        float endX1 = startX1 + static_cast<float>(lineSize);
        float endY1 = startY1 + static_cast<float>(lineSize);

        list->addLine(QaplaUi::Vec2(startX1 - 0.5F, startY1 - 0.5F), QaplaUi::Vec2(endX1 + 1.0F, endY1 + 1.0F), color, lineThickness);
        list->addLine(QaplaUi::Vec2(startX1 - 0.5F, endY1 + 0.5F), QaplaUi::Vec2(endX1 + 1.0F, startY1 - 1.0F), color, lineThickness);
    }

    void drawClear(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        constexpr int LINE_THICKNESS = 2; 
        constexpr float CROSS_BORDER = 6.0F;
        auto color = getFgColor(state);
        drawCross(list, topLeft, size, color, LINE_THICKNESS, CROSS_BORDER);
    }

    void drawCancel(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto color = getFgColor(state);
        
        // Draw circle
        auto center = QaplaUi::Vec2(topLeft.x + size.x / 2.0F, topLeft.y + size.y / 2.0F);
        float radius = std::min(size.x, size.y) / 2.0F - BORDER + 1.0F;
        list->addCircle(center, radius, color, 0, 2.0F);
        
        // Draw small cross inside the circle
        constexpr float CROSS_LINE_THICKNESS = 2.0F;
        float crossSize = radius - 3.0F;
        auto crossTopLeft = QaplaUi::Vec2(center.x - crossSize / 2.0F - 1.0F, center.y - crossSize / 2.0F - 1.0F);
        auto crossSizeVec = QaplaUi::Vec2(crossSize, crossSize);
        drawCross(list, crossTopLeft, crossSizeVec, color, CROSS_LINE_THICKNESS, 0.0F);
    }

    void drawSave(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        
        constexpr float borderThickness = 2.0F;
        constexpr float rounding = 1.0F;
        auto outerRectTopLeft = QaplaUi::Vec2(topLeft.x + BORDER, topLeft.y + BORDER);
        auto outerRectBottomRight = QaplaUi::Vec2(topLeft.x + size.x - BORDER, topLeft.y + size.y - BORDER);
        
        // Draw outer rounded rectangle
        list->addRect(outerRectTopLeft, outerRectBottomRight, fgColor, rounding, borderThickness);
        
        // In the rectangle: A smaller filled rectangle
        constexpr float innerRectWidth = 6.0F;
        constexpr float innerRectHeight = 3.0F;
        auto innerRectTopLeft = QaplaUi::Vec2(outerRectTopLeft.x + 4.0F, outerRectTopLeft.y + 4.0F);
        auto innerRectBottomRight = QaplaUi::Vec2(innerRectTopLeft.x + innerRectWidth, innerRectTopLeft.y + innerRectHeight);
        list->addRectFilled(innerRectTopLeft, innerRectBottomRight, fgColor);
        
        // In the rectangle: A small filled circle at the bottom center
        constexpr float circleRadius = 2.5F;
        auto circleCenter = QaplaUi::Vec2(outerRectTopLeft.x + (outerRectBottomRight.x - outerRectTopLeft.x) / 2.0F,
                                     outerRectBottomRight.y - 5.0F);
        list->addCircleFilled(circleCenter, circleRadius, fgColor);
    }

    void drawOpen(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        
        // Document rectangle with 2px thick border
        constexpr float borderThickness = 2.0F;
        constexpr float rectReduce = 2.0F;
        constexpr float fold = 7.0F;

        auto docTopLeft = QaplaUi::Vec2(topLeft.x + BORDER + rectReduce, topLeft.y + BORDER);
        auto docBottomRight = QaplaUi::Vec2(topLeft.x + size.x - BORDER - rectReduce, topLeft.y + size.y - BORDER);

        // Top side
        list->addLine(QaplaUi::Vec2(docTopLeft.x, docTopLeft.y), QaplaUi::Vec2(docBottomRight.x  - fold + 1, docTopLeft.y), fgColor, borderThickness);
        // Left side
        list->addLine(QaplaUi::Vec2(docTopLeft.x, docTopLeft.y), QaplaUi::Vec2(docTopLeft.x, docBottomRight.y), fgColor, borderThickness);
        // Bottom side  
        list->addLine(QaplaUi::Vec2(docTopLeft.x, docBottomRight.y), QaplaUi::Vec2(docBottomRight.x, docBottomRight.y), fgColor, borderThickness);
        // Right side (stop before the fold area)
        list->addLine(QaplaUi::Vec2(docBottomRight.x, docBottomRight.y), QaplaUi::Vec2(docBottomRight.x, docTopLeft.y + fold), fgColor, borderThickness);
        
        // Triangle sides
        for (int i = 0; i <= static_cast<int>(fold); i++) {
            list->addLine(
                QaplaUi::Vec2(docBottomRight.x - fold + 1, docTopLeft.y + i - 1), 
                QaplaUi::Vec2(docBottomRight.x - fold + i + 1, docTopLeft.y + i - 1), fgColor, 1.0F);
        }
    }

    void drawTest(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        
        // Non-filled rectangle with 2px thick border
        constexpr float borderThickness = 2.0F;
        constexpr float rectReduce = 0.0F;

        auto rectTopLeft = QaplaUi::Vec2(topLeft.x + BORDER + rectReduce, topLeft.y + BORDER + rectReduce);
        auto rectBottomRight = QaplaUi::Vec2(topLeft.x + size.x - BORDER - rectReduce, topLeft.y + size.y - BORDER - rectReduce);

        // Draw rectangle outline
        list->addRect(rectTopLeft, rectBottomRight, fgColor, 0.0F, borderThickness);

        // Draw question mark in the middle
        auto center = QaplaUi::Vec2((rectTopLeft.x + rectBottomRight.x) / 2.0F, (rectTopLeft.y + rectBottomRight.y) / 2.0F);
        
        // Question mark parameters
        constexpr float questionMarkHeight = 10.0F;
        constexpr float questionMarkWidth = 6.0F;
        constexpr float dotRadius = 1.0F;
        constexpr float dotOffset = 3.0F;
        
        // Top arc of question mark
        auto arcCenter = QaplaUi::Vec2(center.x, center.y - questionMarkHeight / 2.0F + questionMarkWidth / 2.0F);
        constexpr float arcRadius = questionMarkWidth / 2.0F;
        list->addCircle(arcCenter, arcRadius, fgColor, 12, borderThickness - 0.5F);
        
        // Vertical line of question mark
        auto lineStart = QaplaUi::Vec2(center.x, arcCenter.y);
        auto lineEnd = QaplaUi::Vec2(center.x, center.y + questionMarkHeight / 2.0F - dotOffset);
        list->addLine(lineStart, lineEnd, fgColor, borderThickness - 0.5F);
        
        // Dot at the bottom
        auto dotCenter = QaplaUi::Vec2(center.x, center.y + questionMarkHeight / 2.0F);
        list->addCircleFilled(dotCenter, dotRadius, fgColor);
    }

    void drawFilter(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        constexpr float borderThickness = 2.0F;
        constexpr float triangleHeight = 10.0F;
        constexpr float triangleBaseWidth = 21.0F;
        constexpr float spoutThickness = 4.0F;
        
        // Draw funnel shape
        auto triangleTopLeft = QaplaUi::Vec2(topLeft.x + BORDER, topLeft.y + BORDER + 1.0F);
        auto triangleTopRight = QaplaUi::Vec2(topLeft.x + triangleBaseWidth, triangleTopLeft.y);
        auto triangleMiddle = QaplaUi::Vec2(
            (triangleTopLeft.x + triangleTopRight.x) / 2.0F, 
            triangleTopLeft.y + triangleHeight);

        list->addLine(triangleTopLeft, triangleTopRight, fgColor, borderThickness);
        list->addLine(triangleTopLeft, triangleMiddle, fgColor, borderThickness);
        list->addLine(triangleTopRight, triangleMiddle, fgColor, borderThickness);
        float rectLeft = std::trunc(triangleMiddle.x - spoutThickness / 2) + 1.0F;
        list->addRectFilled(
            QaplaUi::Vec2(rectLeft, triangleMiddle.y), 
            QaplaUi::Vec2(rectLeft + spoutThickness, topLeft.y + size.y - BORDER), 
            fgColor);

    }

    void drawCopy(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        constexpr float borderThickness = 2.0F;
        constexpr float gap = 3.0F;  // Gap between the two rectangles
        constexpr float rectangleWidth = 0.8F;
        
        // Calculate dimensions for the rectangles
        float rectWidth = (size.x - 2 * BORDER - gap) * rectangleWidth;
        float rectHeight = size.y - 2 * BORDER;
        
        // Back rectangle (L-shaped, left and below)
        float backRectLeft = topLeft.x + (size.x - rectWidth - gap) / 2.0F;
        float backRectTop = topLeft.y + BORDER + gap;
        
        // Draw left vertical line of L-shape
        list->addLine(
            QaplaUi::Vec2(backRectLeft, backRectTop),
            QaplaUi::Vec2(backRectLeft, topLeft.y + size.y - BORDER),
            fgColor, borderThickness);
        
        // Draw bottom horizontal line of L-shape  
        list->addLine(
            QaplaUi::Vec2(backRectLeft, topLeft.y + size.y - BORDER),
            QaplaUi::Vec2(backRectLeft + rectWidth, topLeft.y + size.y - BORDER),
            fgColor, borderThickness);
        
        // Front rectangle (complete rectangle, top and right)
        float frontRectLeft = backRectLeft + gap;
        float frontRectTop = topLeft.y + BORDER;
        float frontRectRight = frontRectLeft + rectWidth;
        float frontRectBottom = frontRectTop + rectHeight - gap;
        
        // Draw complete rectangle
        list->addRect(
            QaplaUi::Vec2(frontRectLeft, frontRectTop),
            QaplaUi::Vec2(frontRectRight, frontRectBottom),
            fgColor, 0.0F, borderThickness);
    }

    void drawPaste(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        constexpr float borderThickness = 2.0F;
        constexpr float widthReduce = 1.0F;
        
        // Calculate dimensions for the clipboard
        float clipboardWidth = size.x - 2 * BORDER - 2 * widthReduce;
        float clipboardHeight = size.y - 2 * BORDER;
        
        // Main clipboard rectangle
        float clipboardLeft = topLeft.x + BORDER + widthReduce;
        float clipboardTop = topLeft.y + BORDER + 3.0F; // Leave space for clip at top
        float clipboardRight = clipboardLeft + clipboardWidth;
        float clipboardBottom = clipboardTop + clipboardHeight - 3.0F;
        
        // Draw main clipboard rectangle
        list->addRect(
            QaplaUi::Vec2(clipboardLeft, clipboardTop),
            QaplaUi::Vec2(clipboardRight, clipboardBottom),
            fgColor, 0.0F, borderThickness);
        
        // Draw clip at the top (small rectangle centered)
        float clipWidth = clipboardWidth * 0.35F;
        float clipHeight = 4.0F;
        float clipLeft = clipboardLeft + (clipboardWidth - clipWidth) * 0.5F;
        float clipTop = topLeft.y + BORDER;
        
        // Draw clip as filled rectangle with rounded corners
        list->addRectFilled(
            QaplaUi::Vec2(clipLeft, clipTop),
            QaplaUi::Vec2(clipLeft + clipWidth, clipTop + clipHeight),
            fgColor, 1.0F);
    }

    void drawMore(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        constexpr float dotRadius = 2.0F;
        constexpr float dotSpacing = 4.5F;
        
        // Center the three dots
        float centerX = topLeft.x + size.x * 0.5F;
        float centerY = topLeft.y + size.y * 0.5F;
        
        // Draw three horizontal dots
        list->addCircleFilled(QaplaUi::Vec2(centerX - dotSpacing, centerY), dotRadius, fgColor);
        list->addCircleFilled(QaplaUi::Vec2(centerX, centerY), dotRadius, fgColor);
        list->addCircleFilled(QaplaUi::Vec2(centerX + dotSpacing, centerY), dotRadius, fgColor);
    }

    std::string showCommandPopup(const std::string& popupId, const std::vector<PopupCommand>& commands) {
        std::vector<QaplaUi::MenuItem> items;
        items.reserve(commands.size());
        for (const auto& command : commands) {
            items.push_back({
                .label = Translator::instance().translate("Button", command.name),
                // Every command stays clickable, as it always was here: whether a disabled one
                // should be is the board's question, not the menu's.
                .highlighted = command.state == ButtonState::Highlighted,
                .enabled = true
            });
        }
        auto selected = QaplaUi::popupMenu(popupId, items);
        if (!selected) {
            return {};
        }
        return commands[*selected].name;
    }

    void drawTimeClock(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto color = getFgColor(state);
        
        // Clock circle center and radius
        auto center = QaplaUi::Vec2(topLeft.x + size.x / 2.0F, topLeft.y + size.y / 2.0F);
        float radius = std::min(size.x, size.y) / 2.0F - BORDER + 1.0F;
        
        // Draw clock circle
        list->addCircle(center, radius, color, 32, 1.5F);
        
        // Clock showing 10 to 12 (11:50)
        // Hour hand pointing at 11:50 (between 11 and 12)
        // Minute hand pointing at 10 (50 minutes)
        
        // Hour hand (shorter, thicker) - pointing to 11:50
        // 11:50 = 11.833 hours, angle = (11.833/12) * 360 - 90 = 264.65 degrees
        float hourAngle = (11.833F / 12.0F) * 2.0F * std::numbers::pi_v<float> - std::numbers::pi_v<float> / 2.0F;
        float hourLength = radius * 0.5F;
        auto hourEnd = QaplaUi::Vec2(
            center.x + cos(hourAngle) * hourLength,
            center.y + sin(hourAngle) * hourLength
        );
        list->addLine(center, hourEnd, color, 2.0F);
        
        // Minute hand (longer, thinner) - pointing to 10 (50 minutes)
        // 50 minutes = (50/60) * 360 - 90 = 210 degrees
        float minuteAngle = (50.0F / 60.0F) * 2.0F * std::numbers::pi_v<float> - std::numbers::pi_v<float> / 2.0F;
        float minuteLength = radius * 0.7F;
        auto minuteEnd = QaplaUi::Vec2(
            center.x + cos(minuteAngle) * minuteLength,
            center.y + sin(minuteAngle) * minuteLength
        );
        list->addLine(center, minuteEnd, color, 1.5F);
        
        // Center dot
        list->addCircleFilled(center, 1.5F, color);
    }

    void drawLog(QaplaUi::DrawList* list, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 size, ButtonState state) {
        auto color = getFgColor(state);
        
        // Calculate rectangle with slight padding
        float padding = BORDER;
        auto rectMin = QaplaUi::Vec2(topLeft.x + padding, topLeft.y + padding);
        auto rectMax = QaplaUi::Vec2(topLeft.x + size.x - padding, topLeft.y + size.y - padding);
        float rounding = 2.0F;
        
        // Draw rounded rectangle (paper/document)
        list->addRect(rectMin, rectMax, color, rounding, 1.5F);
        
        // Draw three horizontal lines (text lines) inside the rectangle
        float lineMargin = (rectMax.x - rectMin.x) * 0.15F;  // 15% margin on each side
        float lineSpacing = (rectMax.y - rectMin.y) / 4.5F;
        for (int index = 0; index < 3; index++) {
            float lineY = rectMin.y + lineSpacing * (index + 1);
            list->addLine(
                QaplaUi::Vec2(rectMin.x + lineMargin, lineY),
                QaplaUi::Vec2(rectMax.x - lineMargin - 1.0F, lineY),
                color, 1.5F
            );
        }
        
    }

    bool drawIconButton(const std::string& id, const std::string& label, QaplaUi::Vec2 size, ButtonState state,
        const IconDrawCallback& iconDraw) {
        auto translatedLabel = Translator::instance().translate("Button", label);
        QaplaUi::IconPainter painter;
        if (iconDraw) {
            painter = [&iconDraw](QaplaUi::DrawList& drawList, QaplaUi::Vec2 topLeft, QaplaUi::Vec2 iconSize) {
                iconDraw(&drawList, topLeft, iconSize);
            };
        }
        return QaplaUi::iconButton(id, translatedLabel, size, static_cast<int>(state), painter);
    }

    QaplaUi::Vec2 calcIconButtonTotalSize(QaplaUi::Vec2 size, const char* label) {
        QaplaUi::Vec2 labelSize = QaplaUi::calcTextSize(label);
        float totalHeight = size.y + 4.0F + labelSize.y;
        float totalWidth = std::max(size.x, labelSize.x);
        return { totalWidth, totalHeight };
    }

    QaplaUi::Vec2 calcIconButtonsTotalSize(QaplaUi::Vec2 buttonSize, const std::vector<std::string>& labels) {
        auto totalSize = buttonSize;
        for (const auto& label : labels) {
            auto translatedLabel = Translator::instance().translate("Button", label);
            auto total = QaplaButton::calcIconButtonTotalSize(buttonSize, translatedLabel.c_str());
            totalSize.x = std::max(totalSize.x, total.x);
            totalSize.y = std::max(totalSize.y, total.y);
        }
        totalSize.x = std::round(totalSize.x);
        totalSize.y = std::round(totalSize.y);
		return totalSize;
    }

} // namespace QaplaButton
