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

#include "chatbot-snackbar-capture.h"
#include "imgui-controls.h"

#include <algorithm>

namespace QaplaWindows::ChatBot {

// ChatbotStepSnackbarMessage implementation

ChatbotStepSnackbarMessage::ChatbotStepSnackbarMessage(SnackbarManager::SnackbarEntry entry)
    : entry_(std::move(entry)) {
    // This step is immediately finished - it just displays a message
    finished_ = true;
}

std::string ChatbotStepSnackbarMessage::draw() {
    // Choose color based on snackbar type
    ImVec4 color;
    switch (entry_.type) {
        case SnackbarManager::SnackbarType::Error:
            color = StepColors::ERROR_COLOR;
            break;
        case SnackbarManager::SnackbarType::Warning:
            color = StepColors::WARNING_COLOR;
            break;
        case SnackbarManager::SnackbarType::Success:
            color = StepColors::SUCCESS_COLOR;
            break;
        case SnackbarManager::SnackbarType::Note:
        default:
            color = ImVec4(0.8F, 0.8F, 0.8F, 1.0F);  // Light gray for notes
            break;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGuiControls::textWrapped(entry_.message);
    ImGui::PopStyleColor();
    
    ImGui::Spacing();

    return "";  // Normal continuation
}

// SnackbarCapture implementation

SnackbarCapture::SnackbarCapture(std::string topic)
    : topics_{std::move(topic)} {
}

SnackbarCapture::SnackbarCapture(std::vector<std::string> topics)
    : topics_(std::move(topics)) {
}

void SnackbarCapture::install() {
    if (filterHandle_) {
        return;  // Already installed
    }

    filterHandle_ = SnackbarManager::instance().registerFilterCallback(
        [this](const SnackbarManager::SnackbarEntry& entry) -> bool {
            if (matchesTopic(entry.topic)) {
                pendingMessages_.push_back(entry);
                return false;  // Don't display in snackbar
            }
            return true;  // Let other messages through
        }
    );
}

void SnackbarCapture::uninstall() {
    filterHandle_.reset();
}

bool SnackbarCapture::matchesTopic(const std::string& topic) const {
    return std::find(topics_.begin(), topics_.end(), topic) != topics_.end();
}

void SnackbarCapture::insertCapturedSteps(
    std::vector<std::unique_ptr<ChatbotStep>>& steps,
    size_t currentStepIndex) {
    
    if (pendingMessages_.empty()) {
        return;
    }

    // Insert all pending messages as steps right after currentStepIndex
    // We insert at currentStepIndex + 1 so they appear after the current step
    size_t insertPosition = currentStepIndex + 1;
    
    // Make sure we don't insert beyond the end
    if (insertPosition > steps.size()) {
        insertPosition = steps.size();
    }

    for (auto& entry : pendingMessages_) {
        auto messageStep = std::make_unique<ChatbotStepSnackbarMessage>(std::move(entry));
        steps.insert(steps.begin() + static_cast<ptrdiff_t>(insertPosition), std::move(messageStep));
        ++insertPosition;  // Next message goes after this one
    }

    pendingMessages_.clear();
}

} // namespace QaplaWindows::ChatBot
