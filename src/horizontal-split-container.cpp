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

#include "horizontal-split-container.h"
#include <format>

namespace QaplaWindows {

    HorizontalSplitContainer::HorizontalSplitContainer(
        const std::string& name,
        ImGuiWindowFlags left,
        ImGuiWindowFlags right) {
        leftFlags_ = left;
        rightFlags_ = right;
        name_ = name;
        leftWidth_ = 400.0F;
    }

    void HorizontalSplitContainer::setLeft(std::unique_ptr<EmbeddedWindow> window) {
        leftWindow_ = std::move(window);
        // Create a callback that calls the embedded window's draw method
        if (leftWindow_) {
            leftCallback_ = [this]() { leftWindow_->draw(); };
        } else {
            leftCallback_ = nullptr;
        }
    }

    void HorizontalSplitContainer::setRight(std::unique_ptr<EmbeddedWindow> window) {
        rightWindow_ = std::move(window);
        // Create a callback that calls the embedded window's draw method
        if (rightWindow_) {
            rightCallback_ = [this]() { rightWindow_->draw(); };
        } else {
            rightCallback_ = nullptr;
        }
    }

    void HorizontalSplitContainer::setLeft(std::function<void()> callback) {
        leftCallback_ = std::move(callback);
        // Clear the embedded window since we're using a callback
        leftWindow_.reset();
    }

    void HorizontalSplitContainer::setRight(std::function<void()> callback) {
        rightCallback_ = std::move(callback);
        // Clear the embedded window since we're using a callback
        rightWindow_.reset();
    }

    void HorizontalSplitContainer::setPresetWidth(float width, bool isLeft) {
        if (isLeft) {
            leftPresetWidth_ = width;
            rightPresetWidth_ = 0;
        } else {
            rightPresetWidth_ = width;
            leftPresetWidth_ = 0;
        }
    }

    void HorizontalSplitContainer::setFixedWidth(float width, bool isLeft) {
        if (isLeft) {
            leftFixed_ = true;
            rightFixed_ = false;
            leftWidth_ = width;
        } else {
            rightFixed_ = true;
            leftFixed_ = false;
            setPresetWidth(width, false);
        }
    }

    float HorizontalSplitContainer::computeLeftWidth(ImVec2 avail) {
        constexpr float imguiPadding = 13.0F; 
        float availableWidth = std::max(avail.x - splitterWidth_ - imguiPadding, 2 * minSize_);
        float leftWidth = leftWidth_;

        // If left panel is fixed, return the fixed width
        if (leftFixed_) {
            return std::max(std::min(leftWidth_, availableWidth - minSize_), minSize_);
        }

        // If right panel is fixed, compute left width based on remaining space
        if (rightFixed_) {
            leftWidth = availableWidth - rightPresetWidth_;
        } else if (rightPresetWidth_ != 0) {
            if (rightWidth_ == 0) {
                leftWidth = availableWidth - rightPresetWidth_;
            } else {
                auto availDelta = avail.x - availX_;
                auto rightWidth = availableWidth - leftWidth - availDelta;
                rightWidth = std::max(rightWidth, std::min(rightWidth + 
                    std::max(availDelta, 0.0F), rightPresetWidth_));
                leftWidth = availableWidth - rightWidth;
            }
        } else if (leftPresetWidth_ != 0) {
            if (leftWidth_ == 0) {
                leftWidth = leftPresetWidth_;
            } else {
                auto availDelta = std::max(avail.x - availX_, 0.0F);
                leftWidth = std::max(leftWidth_, std::min(leftWidth + availDelta, leftPresetWidth_));
            }
        } 

        availX_ = avail.x;
        leftWidth = std::max(leftWidth, minSize_);
        leftWidth = std::min(leftWidth, availableWidth - minSize_);
        return leftWidth;
    }

    void HorizontalSplitContainer::draw() {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float height = avail.y;

        leftWidth_ = computeLeftWidth(avail);

        if (ImGui::BeginChild(("hsplit." + name_ + ".left").c_str(), ImVec2(leftWidth_, height),
            ImGuiChildFlags_None, leftFlags_)) {
            try {
                if (leftCallback_) leftCallback_();
            }
            catch (const std::exception& e) {
                SnackbarManager::instance().showError(
                    std::format("Error in left window: {}", e.what()));
            }
            catch (...) {
                SnackbarManager::instance().showError("Unknown error in left window");
            }
        }
        // Always call EndChild() regardless of BeginChild() return value - see imgui.h line 444:
        // "Always call a matching EndChild() for each BeginChild() call, regardless of its return value"
        ImGui::EndChild();
        
        ImGui::SameLine(0, 0);
        drawSplitter("hsplit." + name_ + ".splitter", ImVec2(splitterWidth_, height));
        ImGui::SameLine(0, 0);

        rightWidth_ = std::max(avail.x - ImGui::GetCursorPosX(), minSize_);

        if (ImGui::BeginChild(("hsplit." + name_ + ".right").c_str(), ImVec2(rightWidth_, height),
            ImGuiChildFlags_None, rightFlags_)) {
            try {
                if (rightCallback_) rightCallback_();
            }
            catch (const std::exception& e) {
                SnackbarManager::instance().showError(
                    std::format("Error in right window: {}", e.what()));
            }
            catch (...) {
                SnackbarManager::instance().showError("Unknown error in right window");
            }
        }
        // Always call EndChild() regardless of BeginChild() return value - see imgui.h line 444:
        // "Always call a matching EndChild() for each BeginChild() call, regardless of its return value"
        ImGui::EndChild();
    }

    void HorizontalSplitContainer::drawSplitter(const std::string& id, const ImVec2& vec) {
        // If either panel is fixed, disable splitter interaction
        bool isFixed = leftFixed_ || rightFixed_;
        
        if (isFixed) {
            // Draw a non-interactive splitter with different styling
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80, 80, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 80, 80, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(80, 80, 80, 255));
            
            ImGui::Button(("###" + id).c_str(), vec);
            
            ImGui::PopStyleColor(3);
        } else {
            // Draw interactive splitter
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 100, 100, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(150, 150, 150, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 180, 180, 255));

            ImGui::Button(("###" + id).c_str(), vec);
            if (ImGui::IsItemActive()) {
                leftWidth_ += ImGui::GetIO().MouseDelta.x;
            }

            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); 
            }

            ImGui::PopStyleColor(3);
        }
    }

} // namespace QaplaWindows
