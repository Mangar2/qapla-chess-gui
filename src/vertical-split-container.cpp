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
        QaplaUi::WindowFlags top,
        QaplaUi::WindowFlags bottom) {
        topFlags_ = top;
        bottomFlags_ = bottom;
        name_ = name;
        topHeight_ = 500.0F;
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

    float VerticalSplitContainer::computeTopHeight(QaplaUi::Vec2 avail) {
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
        QaplaUi::Vec2 avail = QaplaUi::getContentRegionAvail();
        float width = avail.x;

        topHeight_ = computeTopHeight(avail);
        bottomHeight_ = std::max(avail.y - topHeight_ - splitterHeight_, minBottomHeight_);

        // Top window
        if (QaplaUi::beginChild(("vsplit." + name_ + ".top").c_str(), QaplaUi::Vec2(width, topHeight_),
            QaplaUi::ChildFlags::None, topFlags_)) {
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
        QaplaUi::endChild();

        drawSplitter("vsplit." + name_ + ".splitter", QaplaUi::Vec2(width, splitterHeight_));

        // Bottom window
        if (QaplaUi::beginChild(("vsplit." + name_ + ".bottom").c_str(), QaplaUi::Vec2(width, bottomHeight_), 
            QaplaUi::ChildFlags::None, bottomFlags_)) {
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
        QaplaUi::endChild();
    }

    void VerticalSplitContainer::drawSplitter(const std::string& id, const QaplaUi::Vec2& size) {
        // If either panel is fixed, disable splitter interaction
        bool isFixed = topFixed_ || bottomFixed_;
        
        QaplaUi::pushStyleVar(QaplaUi::StyleVar::FramePadding, QaplaUi::Vec2(0, 0));
        QaplaUi::pushStyleVar(QaplaUi::StyleVar::ItemSpacing, QaplaUi::Vec2(0, 0));

        QaplaUi::setCursorPosY(QaplaUi::getCursorPosY() - 4.0F);
        topHeight_ += QaplaUi::splitter(id, size, true, !isFixed);

        QaplaUi::popStyleVar(2);
    }

} // namespace QaplaWindows
