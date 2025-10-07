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

#include <functional>
#include <string>
#include <imgui.h>
#include <math.h>

namespace QaplaButton {

    constexpr float BORDER = 4.0F;

    using IconDrawCallback = std::function<void(ImDrawList*, ImVec2 topLeft, ImVec2 size)>;

    static auto getBgColor(ButtonState state) {
        if (state == ButtonState::Disabled) 
            return ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		if (ImGui::IsItemActive() || state == ButtonState::Active) 
            return ImGui::GetColorU32(ImGuiCol_ButtonActive);

		if (ImGui::IsItemHovered()) return ImGui::GetColorU32(ImGuiCol_ButtonHovered);
		return ImGui::GetColorU32(ImGuiCol_Button);
    }

    static auto getTextColor(ButtonState state) {
        if (state == ButtonState::Disabled) return ImGui::GetColorU32(ImGuiCol_TextDisabled);
		return ImGui::GetColorU32(ImGuiCol_Text);
    }

    static ImVec4 lerpVec4(const ImVec4& a, const ImVec4& b, float t) {
        return ImVec4(
            std::lerp(a.x, b.x, t),
            std::lerp(a.y, b.y, t),
            std::lerp(a.z, b.z, t),
            std::lerp(a.w, b.w, t)
        );
    }

    static auto getFgColor(ButtonState state) {
        if (state == ButtonState::Disabled) return ImGui::GetColorU32(ImGuiCol_TextDisabled);
        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive() || state == ButtonState::Active;

        if (active || hovered)
            return ImGui::GetColorU32(ImGuiCol_Text);

        const ImVec4 a = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        const ImVec4 b = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
        const ImVec4 m = lerpVec4(a, b, 0.80f);
        return ImGui::GetColorU32(m);
    }

    void drawNew(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        constexpr float FIELD_SIZE = 3;
        auto xPos = topLeft.x + BORDER;
        auto yPos = topLeft.y + BORDER;
        auto color = getFgColor(state);
        for (int y = 0; y < 6; y++) {
            for (int x = 0; x < 6; x++) {
                if ((x + y) % 2 == 1) {
					continue; // Skip every second square to create a checkerboard pattern
				}
                list->AddRectFilled(
                    ImVec2(xPos + x * FIELD_SIZE, yPos + y * FIELD_SIZE), 
                    ImVec2(xPos + (x + 1) * FIELD_SIZE, yPos + (y + 1) * FIELD_SIZE),
                    color);
            }
        }
    }

    void drawNow(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto yReduce = BORDER + 5.0F;
        auto thickness = 7.0F;
        auto arrowInset = 4.0F;
        auto xPos = topLeft.x + BORDER - 1.0F;
        auto yPos = topLeft.y + yReduce;
        auto color = getFgColor(state);
        list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + 3.0F, yPos + thickness), color);
        xPos += 3.0F;
        for (int i = 0; i < 8; i++) {
            list->AddLine(ImVec2(xPos + i, yPos - arrowInset + i),
                ImVec2(xPos + i, yPos + thickness + arrowInset - i),
                color);
        }
		xPos += 9.0F;
        for (int i = 0; i <= 3; i++) {
            list->AddLine(ImVec2(xPos + i, yPos + 3.0F - i),
                ImVec2(xPos + i, yPos + 5.0F + i), color);
            list->AddLine(ImVec2(xPos + 7.0F - i, yPos + 3.0F - i),
                ImVec2(xPos + 7.0F - i, yPos + 5.0F + i), color);
        }
    }

    void drawStop(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        for (int i = 1; i <= 2; i++) {
            auto reduce = BORDER + static_cast<float>(i);
            auto color = getFgColor(state);
            list->AddRect(
                ImVec2(topLeft.x + reduce, topLeft.y + reduce),
                ImVec2(topLeft.x + size.x - reduce, topLeft.y + size.y - reduce),
                color,
                0.0F,
                ImDrawFlags_None,
                1.0F);
        }
    }

    void drawRect(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto color = getFgColor(state);
        for (int i = 0; i <= 1; i++) {
            list->AddRect(
                ImVec2(topLeft.x + i, topLeft.y + i),
                ImVec2(topLeft.x + size.x - i, topLeft.y + size.y - i),
                color);
        }
    }

    void drawGrace(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto color = getFgColor(state);
        constexpr float reduce = 4.0F;
        auto rectSize = ImVec2(size.x - BORDER * 2 - reduce, size.y - BORDER * 2 - reduce);
        int centeredX = static_cast<int>((size.x - rectSize.x) / 2);
        auto rectPosX = topLeft.x + static_cast<float>(centeredX);
        drawRect(list, ImVec2(rectPosX, topLeft.y + BORDER), rectSize, state);

        // Parameter
        const float dotSize = 2.0F;   // 2x2 Pixel
        const float gap = 2.0F;       // Abstand zwischen Punkten
        const int count = 3;

        // Referenzpunkt: linke untere Ecke des Symbols
        float startX = rectPosX + 1;
        float startY = topLeft.y + size.y - BORDER - dotSize + 1.0F;

        for (int i = 0; i < count; ++i) {
            float x0 = startX + i*(dotSize + gap);
            list->AddRectFilled(
                ImVec2(x0, startY),
                ImVec2(x0 + dotSize, startY + dotSize),
                color);
        }    
    }

    void drawAdd(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto color = getFgColor(state);
        constexpr int thickness = 3;
        constexpr int lengthPercentage = 60;

        int width = static_cast<int>(size.x);
        int height = static_cast<int>(size.y);

        int horizontalLength = width * lengthPercentage / 100;
        int verticalLength = height * lengthPercentage / 100;

        // Ensure symmetry
        if ((horizontalLength - thickness) % 2 != 0) horizontalLength--; 
        if ((verticalLength - thickness) % 2 != 0) verticalLength--; 

        int horizontalStartX = static_cast<int>(topLeft.x) + (width - horizontalLength) / 2;
        int horizontalStartY = static_cast<int>(topLeft.y) + (height - thickness) / 2;

        int verticalStartX = static_cast<int>(topLeft.x) + (width - thickness) / 2;
        int verticalStartY = static_cast<int>(topLeft.y) + (height - verticalLength) / 2;

        list->AddRectFilled(
            ImVec2(static_cast<float>(horizontalStartX), static_cast<float>(horizontalStartY)),
            ImVec2(static_cast<float>(horizontalStartX + horizontalLength),
                static_cast<float>(horizontalStartY + thickness)),
            color
        );

        list->AddRectFilled(
            ImVec2(static_cast<float>(verticalStartX), static_cast<float>(verticalStartY)),
            ImVec2(static_cast<float>(verticalStartX + thickness), 
                static_cast<float>(verticalStartY + verticalLength)),
            color
        );
    }

    void drawRemove(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto color = getFgColor(state);
        constexpr int thickness = 3;
        constexpr int lengthPercentage = 60;

        int width = static_cast<int>(size.x);
        int height = static_cast<int>(size.y);

        int horizontalLength = width * lengthPercentage / 100;
        int verticalLength = height * lengthPercentage / 100;

        // Ensure symmetry
        if ((horizontalLength - thickness) % 2 != 0) horizontalLength--;
        if ((verticalLength - thickness) % 2 != 0) verticalLength--;

        int horizontalStartX = static_cast<int>(topLeft.x) + (width - horizontalLength) / 2;
        int horizontalStartY = static_cast<int>(topLeft.y) + (height - thickness) / 2;

        list->AddRectFilled(
            ImVec2(static_cast<float>(horizontalStartX), static_cast<float>(horizontalStartY)),
            ImVec2(static_cast<float>(horizontalStartX + horizontalLength), 
                static_cast<float>(horizontalStartY + thickness)),
            color
        );

    }


    void drawRestart (ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        ImVec2 center = topLeft;
        center.x += size.x / 2;
        center.y += size.y / 2;
        float radius = std::min(size.x, size.y) / 2.0F - BORDER - 1;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        constexpr float pi = 3.14159265358979323846f;

        auto color = getFgColor(state);

        float startAngle = 1.5f * pi;   // 270° = 3/2 Pi
        float endAngle = startAngle + 1.5f * pi;  // 270° → 45° 
        drawList->PathArcTo(center, radius, startAngle, endAngle, 20);
        drawList->PathStroke(color, false, 1.5f);

        // Pfeilspitze
        ImVec2 arrowTip = ImVec2(center.x + cosf(pi) * radius,
            center.y + sinf(pi) * radius);

        for (int i = 0; i < 3; ++i) {
            float width = std::max(5.0F - 2.0F * static_cast<float>(i), 1.0F);
            float yOffset = -2.0F * i;
            ImVec2 p1 = ImVec2(arrowTip.x - width / 2.0F, arrowTip.y + yOffset);
            ImVec2 p2 = ImVec2(arrowTip.x + width / 2.0F, arrowTip.y + yOffset);
            drawList->AddLine(p1, p2, color, 2.0F);
        }
    }

    void drawConfig(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto color = getFgColor(state);
        ImVec2 center = ImVec2(topLeft.x + size.x / 2.0F, topLeft.y + size.y / 2.0F);
        float radius = std::min(size.x, size.y) / 2.0F - BORDER - 1;
        constexpr float pi = 3.14159265358979323846f;
        
        float innerRadius = radius - 0.5f;
        list->AddCircle(center, innerRadius, color, 0, 2.0F);
        // Draw the gear shape
        for (int i = 0; i < 8; ++i) {
            float angle = i * (pi / 4.0F); // 45 degrees
            float x = center.x + cos(angle) * radius;
            float y = center.y + sin(angle) * radius;
            list->AddCircleFilled(ImVec2(x, y), 2.0F, color);
        }
	}

    void drawText(const std::string& text, ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto color = getFgColor(state);
        ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
        ImVec2 textPos = ImVec2(
            topLeft.x + std::round((size.x - textSize.x) * 0.5f) - 2.0F,
            topLeft.y - 2.0F
        );
        float fontSize = size.y * 1.0F;
        list->AddText(ImGui::GetFont(), fontSize, textPos, color, text.c_str());
	}

    void drawPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
		auto yReduce = BORDER + 5.0F;
        auto thickness = 7.0F;
        auto arrowInset = 4.0F;
        auto xPos = topLeft.x + BORDER - 1.0F;
        auto yPos = topLeft.y + yReduce;
        auto color = getFgColor(state);
        for (auto add : { 3.0F, 4.0F, 5.0F }) {
            list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + add - (add == 5.0F ? 0.0F : 1.0F), yPos + thickness), color);
            xPos += add;
        }
        for (int i = 0; i < 8; i++) {
            list->AddLine(ImVec2(xPos + i, yPos - arrowInset + i), 
                ImVec2(xPos + i, yPos + thickness + arrowInset - i),
				color);
        }
    }

    void drawAnalyze(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto yReduce = BORDER + 4.0F;
        auto thickness = 5.0F;
        auto arrowInset = 3.0F;
        auto xPos = topLeft.x + BORDER;
        auto yPos = topLeft.y + yReduce;
        auto color = getFgColor(state);
        for (auto add : { 3.0F, 4.0F, 5.0F }) {
            list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + add - (add == 5.0F ? 0.0F : 1.0F), yPos + thickness), color);
            xPos += add;
        }
        for (int i = 0; i < 6; i++) {
            list->AddLine(ImVec2(xPos + i, yPos - arrowInset + i), 
                ImVec2(xPos + i, yPos + thickness + arrowInset - i),
                color);
        }
        xPos = topLeft.x + BORDER;
        for (int i = 0; i < 3; i++) {
            list->AddRectFilled(ImVec2(xPos + i * 4, yPos + thickness + 5),
                ImVec2(xPos + 2 + i * 4, yPos + thickness + 7),
				color);
        }
    }

    void drawAutoPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto yReduce = BORDER + 2.0F;
        auto thickness = 5.0F;
        auto arrowInset = 3.0F;
        auto xPos = topLeft.x + BORDER;
		auto x2Pos = topLeft.x + size.x - BORDER;
        auto yPos = topLeft.y + yReduce;
		auto y2Pos = topLeft.y + yReduce + thickness + 3.0F;
        auto color = getFgColor(state);
        for (auto add : { 3.0F, 4.0F, 5.0F }) {
            list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + add - (add == 5.0F ? 0.0F : 1.0F) , yPos + thickness), color);
            list->AddRectFilled(ImVec2(x2Pos, y2Pos), ImVec2(x2Pos - add + (add == 5.0F ? 0.0F : 1.0F), y2Pos + thickness), color);
            xPos += add;
			x2Pos -= add;
		}
        for (int i = 0; i < 6; i++) {
            list->AddLine(ImVec2(xPos + i, yPos - arrowInset + i),
                ImVec2(xPos + i, yPos + thickness + arrowInset - i),
                color);
			list->AddLine(ImVec2(x2Pos - i, y2Pos - arrowInset + i),
                ImVec2(x2Pos - i, y2Pos + thickness + arrowInset - i),
				color);
        }
    }

    void drawManualPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto yReduce = BORDER + 2.0F;
        auto thickness = 5.0F;
        auto xPos = topLeft.x + BORDER + 3.0F;
        auto x2Pos = topLeft.x + size.x - BORDER - 2.0F;
        auto yPos = topLeft.y + yReduce;
        auto y2Pos = topLeft.y + yReduce + thickness + 3.0F;
        auto color = getFgColor(state);
        for (auto add : { 3.0F, 4.0F, 5.0F }) {
            list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + add - (add == 5.0F ? 0.0F : 1.0F), yPos + thickness), color);
            list->AddRectFilled(ImVec2(x2Pos, y2Pos), ImVec2(x2Pos - add + (add == 5.0F ? 0.0F : 1.0F), y2Pos + thickness), color);
            xPos += add;
            x2Pos -= add;
        }
    }

    void drawAutoDetect(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto color = QaplaButton::getFgColor(state);
        ImVec2 center = ImVec2(topLeft.x + size.x / 2, topLeft.y + size.y / 2);
        float radius = std::min(size.x, size.y) / 2 - BORDER; // Leave some padding

        // Draw outer circle
        list->AddCircle(center, radius, color, 32, 1.5f);

        // Draw inner circle
        list->AddCircle(center, radius * 0.6f, color, 32, 1.5f);

        // Draw scanning line
        if (state == ButtonState::Animated) {
            float angle = static_cast<float>(ImGui::GetTime()) * 2.0F;
            ImVec2 endPoint = ImVec2(
                center.x + cos(angle) * (radius + 2),
                center.y + sin(angle) * (radius + 2)
            );
            list->AddLine(center, endPoint, color, 2.0F);
        }
    }

    void drawSwapEngines(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto xReduce = BORDER + 2.0F;
        auto thickness = 5.0F;
        auto arrowInset = 3.0F;
        auto yPos = topLeft.y + BORDER;
        auto y2Pos = topLeft.y + size.y - BORDER;
        auto xPos = topLeft.x + xReduce;
        auto x2Pos = topLeft.x + xReduce + thickness + 3.0F;
        
        // Colors: white for up arrow, black for down arrow
        auto whiteColor = getFgColor(state);
        auto blackColor = IM_COL32(0, 0, 0, 255);
        
        auto add = 11.0F;

        // Draw white arrow pointing up 
        list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + thickness, yPos + add), whiteColor);
        yPos += add;
        for (int i = 0; i < 6; i++) {
            list->AddLine(ImVec2(xPos - arrowInset + i, yPos + i),
                ImVec2(xPos + thickness + arrowInset - i, yPos + i),
                whiteColor);
        }
        
        // Draw black arrow pointing down (right side)
        list->AddRectFilled(ImVec2(x2Pos, y2Pos), ImVec2(x2Pos + thickness, y2Pos - add), blackColor);
        y2Pos -= add;
        for (int i = 0; i < 6; i++) {
            list->AddLine(ImVec2(x2Pos - arrowInset + i, y2Pos - i),
                ImVec2(x2Pos + thickness + arrowInset - i, y2Pos - i),
                blackColor);
        }
    }

    void drawSetup(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        
        // Left bottom: Square (7x7)
        float squareSize = 7.0F;
        float squareX = topLeft.x + 4.0F;
        float squareY = topLeft.y + size.y - 11.0F;
        list->AddRectFilled(ImVec2(squareX, squareY), ImVec2(squareX + squareSize, squareY + squareSize), fgColor);
        
        // Right bottom: Circle (Radius 4)
        float circleCenterX = topLeft.x + size.x - 7.0F;
        float circleCenterY = topLeft.y + size.y - 8.0F;
        float circleRadius = 4.0F;
        list->AddCircleFilled(ImVec2(circleCenterX, circleCenterY), circleRadius, fgColor);
        
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
            list->AddLine(ImVec2(leftX, y), ImVec2(rightX, y), fgColor, 1.0F);
        }
    }

    void drawClear(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        constexpr int LINE_THICKNESS = 2; 
        constexpr float BORDER = 6.0F;

        auto color = getFgColor(state);

        int lineSize = static_cast<int>(std::min(size.x, size.y)) + (LINE_THICKNESS % 2 == 0 ? 1 : 0) - 2 * BORDER;

        float startX1 = topLeft.x + static_cast<float>((static_cast<int>(size.x) - lineSize) / 2);
        float startY1 = topLeft.y + static_cast<float>((static_cast<int>(size.y) - lineSize) / 2);
        float endX1 = startX1 + static_cast<float>(lineSize);
        float endY1 = startY1 + static_cast<float>(lineSize);


        list->AddLine(ImVec2(startX1, startY1), ImVec2(endX1 + 1.0F, endY1 + 1.0F), color, LINE_THICKNESS);
        list->AddLine(ImVec2(startX1, endY1), ImVec2(endX1 + 1.0F, startY1 - 1.0F), color, LINE_THICKNESS);
    }

    void drawSave(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        
        constexpr float borderThickness = 2.0F;
        constexpr float rounding = 1.0F;
        ImVec2 outerRectTopLeft = ImVec2(topLeft.x + BORDER, topLeft.y + BORDER);
        ImVec2 outerRectBottomRight = ImVec2(topLeft.x + size.x - BORDER, topLeft.y + size.y - BORDER);
        
        // Draw outer rounded rectangle
        list->AddRect(outerRectTopLeft, outerRectBottomRight, fgColor, rounding, ImDrawFlags_None, borderThickness);
        
        // In the rectangle: A smaller filled rectangle
        constexpr float innerRectWidth = 6.0F;
        constexpr float innerRectHeight = 3.0F;
        ImVec2 innerRectTopLeft = ImVec2(outerRectTopLeft.x + 4.0F, outerRectTopLeft.y + 4.0F);
        ImVec2 innerRectBottomRight = ImVec2(innerRectTopLeft.x + innerRectWidth, innerRectTopLeft.y + innerRectHeight);
        list->AddRectFilled(innerRectTopLeft, innerRectBottomRight, fgColor);
        
        // In the rectangle: A small filled circle at the bottom center
        constexpr float circleRadius = 2.5f;
        ImVec2 circleCenter = ImVec2(outerRectTopLeft.x + (outerRectBottomRight.x - outerRectTopLeft.x) / 2.0F,
                                     outerRectBottomRight.y - 5.0F);
        list->AddCircleFilled(circleCenter, circleRadius, fgColor);
    }

    void drawOpen(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        
        // Document rectangle with 2px thick border
        constexpr float borderThickness = 2.0F;
        constexpr float rectReduce = 2.0F;
        constexpr float fold = 7.0F;

        ImVec2 docTopLeft = ImVec2(topLeft.x + BORDER + rectReduce, topLeft.y + BORDER);
        ImVec2 docBottomRight = ImVec2(topLeft.x + size.x - BORDER - rectReduce, topLeft.y + size.y - BORDER);

        // Top side
        list->AddLine(ImVec2(docTopLeft.x, docTopLeft.y), ImVec2(docBottomRight.x  - fold + 1, docTopLeft.y), fgColor, borderThickness);
        // Left side
        list->AddLine(ImVec2(docTopLeft.x, docTopLeft.y), ImVec2(docTopLeft.x, docBottomRight.y), fgColor, borderThickness);
        // Bottom side  
        list->AddLine(ImVec2(docTopLeft.x, docBottomRight.y), ImVec2(docBottomRight.x, docBottomRight.y), fgColor, borderThickness);
        // Right side (stop before the fold area)
        list->AddLine(ImVec2(docBottomRight.x, docBottomRight.y), ImVec2(docBottomRight.x, docTopLeft.y + fold), fgColor, borderThickness);
        
        // Triangle sides
        for (int i = 0; i <= static_cast<int>(fold); i++) {
            list->AddLine(
                ImVec2(docBottomRight.x - fold + 1, docTopLeft.y + i - 1), 
                ImVec2(docBottomRight.x - fold + i + 1, docTopLeft.y + i - 1), fgColor, 1.0F);
        }
    }

    void drawFilter(ImDrawList* list, ImVec2 topLeft, ImVec2 size, ButtonState state) {
        auto fgColor = getFgColor(state);
        constexpr float borderThickness = 2.0F;
        constexpr float triangleHeight = 10.0F;
        constexpr float triangleBaseWidth = 21.0F;
        constexpr float spoutThickness = 4.0F;
        
        // Draw funnel shape
        ImVec2 triangleTopLeft = ImVec2(topLeft.x + BORDER, topLeft.y + BORDER + 1.0F);
        ImVec2 triangleTopRight = ImVec2(topLeft.x + triangleBaseWidth, triangleTopLeft.y);
        ImVec2 triangleMiddle = ImVec2(
            (triangleTopLeft.x + triangleTopRight.x) / 2.0F, 
            triangleTopLeft.y + triangleHeight);

        list->AddLine(triangleTopLeft, triangleTopRight, fgColor, borderThickness);
        list->AddLine(triangleTopLeft, triangleMiddle, fgColor, borderThickness);
        list->AddLine(triangleTopRight, triangleMiddle, fgColor, borderThickness);
        float rectLeft = std::trunc(triangleMiddle.x - spoutThickness / 2) + 1.0F;
        list->AddRectFilled(
            ImVec2(rectLeft, triangleMiddle.y), 
            ImVec2(rectLeft + spoutThickness, topLeft.y + size.y - BORDER), 
            fgColor);

    }

    bool drawIconButton(const std::string& id, const std::string& label, ImVec2 size, ButtonState state,
        IconDrawCallback iconDraw) {
        ImVec2 topLeft = ImGui::GetCursorScreenPos();
        bool clicked = ImGui::InvisibleButton(id.c_str(), size);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImU32 bgColor = getBgColor(state);

        drawList->AddRectFilled(topLeft, ImVec2(topLeft.x + size.x, topLeft.y + size.y), bgColor, 3.0F);

        if (iconDraw) {
            iconDraw(drawList, topLeft, size);
        }

        if (state == ButtonState::Highlighted) {
            // draw red dot on top right position
            constexpr float dotRadius = 6.0F;
            constexpr float dotLocation = 2.0F;
            ImVec2 dotPos = ImVec2(topLeft.x + size.x - dotLocation, topLeft.y + dotLocation);
            drawList->AddCircleFilled(dotPos, dotRadius, IM_COL32(192, 0, 0, 192));
        }

        ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
        ImVec2 labelPos = ImVec2(topLeft.x + size.x * 0.5f - labelSize.x * 0.5f, topLeft.y + size.y + 4.0F);
        drawList->AddText(labelPos, getTextColor(state), label.c_str());

        return clicked;
    }

    /**
     * @brief Calculates the total area needed to draw the icon button including its label.
     *
     * @param size   Size of the clickable icon area (excluding label).
     * @param label  The label text shown below the button.
     * @return Total size including label.
     */
    ImVec2 calcIconButtonTotalSize(ImVec2 size, const char* label) {
        ImVec2 labelSize = ImGui::CalcTextSize(label);
        float totalHeight = size.y + 4.0F + labelSize.y;
        float totalWidth = std::max(size.x, labelSize.x);
        return ImVec2(totalWidth, totalHeight);
    }

} // namespace QaplaButton
