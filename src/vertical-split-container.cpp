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

#include "vertical-split-container.h"
#include <format>

namespace QaplaWindows {

    VerticalSplitContainer::VerticalSplitContainer(
        const std::string& name,
        ImGuiWindowFlags top,
        ImGuiWindowFlags bottom) {
        topFlags_ = top;
        bottomFlags_ = bottom;
        name_ = name;
        topHeight_ = 500.0f;
    }

    void VerticalSplitContainer::setTop(std::unique_ptr<EmbeddedWindow> window) {
        topWindow_ = std::move(window);
        // Create a callback that calls the embedded window's draw method
        if (topWindow_) {
            topCallback_ = [this]() { topWindow_->draw(); };
        } else {
            topCallback_ = nullptr;
        }
    }

    void VerticalSplitContainer::setBottom(std::unique_ptr<EmbeddedWindow> window) {
        bottomWindow_ = std::move(window);
        // Create a callback that calls the embedded window's draw method
        if (bottomWindow_) {
            bottomCallback_ = [this]() { bottomWindow_->draw(); };
        } else {
            bottomCallback_ = nullptr;
        }
    }

    void VerticalSplitContainer::setTop(std::function<void()> callback) {
        topCallback_ = std::move(callback);
        // Clear the embedded window since we're using a callback
        topWindow_.reset();
    }

    void VerticalSplitContainer::setBottom(std::function<void()> callback) {
        bottomCallback_ = std::move(callback);
        // Clear the embedded window since we're using a callback
        bottomWindow_.reset();
    }

    void VerticalSplitContainer::setPresetHeight(float height, bool isTop) {
        if (isTop) {
            topPresetHeight_ = height;
            bottomPresetHeight_ = 0;
        } else {
            bottomPresetHeight_ = height;
            topPresetHeight_ = 0;
        }
    }

    void VerticalSplitContainer::setFixedHeight(float height, bool isTop) {
        if (isTop) {
            topFixed_ = true;
            bottomFixed_ = false;
            topHeight_ = height;
        } else {
            bottomFixed_ = true;
            topFixed_ = false;
            setPresetHeight(height, false);
        }
    }

    void VerticalSplitContainer::setMinTopHeight(float height) {
        minTopHeight_ = height;
    }

    void VerticalSplitContainer::setMinBottomHeight(float height) {
        minBottomHeight_ = height;
    }

    float VerticalSplitContainer::computeTopHeight(ImVec2 avail) {
        // If top panel is fixed, return the fixed height
        if (topFixed_) {
            return std::max(std::min(topHeight_, avail.y - minBottomHeight_ - splitterHeight_), minTopHeight_);
        }

        float availableHeight = std::max(avail.y - splitterHeight_, minTopHeight_ + minBottomHeight_);
        float height = topHeight_;

        // If bottom panel is fixed, compute top height based on remaining space
        if (bottomFixed_) {
            height = availableHeight - bottomPresetHeight_;
        } else if (bottomPresetHeight_ != 0) {
            if (bottomHeight_ == 0) {
                height = availableHeight - bottomPresetHeight_;
            } else {
                auto availDelta = avail.y - availY_;
                height += availDelta;
            }
        }

        availY_ = avail.y;
        height = std::max(height, minTopHeight_);
        height = std::min(height, availableHeight - minBottomHeight_);
        return height;
    }

    void VerticalSplitContainer::draw() {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float width = avail.x;

        topHeight_ = computeTopHeight(avail);
        bottomHeight_ = std::max(avail.y - topHeight_ - splitterHeight_, minBottomHeight_);

        // Top window
        if (ImGui::BeginChild(("vsplit." + name_ + ".top").c_str(), ImVec2(width, topHeight_),
            ImGuiChildFlags_None, topFlags_)) {
            try {
                if (topCallback_) topCallback_();
            }
            catch (const std::exception& e) {
                SnackbarManager::instance().showError(
                    std::format("Error in top window: {}", e.what()));
            }
            catch (...) {
                SnackbarManager::instance().showError("Unknown error in top window");
            }
        }
        // Always call EndChild() regardless of BeginChild() return value - see imgui.h line 444:
        // "Always call a matching EndChild() for each BeginChild() call, regardless of its return value"
        ImGui::EndChild();

        drawSplitter("vsplit." + name_ + ".splitter", ImVec2(width, splitterHeight_));

        // Bottom window
        if (ImGui::BeginChild(("vsplit." + name_ + ".bottom").c_str(), ImVec2(width, bottomHeight_), 
            ImGuiChildFlags_None, bottomFlags_)) {
            try {
                if (bottomCallback_) bottomCallback_();
            }
            catch (const std::exception& e) {
                SnackbarManager::instance().showError(
                    std::format("Error in bottom window: {}", e.what()));
            }
            catch (...) {
                SnackbarManager::instance().showError("Unknown error in bottom window");
            }
        }
        // Always call EndChild() regardless of BeginChild() return value - see imgui.h line 444:
        // "Always call a matching EndChild() for each BeginChild() call, regardless of its return value"
        ImGui::EndChild();
    }

    void VerticalSplitContainer::drawSplitter(const std::string& id, const ImVec2& size) {
        // If either panel is fixed, disable splitter interaction
        bool isFixed = topFixed_ || bottomFixed_;
        
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        if (isFixed) {
            // Draw a non-interactive splitter with different styling
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80, 80, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 80, 80, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(80, 80, 80, 255));
            
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
            ImGui::Button(("###" + id).c_str(), size);
            
            ImGui::PopStyleColor(3);
        } else {
            // Draw interactive splitter
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 100, 100, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(150, 150, 150, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 180, 180, 255));

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4.0f);
            ImGui::Button(("###" + id).c_str(), size);

            if (ImGui::IsItemActive()) {
                topHeight_ += ImGui::GetIO().MouseDelta.y;
            }

            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }

            ImGui::PopStyleColor(3);
        }

        ImGui::PopStyleVar(2);
    }

} // namespace QaplaWindows
