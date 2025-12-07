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
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "chatbot-messages.h"
#include "imgui-controls.h"
#include "snackbar.h"

#include <imgui.h>
#include <format>

namespace QaplaWindows::ChatBot {

// ============================================================================
// ChatbotMessages implementation
// ============================================================================

void ChatbotMessages::start() {
    steps_.clear();
    currentStepIndex_ = 0;
    stopped_ = false;

    steps_.push_back(std::make_unique<ChatbotStepMessages>());
}

bool ChatbotMessages::draw() {
    if (stopped_ || steps_.empty()) {
        return false;
    }

    // Draw all completed steps 
    for (size_t i = 0; i < currentStepIndex_ && i < steps_.size(); ++i) {
        static_cast<void>(steps_[i]->draw());
    }

    // Draw and handle current step
    if (currentStepIndex_ < steps_.size()) {
        std::string result = steps_[currentStepIndex_]->draw();
        
        if (result == "stop") {
            stopped_ = true;
            return false;
        }

        if (steps_[currentStepIndex_]->isFinished()) {
            ++currentStepIndex_;
            return true;  // Content changed, need scroll
        }
    }

    return false;
}

bool ChatbotMessages::isFinished() const {
    return stopped_ || (currentStepIndex_ >= steps_.size());
}

std::unique_ptr<ChatbotThread> ChatbotMessages::clone() const {
    return std::make_unique<ChatbotMessages>();
}

// ============================================================================
// ChatbotStepMessages implementation
// ============================================================================

ChatbotStepMessages::ChatbotStepMessages() {
    displayCount_ = INITIAL_MESSAGE_COUNT;
}

std::string ChatbotStepMessages::draw() {
    const auto& history = SnackbarManager::instance().getHistory();
    
    if (history.empty()) {
        ImGuiControls::textWrapped("No messages in history.");
        ImGui::Spacing();
        
        if (ImGuiControls::textButton("Close")) {
            finished_ = true;
        }
        QaplaWindows::ImGuiControls::hooverTooltip("Close the message window and return to the previous view.");
        return "";
    }

    // Calculate how many messages to show (from newest to oldest)
    size_t totalMessages = history.size();
    size_t showCount = std::min(displayCount_, totalMessages);
    
    ImGuiControls::textWrapped(
        std::format("Showing {} of {} messages:", showCount, totalMessages));
    ImGui::Spacing();
    ImGui::Separator();
    
    // Display messages from newest to oldest
    for (size_t i = 0; i < showCount; ++i) {
        size_t index = totalMessages - 1 - i;
        const auto& entry = history[index];
        
        // Determine color based on type
        ImVec4 typeColor;
        const char* typeName;
        switch (entry.type) {
            case SnackbarManager::SnackbarType::Error:
                typeColor = StepColors::ERROR_COLOR;
                typeName = "Error";
                break;
            case SnackbarManager::SnackbarType::Warning:
                typeColor = StepColors::WARNING_COLOR;
                typeName = "Warning";
                break;
            case SnackbarManager::SnackbarType::Success:
                typeColor = StepColors::SUCCESS_COLOR;
                typeName = "Success";
                break;
            case SnackbarManager::SnackbarType::Note:
            default:
                typeColor = ImVec4(0.7F, 0.7F, 0.7F, 1.0F);
                typeName = "Note";
                break;
        }
        
        ImGui::Spacing();
        
        // Type label with color
        ImGui::TextColored(typeColor, "[%s]", typeName);
        
        // Topic if present
        if (!entry.topic.empty()) {
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", entry.topic.c_str());
        }
        
        // Message content
        ImGuiControls::textWrapped(entry.message);
        
        ImGui::Separator();
    }
    
    ImGui::Spacing();
    
    // "More" button - only show if there are more messages to display
    if (showCount < totalMessages) {
        if (ImGuiControls::textButton("More...")) {
            displayCount_ *= 2;  // Double the display count
        }
        QaplaWindows::ImGuiControls::hooverTooltip("Load older messages from history (double the display count).");
        ImGui::SameLine();
    }
    
    // Close button
    if (ImGuiControls::textButton("Close")) {
        finished_ = true;
    }
    QaplaWindows::ImGuiControls::hooverTooltip("Close the message window and return to the previous view.");
    
    return "";
}

} // namespace QaplaWindows::ChatBot
