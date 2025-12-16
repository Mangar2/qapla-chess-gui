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
 *
 * @author GitHub Copilot
 * @copyright Copyright (c) 2025 GitHub Copilot
 */

#include "chatbot-window.h"
#include "chatbot-step-option-list.h"
#include "chatbot-choose-language.h"
#include "chatbot-tournament.h"
#include "tutorial/chatbot-tutorial.h"
#include "chatbot-messages.h"
#include "sprt/chatbot-sprt.h"
#include "epd/chatbot-epd.h"
#include "board/chatbot-board.h"
#include "add-engines/chatbot-add-engines.h"
#include "i18n.h"
#include "imgui-controls.h"
#include "snackbar.h"

#include <imgui.h>

#include <format>

namespace QaplaWindows::ChatBot {

// Constants for layout
constexpr float BOTTOM_PADDING = 40.0F;
constexpr float RIGHT_MARGIN = 5.0F;
constexpr float LEFT_MARGIN = 10.0F;

ChatbotWindow::ChatbotWindow() {
    initializeThreads();
    resetToMainMenu();
}

void ChatbotWindow::initializeThreads() {
    registeredThreads_.clear();
    registeredThreads_.push_back(std::make_unique<ChatbotTournament>());
    registeredThreads_.push_back(std::make_unique<ChatbotSprt>());
    registeredThreads_.push_back(std::make_unique<ChatbotEpd>());
    registeredThreads_.push_back(std::make_unique<ChatbotBoard>());
    registeredThreads_.push_back(std::make_unique<ChatbotAddEngines>());
    registeredThreads_.push_back(std::make_unique<ChatbotTutorial>());
    registeredThreads_.push_back(std::make_unique<ChatbotChooseLanguage>());
    registeredThreads_.push_back(std::make_unique<ChatbotMessages>());
}

void ChatbotWindow::registerThread(std::unique_ptr<ChatbotThread> thread) {
    registeredThreads_.push_back(std::move(thread));
    resetToMainMenu();
}

void ChatbotWindow::startThread(const ChatbotThread& threadPrototype) {
    activeThread_ = threadPrototype.clone();
    activeThread_->start();
    // Do not reset mainMenuStep_ here, as we might be inside its draw() method (callback).
    // It will be replaced when resetToMainMenu() is called later.
}

void ChatbotWindow::resetToMainMenu() {
    std::vector<ChatbotStepOptionList::Option> options;
    for (const auto& thread : registeredThreads_) {
        // Capture raw pointer to avoid reference invalidation on vector resize
        ChatbotThread* ptr = thread.get();
        options.push_back({
            Translator::instance().translate("Chatbot", thread->getTitle()),
            [this, ptr]() { 
                startThread(*ptr); 
            }
        });
    }
    mainMenuStep_ = std::make_unique<ChatbotStepOptionList>(
        Translator::instance().translate("Chatbot", "How can I help you?"),
        std::move(options)
    );
    lastCursorY_ = 0.0F;  // Reset scroll tracking when content is cleared
}

void ChatbotWindow::reset() {
    activeThread_.reset();
    completedThreads_.clear();
    initializeThreads();
    resetToMainMenu();
}

void ChatbotWindow::draw() {
    
    // Outer child window: provides vertical scrollbar
    ImGui::BeginChild("ChatbotWindowOuter", ImVec2(0, 0), false, 
        ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
    
    // Inner child window: provides margins, auto-resizes to content height so outer window scrolls
    ImGui::Indent(LEFT_MARGIN);
    ImVec2 innerSize = ImGui::GetContentRegionAvail();
    innerSize.x -= RIGHT_MARGIN;
    innerSize.y = 0;  // Auto-size height to content
    ImGui::BeginChild("ChatbotWindowInner", innerSize, 
        ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeX, 
        ImGuiWindowFlags_NoScrollbar);
    
    try {
        ImGui::Spacing();
        // Draw history
        if (!completedThreads_.empty()) {
            if (ImGuiControls::CollapsingHeaderWithDot("History")) {
                for (const auto& thread : completedThreads_) {
                    QaplaWindows::ImGuiControls::textDisabled(thread->getTitle());
                }
            }
            ImGui::Separator();
        }
        
        if (activeThread_) {
            static_cast<void>(activeThread_->draw());
            if (activeThread_->isFinished()) {
                completedThreads_.push_back(std::move(activeThread_));
                resetToMainMenu();
            }
        } else {
            if (!mainMenuStep_) {
                resetToMainMenu();
            }
            static_cast<void>(mainMenuStep_->draw());
        }
        
        // Add dummy space at the bottom to ensure current chat stays visible
        ImGui::Dummy(ImVec2(0.0F, BOTTOM_PADDING));
        
    } catch (const std::exception& e) {
        SnackbarManager::instance().showError(
            std::format("An error occurred in the Chatbot window:\n{}", e.what()));
    }
    ImGui::EndChild();
    ImGui::Unindent(LEFT_MARGIN);
    
    // Auto-scroll to bottom when content grows
    // Use GetCursorPos() instead of GetCursorScreenPos() to get scroll-independent position
    float currentCursorY = ImGui::GetCursorPos().y;
    if (currentCursorY > lastCursorY_) {
        ImGui::SetScrollHereY(1.0F);
    }
    lastCursorY_ = currentCursorY;
    
    ImGui::EndChild();
}

} // namespace QaplaWindows::ChatBot
