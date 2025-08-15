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

namespace QaplaButton {

    constexpr float BORDER = 4.0f;

    using IconDrawCallback = std::function<void(ImDrawList*, ImVec2 topLeft, ImVec2 size)>;

    static auto getBgColor(bool active = false) {
        bool hovered = ImGui::IsItemHovered();
		active = active || ImGui::IsItemActive();

		if (active) return ImGui::GetColorU32(ImGuiCol_ButtonActive);
		if (hovered) return ImGui::GetColorU32(ImGuiCol_ButtonHovered);
		return ImGui::GetColorU32(ImGuiCol_Button);
    }

    static auto getFgColor(bool active = false) {
        bool hovered = ImGui::IsItemHovered();
        active = active || ImGui::IsItemActive();

        if (active) return ImGui::GetColorU32(ImGuiCol_Text);
        if (hovered) return ImGui::GetColorU32(ImGuiCol_Text);
        return ImGui::GetColorU32(ImGuiCol_TextDisabled);

    }

    void drawNew(ImDrawList* list, ImVec2 topLeft, ImVec2 size) {
        constexpr float FIELD_SIZE = 3;
        auto xPos = topLeft.x + BORDER;
        auto yPos = topLeft.y + BORDER;
        auto color = getFgColor();
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

    void drawNow(ImDrawList* list, ImVec2 topLeft, ImVec2 size) {
        auto yReduce = BORDER + 5.0f;
        auto thickness = 7.0f;
        auto arrowInset = 4.0f;
        auto xPos = topLeft.x + BORDER - 1.0f;
        auto yPos = topLeft.y + yReduce;
        auto color = getFgColor();
        list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + 3.0f, yPos + thickness), color);
        xPos += 3.0f;
        for (int i = 0; i < 8; i++) {
            list->AddLine(ImVec2(xPos + i, yPos - arrowInset + i),
                ImVec2(xPos + i, yPos + thickness + arrowInset - i),
                color);
        }
		xPos += 9.0f;
        for (int i = 0; i <= 3; i++) {
            list->AddLine(ImVec2(xPos + i, yPos + 3.0f - i),
                ImVec2(xPos + i, yPos + 5.0f + i), color);
            list->AddLine(ImVec2(xPos + 7.0f - i, yPos + 3.0f - i),
                ImVec2(xPos + 7.0f - i, yPos + 5.0f + i), color);
        }
    }

    void drawStop(ImDrawList* list, ImVec2 topLeft, ImVec2 size) {
        for (int i = 1; i <= 2; i++) {
            auto reduce = BORDER + static_cast<float>(i);
            auto color = getFgColor();
            list->AddRect(
                ImVec2(topLeft.x + reduce, topLeft.y + reduce),
                ImVec2(topLeft.x + size.x - reduce, topLeft.y + size.y - reduce),
                color,
                0.0f,
                ImDrawFlags_None,
                1.0f);
        }
    }

    void drawAdd(ImDrawList* list, ImVec2 topLeft, ImVec2 size) {
        auto color = getFgColor();
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
            ImVec2(horizontalStartX, horizontalStartY),
            ImVec2(horizontalStartX + horizontalLength, horizontalStartY + thickness),
            color
        );

        list->AddRectFilled(
            ImVec2(verticalStartX, verticalStartY),
            ImVec2(verticalStartX + thickness, verticalStartY + verticalLength),
            color
        );
    }

    void drawRemove(ImDrawList* list, ImVec2 topLeft, ImVec2 size) {
        auto color = getFgColor();
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
            ImVec2(horizontalStartX, horizontalStartY),
            ImVec2(horizontalStartX + horizontalLength, horizontalStartY + thickness),
            color
        );

    }


    void drawRestart (ImDrawList* list, ImVec2 topLeft, ImVec2 size) {
        ImVec2 center = topLeft;
        center.x += size.x / 2;
        center.y += size.y / 2;
        float radius = std::min(size.x, size.y) / 2.0f - BORDER - 1;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        constexpr float pi = 3.14159265358979323846f;

        auto color = getFgColor();

        float startAngle = 1.5f * pi;   // 270° = 3/2 Pi
        float endAngle = startAngle + 1.5f * pi;  // 270° → 45° 
        drawList->PathArcTo(center, radius, startAngle, endAngle, 20);
        drawList->PathStroke(color, false, 1.5f);

        // Pfeilspitze
        ImVec2 arrowTip = ImVec2(center.x + cosf(pi) * radius,
            center.y + sinf(pi) * radius);

        for (int i = 0; i < 3; ++i) {
            float width = std::max(5.0f - 2.0f * static_cast<float>(i), 1.0f);
            float yOffset = -2.0f * i;
            ImVec2 p1 = ImVec2(arrowTip.x - width / 2.0f, arrowTip.y + yOffset);
            ImVec2 p2 = ImVec2(arrowTip.x + width / 2.0f, arrowTip.y + yOffset);
            drawList->AddLine(p1, p2, color, 2.0f);
        }
    }

    void drawConfig(ImDrawList* list, ImVec2 topLeft, ImVec2 size) {
        auto color = getFgColor();
        ImVec2 center = ImVec2(topLeft.x + size.x / 2.0f, topLeft.y + size.y / 2.0f);
        float radius = std::min(size.x, size.y) / 2.0f - BORDER - 1;
        constexpr float pi = 3.14159265358979323846f;
        
        float innerRadius = radius - 0.5f;
        list->AddCircle(center, innerRadius, color, 0, 2.0f);
        // Draw the gear shape
        for (int i = 0; i < 8; ++i) {
            float angle = i * (pi / 4.0f); // 45 degrees
            float x = center.x + cos(angle) * radius;
            float y = center.y + sin(angle) * radius;
            list->AddCircleFilled(ImVec2(x, y), 2.0f, color);
        }
	}

    void drawText(const std::string& text, ImDrawList* list, ImVec2 topLeft, ImVec2 size) {
        auto color = getFgColor();
        ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
        ImVec2 textPos = ImVec2(
            topLeft.x + std::round((size.x - textSize.x) * 0.5f) - 2.0f,
            topLeft.y - 2.0f
        );
        float fontSize = size.y * 1.0f;
        list->AddText(ImGui::GetFont(), fontSize, textPos, color, text.c_str());
	}

    void drawPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool active) {
		auto yReduce = BORDER + 5.0f;
        auto thickness = 7.0f;
        auto arrowInset = 4.0f;
        auto xPos = topLeft.x + BORDER - 1.0f;
        auto yPos = topLeft.y + yReduce;
        auto color = getFgColor(active);
        for (auto add : { 3.0f, 4.0f, 5.0f }) {
            list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + add - (add == 5.0f ? 0.0f : 1.0f), yPos + thickness), color);
            xPos += add;
        }
        for (int i = 0; i < 8; i++) {
            list->AddLine(ImVec2(xPos + i, yPos - arrowInset + i), 
                ImVec2(xPos + i, yPos + thickness + arrowInset - i),
				color);
        }
    }

    void drawAnalyze(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool active) {
        auto yReduce = BORDER + 4.0f;
        auto thickness = 5.0f;
        auto arrowInset = 3.0f;
        auto xPos = topLeft.x + BORDER;
        auto yPos = topLeft.y + yReduce;
        auto color = getFgColor(active);
        for (auto add : { 3.0f, 4.0f, 5.0f }) {
            list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + add - (add == 5.0f ? 0.0f : 1.0f), yPos + thickness), color);
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

    void drawAutoPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool active) {
        auto yReduce = BORDER + 2.0f;
        auto thickness = 5.0f;
        auto arrowInset = 3.0f;
        auto xPos = topLeft.x + BORDER;
		auto x2Pos = topLeft.x + size.x - BORDER;
        auto yPos = topLeft.y + yReduce;
		auto y2Pos = topLeft.y + yReduce + thickness + 3.0f;
        auto color = getFgColor(active);
        for (auto add : { 3.0f, 4.0f, 5.0f }) {
            list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + add - (add == 5.0f ? 0.0f : 1.0f) , yPos + thickness), color);
            list->AddRectFilled(ImVec2(x2Pos, y2Pos), ImVec2(x2Pos - add + (add == 5.0f ? 0.0f : 1.0f), y2Pos + thickness), color);
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

    void drawManualPlay(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool active) {
        auto yReduce = BORDER + 2.0f;
        auto thickness = 5.0f;
        auto xPos = topLeft.x + BORDER + 3.0f;
        auto x2Pos = topLeft.x + size.x - BORDER - 2.0f;
        auto yPos = topLeft.y + yReduce;
        auto y2Pos = topLeft.y + yReduce + thickness + 3.0f;
        auto color = getFgColor(active);
        for (auto add : { 3.0f, 4.0f, 5.0f }) {
            list->AddRectFilled(ImVec2(xPos, yPos), ImVec2(xPos + add - (add == 5.0f ? 0.0f : 1.0f), yPos + thickness), color);
            list->AddRectFilled(ImVec2(x2Pos, y2Pos), ImVec2(x2Pos - add + (add == 5.0f ? 0.0f : 1.0f), y2Pos + thickness), color);
            xPos += add;
            x2Pos -= add;
        }
    }

    void drawAutoDetect(ImDrawList* list, ImVec2 topLeft, ImVec2 size, bool rotate) {
        auto color = QaplaButton::getFgColor();
        ImVec2 center = ImVec2(topLeft.x + size.x / 2, topLeft.y + size.y / 2);
        float radius = std::min(size.x, size.y) / 2 - BORDER; // Leave some padding

        // Draw outer circle
        list->AddCircle(center, radius, color, 32, 1.5f);

        // Draw inner circle
        list->AddCircle(center, radius * 0.6f, color, 32, 1.5f);

        // Draw scanning line
        if (rotate) {
            float angle = ImGui::GetTime() * 2.0f;
            ImVec2 endPoint = ImVec2(
                center.x + cos(angle) * (radius + 2),
                center.y + sin(angle) * (radius + 2)
            );
            list->AddLine(center, endPoint, color, 2.0f);
        }
    }

    /**
     * @brief Draws a clickable icon-style button with optional icon and label below.
     *
     * @param id        Unique ImGui ID.
     * @param label     Text label shown below the button.
     * @param size      Size of the clickable icon area (excluding label).
     * @param iconDraw  Optional callback to draw the icon content (may be nullptr).
     * @return true if the button was clicked.
     */
    bool drawIconButton(const std::string& id, const std::string& label, ImVec2 size, bool active,
        IconDrawCallback iconDraw) {
        ImVec2 topLeft = ImGui::GetCursorScreenPos();
        bool clicked = ImGui::InvisibleButton(id.c_str(), size);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImU32 bgColor = getBgColor(active);

        drawList->AddRectFilled(topLeft, ImVec2(topLeft.x + size.x, topLeft.y + size.y), bgColor, 3.0f);

        if (iconDraw) {
            iconDraw(drawList, topLeft, size);
        }

        ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
        ImVec2 labelPos = ImVec2(topLeft.x + size.x * 0.5f - labelSize.x * 0.5f, topLeft.y + size.y + 4.0f);
        drawList->AddText(labelPos, IM_COL32(192, 192, 192, 255), label.c_str());

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
        float totalHeight = size.y + 4.0f + labelSize.y;
        float totalWidth = std::max(size.x, labelSize.x);
        return ImVec2(totalWidth, totalHeight);
    }

} // namespace QaplaButton
