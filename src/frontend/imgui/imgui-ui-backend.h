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

#pragma once

/**
 * @file imgui-ui-backend.h
 * @brief What the Dear ImGui frontend shares between its own files: the conversions between
 * QaplaUi's types and ImGui's, and the fonts.
 *
 * Nothing outside src/frontend/imgui includes this.
 */

#include "ui/qapla-ui.h"

#include <imgui.h>

namespace QaplaImGui {

    inline ImVec2 toIm(QaplaUi::Vec2 value) { return { value.x, value.y }; }
    inline ImVec4 toIm(const QaplaUi::Vec4& value) { return { value.x, value.y, value.z, value.w }; }
    inline QaplaUi::Vec2 fromIm(ImVec2 value) { return { value.x, value.y }; }
    inline QaplaUi::Vec4 fromIm(const ImVec4& value) { return { value.x, value.y, value.z, value.w }; }

    ImGuiCol toIm(QaplaUi::StyleColor color);
    ImGuiWindowFlags toIm(QaplaUi::WindowFlags flags);
    ImGuiChildFlags toIm(QaplaUi::ChildFlags flags);
    ImGuiTableFlags toIm(QaplaUi::TableFlags flags);
    ImGuiTableColumnFlags toIm(QaplaUi::TableColumnFlags flags);

    /** @brief Adds the application's fonts to ImGui's atlas; once, before the first frame. */
    void loadFonts();

    /** @brief ImGui's font for one of the application's fonts; never null after loadFonts(). */
    ImFont* font(QaplaUi::Font font);

    /** @brief A QaplaUi::DrawList drawing into an ImGui draw list. */
    class DrawListAdapter : public QaplaUi::DrawList {
    public:
        explicit DrawListAdapter(ImDrawList* list = nullptr) : list_(list) {}
        void setList(ImDrawList* list) { list_ = list; }

        void addLine(QaplaUi::Vec2 from, QaplaUi::Vec2 to, QaplaUi::Color32 color,
            float thickness) override;
        void addRect(QaplaUi::Vec2 min, QaplaUi::Vec2 max, QaplaUi::Color32 color, float rounding,
            float thickness) override;
        void addRectFilled(QaplaUi::Vec2 min, QaplaUi::Vec2 max, QaplaUi::Color32 color,
            float rounding) override;
        void addCircle(QaplaUi::Vec2 center, float radius, QaplaUi::Color32 color, int segments,
            float thickness) override;
        void addCircleFilled(QaplaUi::Vec2 center, float radius, QaplaUi::Color32 color,
            int segments) override;
        void addTriangleFilled(QaplaUi::Vec2 first, QaplaUi::Vec2 second, QaplaUi::Vec2 third,
            QaplaUi::Color32 color) override;
        void addText(QaplaUi::Vec2 position, QaplaUi::Color32 color, std::string_view text) override;
        void addText(QaplaUi::Font font, float size, QaplaUi::Vec2 position, QaplaUi::Color32 color,
            std::string_view text) override;
        void pathArcTo(QaplaUi::Vec2 center, float radius, float fromAngle, float toAngle,
            int segments) override;
        void pathStroke(QaplaUi::Color32 color, bool closed, float thickness) override;
        void pushClipRect(QaplaUi::Vec2 min, QaplaUi::Vec2 max, bool intersectWithCurrent) override;
        void popClipRect() override;

    private:
        ImDrawList* list_;
    };

} // namespace QaplaImGui
