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
#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

namespace QaplaWindows {

    /**
     * ImGuiTabBar manages multiple EmbeddedWindows inside a tab bar.
     */
    class ImGuiTabBar : public EmbeddedWindow {
    public:


        /**
         * @brief Add a tab to the tab bar.
         * @param name The name of the tab.
         * @param window The window to display in the tab.
         */
        void addTab(std::string name, std::unique_ptr<EmbeddedWindow> window) {
            tabs.emplace_back(Tab{ std::move(name), std::move(window) });
        }

        /**
         * @brief Remove a tab by name.
         * @param name The name of the tab to remove.
         */
        void removeTab(std::string name) {
            tabs.erase(std::remove_if(tabs.begin(), tabs.end(),
                [&name](const Tab& tab) { return tab.name == name; }), tabs.end());
        }

        void draw() override {
            if (ImGui::BeginChild("##TabBarChild", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None)) {
                if (ImGui::BeginTabBar("QaplaTabBar")) {
                    for (auto& tab : tabs) {
                        if (ImGui::BeginTabItem(tab.name.c_str())) {
                            tab.window->draw();
                            ImGui::EndTabItem();
                        }
                        if (dynamicTabsCallback) {
                            dynamicTabsCallback();
                        }
                    }
                    ImGui::EndTabBar();
                }
                ImGui::EndChild();
            }
        }

        /**
         * @brief Sets a callback to draw additional dynamic tabs.
         * @param callback The function to be called after drawing static tabs.
         */
        void setDynamicTabsCallback(std::function<void()> callback) {
            dynamicTabsCallback = std::move(callback);
        }

    private:
        struct Tab {
            std::string name;
            std::unique_ptr<EmbeddedWindow> window;
        };
        std::vector<Tab> tabs;
        std::function<void()> dynamicTabsCallback;

    };

} // namespace QaplaWindows

