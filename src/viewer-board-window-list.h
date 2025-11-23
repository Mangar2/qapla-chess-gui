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

#include "viewer-board-window.h"
#include "game-manager-pool-access.h"
#include "imgui-table.h"

#include <vector>
#include <functional>
#include <algorithm>
#include <mutex>

namespace QaplaWindows {

/**
 * @class ViewerBoardWindowList
 * @brief Manages a list of viewer board windows and populates them with game data.
 */
class ViewerBoardWindowList {
public:
    /**
     * @brief Constructor for ViewerBoardWindowList.
     * @param poolAccess Access to the GameManagerPool instance.
     */
    explicit ViewerBoardWindowList(std::string name)
        : name_(std::move(name)) {
        std::scoped_lock lock(instancesMutex_);
        instances_.push_back(this);
    }

    /**
     * @brief Destructor for ViewerBoardWindowList.
     */
    ~ViewerBoardWindowList() {
        std::scoped_lock lock(instancesMutex_);
        std::erase(instances_, this);
    }

    /**
     * @brief Sets the GameManagerPoolAccess instance.
     * @param poolAccess Access to the GameManagerPool instance.
     */
    void setPoolAccess(GameManagerPoolAccess poolAccess) {
        poolAccess_ = std::move(poolAccess);
    }

    /**
     * @brief Populates all viewer windows with current game data.
     */
    void populateViews() {
        clearRunningFlags();

        poolAccess_->withGameRecords(
            [&](const QaplaTester::GameRecord& game, uint32_t gameIndex) {
                ensureWindowExists(gameIndex);
                boardWindows_[gameIndex].setFromGameRecord(game);
                boardWindows_[gameIndex].setRunning(true);
            },
            [&](uint32_t gameIndex) -> bool {
                ensureWindowExists(gameIndex);
                return true;
            }
        );

        poolAccess_->withEngineRecords(
            [&](const QaplaTester::EngineRecords& records, uint32_t gameIndex) {
                if (gameIndex >= boardWindows_.size()) {
                    return;
                }
                boardWindows_[gameIndex].setFromEngineRecords(records);
            },
            [&](uint32_t gameIndex) -> bool {
                if (gameIndex >= boardWindows_.size()) {
                    return false;
                }
                return boardWindows_[gameIndex].isActive();
            }
        );

        poolAccess_->withMoveRecord(
            [&](const QaplaTester::MoveRecord& record, uint32_t gameIndex, uint32_t playerIndex) {
                if (gameIndex >= boardWindows_.size()) {
                    return;
                }
                boardWindows_[gameIndex].setFromMoveRecord(record, playerIndex);
            },
            [&](uint32_t gameIndex) -> bool {
                if (gameIndex >= boardWindows_.size()) {
                    return false;
                }
                return boardWindows_[gameIndex].isActive();
            }
        );
    }

    /**
     * @brief Checks if any window is currently running.
     * @return True if at least one window is running, false otherwise.
     */
    [[nodiscard]] 
    bool isAnyRunning() const {
        return std::ranges::any_of(boardWindows_,
            [](const ViewerBoardWindow& window) { return window.isRunning(); });
    }

    /**
     * @brief Draws the tabs for all ViewerBoardWindowList instances.
     */
    static void drawAllTabs() {
        std::scoped_lock lock(instancesMutex_);
        for (auto* instance : instances_) {
            if (instance != nullptr) {
                instance->drawTabs();
            }
        }
    }

    /**
     * @brief Gets the list of board windows.
     * @return Reference to the vector of board windows.
     */
    std::vector<ViewerBoardWindow>& getWindows() { return boardWindows_; }

    /**
     * @brief Gets the list of board windows (const version).
     * @return Const reference to the vector of board windows.
     */
    [[nodiscard]] 
    const std::vector<ViewerBoardWindow>& getWindows() const { return boardWindows_; }

    /**
     * @brief Activates a tab by its window ID.
     * @param windowId The ID of the window to activate.
     */
    void setActiveWindowId(const std::string& windowId) {
        activeWindowId_ = windowId;
    }

    /**
     * @brief Gets the currently active window ID.
     * @return The active window ID as a string.
     */
    [[nodiscard]]
    const std::string& getActiveWindowId() const {
        return activeWindowId_;
    }

private:
    static inline std::vector<ViewerBoardWindowList*> instances_;
    static inline std::mutex instancesMutex_;

    GameManagerPoolAccess poolAccess_;
    std::vector<ViewerBoardWindow> boardWindows_;
    size_t selectedIndex_ = 0;
    std::string name_;
    std::string activeWindowId_;

    /**
     * @brief Ensures that a window exists at the given index.
     * @param index The index to ensure exists.
     */
    void ensureWindowExists(size_t index) {
        while (index >= boardWindows_.size()) {
            boardWindows_.emplace_back();
        }
    }

    /**
     * @brief Resets the running flag for all windows.
     */
    void clearRunningFlags() {
        for (auto& window : boardWindows_) {
            window.setRunning(false);
        }
    }

    /**
     * @brief Draws the tabs for all viewer windows.
     */
    void drawTabs() {
        size_t newIndex = -1;
        for (size_t index = 0; index < boardWindows_.size(); index++) {
            auto& window = boardWindows_[index];
            std::string tabName = window.id();
            std::string tabId = std::format("###Game_{}_{}", name_, index);
            if (!window.isRunning() && index != selectedIndex_) {
                continue;
            }
            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            // If this window is not active but matches the activeWindowId_ there was a request to activate it
            if (!activeWindowId_.empty() && activeWindowId_ == window.getWindowId()) {
                flags |= ImGuiTabItemFlags_SetSelected;
                activeWindowId_.clear();
            }

            bool open = ImGui::BeginTabItem((tabName + tabId).c_str(), nullptr, flags);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", window.getTooltip().c_str());
            }
            if (open) {
                // We may not show the window directly after activating because it needs one
                // frame to populate data
                if (window.isActive()) {
                    window.draw();
                } else if (selectedIndex_ >= 0 && std::cmp_equal(selectedIndex_, boardWindows_.size())) {
                    // Once more draw the old window to prevent flickering
                    boardWindows_[selectedIndex_].draw();
                }
                window.setActive(true);
                newIndex = index;
                ImGui::EndTabItem();
            } else {
                window.setActive(false);
            }
        }
        selectedIndex_ = newIndex;
    }


};

} // namespace QaplaWindows