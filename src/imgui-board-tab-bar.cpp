/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "imgui-board-tab-bar.h"

#include "interactive-board-window.h"
#include "viewer-board-window-list.h"
#include "chatbot/chatbot-window.h"

namespace QaplaWindows {

ImGuiBoardTabBar::ImGuiBoardTabBar() {
    // Load and add all board instances
    addTab("Chatbot", ChatBot::ChatbotWindow::instance());
    
    auto instances = InteractiveBoardWindow::loadInstances();
    for (size_t index = 0; index < instances.size(); ++index) {
        auto title = instances[index]->getTitle();
        addTab(title, std::move(instances[index]), 
            index == 0 ? ImGuiTabItemFlags_None : ImGuiTabItemFlags_NoAssumedClosure);
    }

    // Set callback for the + button
    setAddTabCallback([](ImGuiTabBar& tabBar) {
        auto instance = InteractiveBoardWindow::createInstance();
        auto title = instance->getTitle();
        tabBar.addTab(
            title,
            std::move(instance),
            static_cast<ImGuiTabItemFlags>(
                ImGuiTabItemFlags_NoAssumedClosure | ImGuiTabItemFlags_SetSelected));
    });

    // Set dynamic tabs for viewer boards
    setDynamicTabsCallback([]() {
        ViewerBoardWindowList::drawAllTabs();
    });

    // Handle create_board and switch_to_board messages from chatbot
    messageHandle_ = StaticCallbacks::message().registerCallback(
        [this](const std::string& message) {
            if (message == "create_board") {
                auto instance = InteractiveBoardWindow::createInstance();
                auto title = instance->getTitle();
                addTab(
                    title,
                    std::move(instance),
                    ImGuiTabItemFlags_NoAssumedClosure);
            } else if (message.rfind("switch_to_board_", 0) == 0) {
                // Extract board ID from message and switch to board tab
                processMessage(message);
            }
        });
}

} // namespace QaplaWindows
