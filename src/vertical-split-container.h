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

        float computeTopHeight(ImVec2 avail) {

            if (fixedTopHeight_) {
                return *fixedTopHeight_;
            }

            float availableHeight = std::max(avail.y - splitterHeight_, minTopHeight_ + minBottomHeight_);

            if (bottomPresetHeight_ != 0) {
                if (bottomHeight_ == 0) {
                    topHeight_ = std::max(topHeight_, availableHeight - bottomPresetHeight_);
                }
                else {
                    auto availDelta = avail.y - availY_;
                    topHeight_ += availDelta;
                }
            }
            availY_ = avail.y;
            auto adjustedTopHeight = std::max(topHeight_, minTopHeight_);
            adjustedTopHeight = std::min(adjustedTopHeight, availableHeight - minBottomHeight_);
            return adjustedTopHeight;
        }

        void draw() override {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float width = avail.x;

            
            float adjustedHeight = computeTopHeight(avail);
            bottomHeight_ = std::max(avail.y - adjustedHeight - splitterHeight_, minBottomHeight_);

            std::string idPrefix = "##vsplit_" + std::to_string(reinterpret_cast<uintptr_t>(this));

            // Top window
            ImGui::BeginChild((idPrefix + "_top").c_str(), ImVec2(width, adjustedHeight),
                ImGuiChildFlags_None,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            try {
                if (topWindow) topWindow->draw();
            }
            catch (const std::exception& e) {
                ImGui::Text("Error in top window: %s", e.what());
			}
            catch (...) {
                ImGui::Text("Unknown error in top window.");
			}
            ImGui::EndChild();

			drawSplitter(idPrefix + "_splitter", ImVec2(width, splitterHeight_));

            // Bottom window
            ImGui::BeginChild((idPrefix + "_bottom").c_str(), ImVec2(width, bottomHeight_), 
                ImGuiChildFlags_None,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            try {
                if (bottomWindow) bottomWindow->draw();
			}
            catch (const std::exception& e) {
                ImGui::Text("Error in bottom window: %s", e.what());
            }
            catch (...) {
                ImGui::Text("Unknown error in bottom window.");
			}
            ImGui::EndChild();
        }

        /**
         * @brief Sets a fixed height for the top window. If set, the splitter cannot be moved.
         */
        void setFixedTopHeight(float height) {
            fixedTopHeight_ = height;
        }

        /**
         * @brief Sets the minimum height for the top window.
         */
        void setMinTopHeight(float height) {
            minTopHeight_ = height;
        }

        /**
         * @brief Sets the minimum height for the bottom window.
         */
        void setMinBottomHeight(float height) {
            minBottomHeight_ = height;
        }

        void setTopPresetHeight(float height) {
            topPresetHeight_ = height;
            bottomPresetHeight_ = 0;
        }

        void setBottomPresetHeight(float height) {
            bottomPresetHeight_ = height;
            topPresetHeight_ = 0;
        }

    private:
        void drawSplitter(const std::string& id, const ImVec2& size) {

            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 100, 100, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(150, 150, 150, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 180, 180, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
            ImGui::Button(id.c_str(), size);

            if (!fixedTopHeight_) {
                if (ImGui::IsItemActive()) {
                    topHeight_ += ImGui::GetIO().MouseDelta.y;
                }
                if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
		}
        std::unique_ptr<EmbeddedWindow> topWindow;
        std::unique_ptr<EmbeddedWindow> bottomWindow;
        std::optional<float> fixedTopHeight_;

        const float splitterHeight_ = 5.0f;

        float minTopHeight_ = 100.0f;
        float minBottomHeight_ = 100.0f;

        float topHeight_ = 500.0f;
        float bottomHeight_ = 0.0f;
        float topPresetHeight_ = 0.0f;
        float bottomPresetHeight_ = 0.0f;
        float availY_ = 0.0f;
    };

} // namespace QaplaWindows
