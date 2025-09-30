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
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>

namespace QaplaWindows {

    /**
     * @brief ImGuiTabBar manages multiple EmbeddedWindows or callbacks inside a tab bar.
     * 
     * This class provides a tabbed interface for displaying multiple windows or content areas.
     * Tabs can contain either EmbeddedWindow instances or custom callback functions.
     * The tab bar automatically manages the lifetime of embedded windows.
     */
    class ImGuiTabBar : public EmbeddedWindow {
    public:

        /**
         * @brief Add a tab with an EmbeddedWindow.
         * 
         * Creates a tab that displays the content of the provided EmbeddedWindow.
         * The window's lifetime is managed by the tab bar - it will be destroyed
         * when the tab is removed or the tab bar is destroyed.
         * 
         * @param name The display name of the tab
         * @param window Unique pointer to the EmbeddedWindow to display (ownership transferred)
         * @param flags Additional ImGuiTabItemFlags for the tab (default: ImGuiTabItemFlags_None)
         */
        void addTab(const std::string& name, std::unique_ptr<EmbeddedWindow> window,
            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None);

        /**
         * @brief Add a tab with a custom callback function.
         * 
         * Creates a tab that executes the provided callback function when drawn.
         * The callback is responsible for rendering the tab's content.
         * 
         * @param name The display name of the tab
         * @param callback Function to call when drawing the tab content
         * @param flags Additional ImGuiTabItemFlags for the tab (default: ImGuiTabItemFlags_None)
         */
        void addTab(const std::string& name, std::function<void()> callback, 
            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None);

        /**
         * @brief Remove a tab by name.
         * 
         * Removes the first tab found with the specified name. If the tab contained
         * an EmbeddedWindow, it will be destroyed automatically.
         * 
         * @param name The name of the tab to remove
         * @return true if a tab was removed, false if no tab with that name was found
         */
        bool removeTab(const std::string& name);

        /**
         * @brief Render the tab bar and its contents.
         * 
         * Draws the ImGui tab bar with all registered tabs. For each visible tab,
         * executes its associated callback or draws its EmbeddedWindow.
         * Also calls the dynamic tabs callback if set.
         */
        void draw() override;

        /**
         * @brief Set a callback for drawing additional dynamic tabs.
         * 
         * This callback is executed after all static tabs are processed.
         * It can be used to add tabs that are created dynamically at runtime.
         * 
         * @param callback Function to call for drawing dynamic tabs, or nullptr to disable
         */
        void setDynamicTabsCallback(std::function<void()> callback);

        /**
         * @brief Set a callback for adding new tabs via the + button.
         * 
         * This callback is executed when the user clicks the + button in the tab bar.
         * The callback receives a reference to the TabBar itself, allowing safe access
         * to add new tabs without ownership concerns.
         * 
         * @param callback Function to call when adding a new tab, receives TabBar reference,
         *                 or nullptr to disable the + button
         */
        void setAddTabCallback(std::function<void(ImGuiTabBar&)> callback);

        /**
         * @brief Get the number of tabs in the tab bar.
         * 
         * @return The current number of tabs
         */
        size_t getTabCount() const noexcept;

        /**
         * @brief Check if a tab with the given name exists.
         * 
         * @param name The name to search for
         * @return true if a tab with this name exists, false otherwise
         */
        bool hasTab(const std::string& name) const;

    private:
        struct Tab {
            std::string name;
            // We keep the window here to manage its lifetime
            // Using shared_ptr allows the callback to safely reference the window
            std::shared_ptr<EmbeddedWindow> window;
            std::function<void()> callback;
            ImGuiTabItemFlags defaultTabFlags = ImGuiTabItemFlags_None;
        };
        std::vector<Tab> tabs_;
        std::function<void()> dynamicTabsCallback_;
        std::function<void(ImGuiTabBar&)> addTabCallback_; // Callback for adding new tabs via + button
    };

} // namespace QaplaWindows

