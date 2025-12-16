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

#include "imgui-tab-bar.h"
#include "imgui-controls.h"
#include "i18n.h"

#include <algorithm>


namespace QaplaWindows {

    ImGuiTabBar::ImGuiTabBar() {
        messageCallbackHandle_ = QaplaWindows::StaticCallbacks::message().registerCallback(
            [this](const std::string& msg) {
                    this->processMessage(msg);
            }
        );
    }

    void ImGuiTabBar::addTab(const std::string& name, std::unique_ptr<EmbeddedWindow> window, 
        ImGuiTabItemFlags flags) {
        // Create a callback that calls the embedded window's draw method
        // The shared_ptr ensures the window stays alive as long as the callback exists
        auto windowPtr = std::shared_ptr<EmbeddedWindow>(window.release());
        auto callback = windowPtr ? std::function<void()>(
            [windowPtr]() { windowPtr->draw(); }
        ) : nullptr;
        
        tabs_.emplace_back(Tab{ name, windowPtr, nullptr, std::move(callback), flags });
    }

    void ImGuiTabBar::addTab(const std::string& name, EmbeddedWindow* window, 
        ImGuiTabItemFlags flags) {
        // Non-owning: caller manages lifetime
        // Capture raw pointer - safe as long as caller ensures lifetime
        auto callback = window ? std::function<void()>(
            [window]() { window->draw(); }
        ) : nullptr;
        
        tabs_.emplace_back(Tab{ name, nullptr, window, std::move(callback), flags });
    }

    void ImGuiTabBar::addTab(const std::string& name, std::function<void()> callback, 
        ImGuiTabItemFlags flags) {
        tabs_.emplace_back(Tab{ name, nullptr, nullptr, std::move(callback), flags });
    }

    bool ImGuiTabBar::removeTab(const std::string& name) {
        auto it = std::find_if(tabs_.begin(), tabs_.end(),
            [&name](const Tab& tab) { return tab.name == name; });
        
        if (it != tabs_.end()) {
            if (auto* window = it->getWindow()) {
                window->save();
            }
            tabs_.erase(it);
            return true;
        }
        return false;
    }

    void ImGuiTabBar::draw() {
        if (ImGui::BeginTabBar("QaplaTabBar", ImGuiTabBarFlags_Reorderable)) {
            // Draw all tabs

            auto maxIndex = tabs_.size();
            std::optional<size_t> closeIndex;
            for (auto index = size_t{0}; index < maxIndex; ++index) {
                auto& tab = tabs_[index];
                bool open = true;
                
                // Set flags based on whether tab is closable
                ImGuiTabItemFlags flags = tab.defaultTabFlags;
                tab.defaultTabFlags &= ~ImGuiTabItemFlags_SetSelected;
                bool closable = flags & ImGuiTabItemFlags_NoAssumedClosure;

                // Check if window is highlighted
                auto* window = tab.getWindow();
                bool isHighlighted = window && window->highlighted();
                auto translatedName = Translator::instance().translate("Tab", tab.name);
                auto tabItemText = std::format("{}###{}", translatedName, tab.name);
                bool tabIsActive = ImGui::BeginTabItem(tabItemText.c_str(), closable ? &open : nullptr, flags);
                
                // Draw dot if window is highlighted (after BeginTabItem, independent of whether tab is active)
                if (isHighlighted) {
                    constexpr float dotOffsetX = 6.0F;
                    constexpr float dotOffsetY = 10.0F;
                    ImGuiControls::drawDot(dotOffsetX, dotOffsetY);
                }
                
                if (tabIsActive) {
                    // Always use callback (which is created for EmbeddedWindows too)
                    if (tab.callback) {
                        tab.callback();
                    }
                    ImGui::EndTabItem();
                }
                
                // Mark tab for deletion if closed
                if (closable && !open) {
                    closeIndex = index;
                }
            }
            
            // Remove closed tabs in a separate loop
            if (closeIndex.has_value()) {
                tabs_.erase(tabs_.begin() + *closeIndex);
            }
            
            // Add + button if callback is set
            if (addTabCallback_) {
                if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
                    addTabCallback_(*this);
                }
            }
            
            // Call dynamic tabs callback after processing all static tabs
            if (dynamicTabsCallback_) {
                dynamicTabsCallback_();
            }
            
            ImGui::EndTabBar();
        }
    }

    void ImGuiTabBar::setDynamicTabsCallback(std::function<void()> callback) {
        dynamicTabsCallback_ = std::move(callback);
    }

    void ImGuiTabBar::setAddTabCallback(std::function<void(ImGuiTabBar&)> callback) {
        addTabCallback_ = std::move(callback);
    }

    size_t ImGuiTabBar::getTabCount() const noexcept {
        return tabs_.size();
    }

    bool ImGuiTabBar::hasTab(const std::string& name) const {
        return std::find_if(tabs_.begin(), tabs_.end(),
            [&name](const Tab& tab) { return tab.name == name; }) != tabs_.end();
    }
    
    void ImGuiTabBar::processMessage(const std::string& message) {
        std::map<std::string, std::string> commands = {
            { "switch_to_tournament_view", "Tournament" },
            { "switch_to_sprt_view", "SPRT" },
            { "switch_to_epd_view", "Epd" }
        };
        for (const auto& [cmd, tabName] : commands) {
            if (message == cmd) {
                for (auto& tab : tabs_) {
                    if (tab.name == tabName) {
                        tab.defaultTabFlags = static_cast<ImGuiTabItemFlags>(
                            tab.defaultTabFlags | ImGuiTabItemFlags_SetSelected);
                    }
                }
            }
        }
        
        // Handle board switching messages (format: "switch_to_board_<id>")
        if (message.rfind("switch_to_board_", 0) == 0) {
            try {
                auto idStr = message.substr(16);  // Skip "switch_to_board_"
                auto boardId = static_cast<uint32_t>(std::stoul(idStr));
                std::string boardName = "Board " + std::to_string(boardId);
                for (auto& tab : tabs_) {
                    if (tab.name == boardName) {
                        tab.defaultTabFlags = static_cast<ImGuiTabItemFlags>(
                            tab.defaultTabFlags | ImGuiTabItemFlags_SetSelected);
                        break;
                    }
                }
            } catch (...) {
                // Invalid board ID, ignore
            }
        }
    }

} // namespace QaplaWindows