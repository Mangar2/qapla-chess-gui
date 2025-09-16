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
#include <algorithm>

namespace QaplaWindows {

    void ImGuiTabBar::addTab(const std::string& name, std::unique_ptr<EmbeddedWindow> window) {
        // Create a callback that calls the embedded window's draw method
        // The shared_ptr ensures the window stays alive as long as the callback exists
        auto windowPtr = std::shared_ptr<EmbeddedWindow>(window.release());
        auto callback = windowPtr ? std::function<void()>(
            [windowPtr]() { windowPtr->draw(); }
        ) : nullptr;
        
        tabs.emplace_back(Tab{ name, windowPtr, std::move(callback) });
    }

    void ImGuiTabBar::addTab(const std::string& name, std::function<void()> callback) {
        tabs.emplace_back(Tab{ name, nullptr, std::move(callback) });
    }

    bool ImGuiTabBar::removeTab(const std::string& name) {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [&name](const Tab& tab) { return tab.name == name; });
        
        if (it != tabs.end()) {
            tabs.erase(it);
            return true;
        }
        return false;
    }

    void ImGuiTabBar::draw() {
        if (ImGui::BeginTabBar("QaplaTabBar")) {
            for (auto& tab : tabs) {
                if (ImGui::BeginTabItem(tab.name.c_str())) {
                    // Always use callback (which is created for EmbeddedWindows too)
                    if (tab.callback) {
                        tab.callback();
                    }
                    ImGui::EndTabItem();
                }
            }
            
            // Call dynamic tabs callback after processing all static tabs
            if (dynamicTabsCallback) {
                dynamicTabsCallback();
            }
            
            ImGui::EndTabBar();
        }
    }

    void ImGuiTabBar::setDynamicTabsCallback(std::function<void()> callback) {
        dynamicTabsCallback = std::move(callback);
    }

    size_t ImGuiTabBar::getTabCount() const noexcept {
        return tabs.size();
    }

    bool ImGuiTabBar::hasTab(const std::string& name) const {
        return std::find_if(tabs.begin(), tabs.end(),
            [&name](const Tab& tab) { return tab.name == name; }) != tabs.end();
    }

} // namespace QaplaWindows
