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
#include "i18n.h"
#include "imgui-controls.h"

#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotWindow::ChatbotWindow() {
    registerThread(std::make_unique<ChatbotChooseLanguage>());
    registerThread(std::make_unique<ChatbotTournament>());
    resetToMainMenu();
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
            [this, ptr]() { startThread(*ptr); }
        });
    }
    mainMenuStep_ = std::make_unique<ChatbotStepOptionList>(
        Translator::instance().translate("Chatbot", "How can I help you?"),
        std::move(options)
    );
}

void ChatbotWindow::draw() {
    ImGui::Spacing();
    ImGui::Indent(10.0F);
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
        activeThread_->draw();
        if (activeThread_->isFinished()) {
            completedThreads_.push_back(std::move(activeThread_));
            resetToMainMenu();
        }
    } else {
        if (!mainMenuStep_) {
            resetToMainMenu();
        }
        mainMenuStep_->draw();
    }
    ImGui::Unindent(10.0F);
}

} // namespace QaplaWindows::ChatBot
