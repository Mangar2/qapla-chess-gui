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
#include <imgui.h>

ChatbotWindow::ChatbotWindow() {
    registerThread(std::make_unique<ChatbotChooseLanguage>());
    resetToMainMenu();
}

void ChatbotWindow::registerThread(std::unique_ptr<ChatbotThread> thread) {
    m_registeredThreads.push_back(std::move(thread));
    resetToMainMenu();
}

void ChatbotWindow::startThread(const ChatbotThread& threadPrototype) {
    m_activeThread = threadPrototype.clone();
    m_activeThread->start();
    m_mainMenuStep.reset();
}

void ChatbotWindow::resetToMainMenu() {
    std::vector<ChatbotStepOptionList::Option> options;
    for (const auto& thread : m_registeredThreads) {
        // Capture raw pointer to avoid reference invalidation on vector resize
        ChatbotThread* ptr = thread.get();
        options.push_back({
            thread->getTitle(),
            [this, ptr]() { startThread(*ptr); }
        });
    }
    m_mainMenuStep = std::make_unique<ChatbotStepOptionList>("How can I help you?", std::move(options));
}

void ChatbotWindow::draw() {
    // Draw history
    if (!m_completedThreads.empty()) {
        if (ImGui::CollapsingHeader("History")) {
            for (const auto& thread : m_completedThreads) {
                ImGui::TextDisabled("%s", thread->getTitle().c_str());
            }
        }
        ImGui::Separator();
    }

    if (m_activeThread) {
        m_activeThread->draw();
        if (m_activeThread->isFinished()) {
            m_completedThreads.push_back(std::move(m_activeThread));
            resetToMainMenu();
        }
    } else {
        if (!m_mainMenuStep) {
            resetToMainMenu();
        }
        m_mainMenuStep->draw();
    }
}
