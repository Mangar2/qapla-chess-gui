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
 * @author Volker B�hm
 * @copyright Copyright (c) 2025 Volker B�hm
 */

#pragma once

#include "embedded-window.h"
#include "snackbar.h"
#include <memory>
#include <algorithm>
#include <imgui.h>

namespace QaplaWindows {

    /**
     * @brief Displays two embedded windows side by side with a draggable splitter.
     */
    class HorizontalSplitContainer : public EmbeddedWindow {
    public:
        void setLeft(std::unique_ptr<EmbeddedWindow> window) {
            leftWindow_ = std::move(window);
        }

        void setRight(std::unique_ptr<EmbeddedWindow> window) {
            rightWindow_ = std::move(window);
        }

        void draw() override {
            
            ImVec2 region = ImGui::GetContentRegionAvail();
            float height = region.y;

            float splitterWidth = 5.0f;
            float minSize = 100.0f;
            float availableWidth = std::max(region.x - splitterWidth - 13, 2 * minSize);
            float adjustedLeftWidth;

            adjustedLeftWidth = std::clamp(leftWidth_, minSize, availableWidth - minSize);
            std::string idPrefix = "hsplit_" + std::to_string(reinterpret_cast<uintptr_t>(this));

            ImGui::BeginChild((idPrefix + "_left").c_str(), ImVec2(adjustedLeftWidth, height),
                ImGuiChildFlags_None,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            try {
                if (leftWindow_) leftWindow_->draw();
			}
            catch (const std::exception& e) {
                SnackbarManager::instance().showError(
					std::format("Error in left window: {}", e.what()));
            }
            catch (...) {
				SnackbarManager::instance().showError("Unknown error in left window");
			}
            ImGui::EndChild();
            
            ImGui::SameLine(0, 0);
            drawSplitter(idPrefix + "_splitter", ImVec2(splitterWidth, height));
            ImGui::SameLine(0, 0);

            float rightWidth = region.x - ImGui::GetCursorPosX();

            ImGui::BeginChild((idPrefix + "_right").c_str(), ImVec2(rightWidth, height),
                ImGuiChildFlags_None,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            try {
                if (rightWindow_) rightWindow_->draw();
            }
            catch (const std::exception& e) {
				SnackbarManager::instance().showError(
					std::format("Error in right window: {}", e.what()));
			}
            catch (...) {
				SnackbarManager::instance().showError("Unknown error in right window");
			}
            ImGui::EndChild();

        }

    private:
        void drawSplitter(const std::string& id, const ImVec2& vec) {
            // Splitter
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 100, 100, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(150, 150, 150, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 180, 180, 255));

            ImGui::Button(id.c_str(), vec);
            if (ImGui::IsItemActive()) {
                leftWidth_ += ImGui::GetIO().MouseDelta.x;
            }

            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); // oder ResizeNS
            }

            ImGui::PopStyleColor(3);
		}
        std::unique_ptr<EmbeddedWindow> leftWindow_;
        std::unique_ptr<EmbeddedWindow> rightWindow_;
        float leftWidth_ = 400.0f;
    };

} // namespace QaplaWindows
