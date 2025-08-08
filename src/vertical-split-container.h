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

#include "embedded-window.h"
#include <memory>
#include <algorithm>
#include <string>

#include "imgui.h"

namespace QaplaWindows {

    /**
     * @brief Displays two embedded windows vertically with a draggable splitter.
     */
    class VerticalSplitContainer : public EmbeddedWindow {
    public:
        void setTop(std::unique_ptr<EmbeddedWindow> window) {
            topWindow = std::move(window);
        }

        void setBottom(std::unique_ptr<EmbeddedWindow> window) {
            bottomWindow = std::move(window);
        }

        void draw() override {
            ImVec2 region = ImGui::GetContentRegionAvail();
            float width = region.x;

            float splitterHeight = 5.0f;
            float minSize = 100.0f;
            float availableHeight = std::max(region.y - splitterHeight, 2 * minSize);

            if (fixedTopHeight) {
                topHeight = std::clamp(*fixedTopHeight, minSize, availableHeight - minSize);
            }
            else {
                topHeight = std::clamp(topHeight, minSize, availableHeight - minSize);
            }
            float bottomHeight = availableHeight - topHeight;

            std::string idPrefix = "##vsplit_" + std::to_string(reinterpret_cast<uintptr_t>(this));

            // Top window
            ImGui::BeginChild((idPrefix + "_top").c_str(), ImVec2(width, topHeight), 
                ImGuiChildFlags_None,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            if (topWindow) topWindow->draw();
            ImGui::EndChild();

            // Splitter
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 100, 100, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(150, 150, 150, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 180, 180, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
            ImGui::Button((idPrefix + "_splitter").c_str(), ImVec2(width, splitterHeight));

            if (!fixedTopHeight) {
                if (ImGui::IsItemActive()) {
                    topHeight += ImGui::GetIO().MouseDelta.y;
                }
                if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);

            // Bottom window
            ImGui::BeginChild((idPrefix + "_bottom").c_str(), ImVec2(width, bottomHeight), 
                ImGuiChildFlags_None,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            if (bottomWindow) bottomWindow->draw();
            ImGui::EndChild();
        }

        void setFixedTopHeight(float height) {
            fixedTopHeight = height;
        }

    private:
        std::unique_ptr<EmbeddedWindow> topWindow;
        std::unique_ptr<EmbeddedWindow> bottomWindow;
        std::optional<float> fixedTopHeight;

        float topHeight = 500.0f;
    };

} // namespace QaplaWindows
