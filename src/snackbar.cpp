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

#include "snackbar.h"
#include "configuration.h"

#include "qapla-tester/string-helper.h"

#include <imgui.h>

void SnackbarManager::show(const std::string& message, SnackbarType type, bool sticky) {
    SnackbarEntry entry;
    
    // Check if the same message with the same type is already at the top of the stack
    if (!snackbarStack_.empty() && 
        snackbarStack_.back().message == message && 
        snackbarStack_.back().type == type) {
        // Reset the start time to extend the display duration
        snackbarStack_.back().startTime = std::chrono::steady_clock::now();
        return;
    }
    
    entry.message = message;
    // Remove leading newline if present
    if (!entry.message.empty() && entry.message[0] == '\n') {
        entry.message.erase(0, 1); 
    }
    
    entry.startTime = std::chrono::steady_clock::now();
    entry.duration = getDuration(type);
    entry.type = type;
    entry.sticky = sticky;
    entry.isTutorial = false;
    
    snackbarStack_.emplace_back(std::move(entry)); 
}

void SnackbarManager::showTutorial(const std::string& message, SnackbarType type, bool sticky) {
    SnackbarEntry entry;
    
    entry.message = message;
    // Remove leading newline if present
    if (!entry.message.empty() && entry.message[0] == '\n') {
        entry.message.erase(0, 1); 
    }
    
    entry.startTime = std::chrono::steady_clock::now();
    entry.duration = getDuration(type);
    entry.type = type;
    entry.sticky = sticky;
    entry.isTutorial = true;
    
    snackbarStack_.emplace_back(std::move(entry)); 
}

void SnackbarManager::draw() {
    constexpr float snackbarWidth = 450.0F;
    constexpr float minSnackbarHeight = 120.0F;
    constexpr float maxSnackbarHeight = 400.0F;
    constexpr float closeButtonRadius = 10.0F;
    constexpr float borderThickness = 2.0F;
    constexpr float padding = 20.0F;

    while (!snackbarStack_.empty()) {
        auto& currentSnackbar = snackbarStack_.back();
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - currentSnackbar.startTime).count();

        // Remove expired snackbars (but not sticky ones)
        if (!currentSnackbar.sticky && elapsed > currentSnackbar.duration) {
            // If it's a tutorial snackbar, increment counter when it expires
            snackbarStack_.pop_back();
            if (currentSnackbar.isTutorial) {
                showNextTutorialStep();
            }
            continue;
        }

        ImVec4 bgColor = colors[static_cast<int>(currentSnackbar.type)];
        ImVec4 borderColor = ImVec4(bgColor.x + 0.2f, bgColor.y + 0.2f, bgColor.z + 0.2f, 1.0F);

        // Calculate required height for text
        ImGui::PushFont(ImGui::GetFont());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
        
        float availableWidth = snackbarWidth - 2 * padding;
        float fontSize = ImGui::GetFontSize() * 1.1f;
        
        // Calculate text size with wrapping
        std::string fullText = std::string(typeNames[static_cast<int>(currentSnackbar.type)]) + ":\n" + currentSnackbar.message;
        ImVec2 textSize = ImGui::CalcTextSize(fullText.c_str(), nullptr, false, availableWidth);
        
        // Calculate required height (text + padding + close button space)
        float requiredHeight = textSize.y + 2 * padding + 20.0F;
        float snackbarHeight = std::max(minSnackbarHeight, std::min(requiredHeight, maxSnackbarHeight));
        
        ImVec2 snackbarSize = ImVec2(snackbarWidth, snackbarHeight);
        
        ImGui::PopStyleVar();
        ImGui::PopFont();

        ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColor);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0F);

        ImVec2 windowPos = ImGui::GetMainViewport()->Pos;
        ImVec2 windowSize = ImGui::GetMainViewport()->Size;
        ImVec2 snackbarPos = ImVec2(windowPos.x + 20.0F, windowPos.y + windowSize.y - snackbarSize.y - 20.0F);

        ImGui::SetNextWindowPos(snackbarPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(snackbarSize, ImGuiCond_Always); 

        ImGui::Begin("##Snackbar", nullptr, ImGuiWindowFlags_NoDecoration);

        // Draw border
        auto drawList = ImGui::GetWindowDrawList();
        ImVec2 p1 = ImVec2(snackbarPos.x, snackbarPos.y);
        ImVec2 p2 = ImVec2(snackbarPos.x + snackbarSize.x, snackbarPos.y + snackbarSize.y);
        drawList->AddRect(p1, p2, ImColor(borderColor), 10.0F, 0, borderThickness);

        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x); 
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0F, 0.0F, 0.0F, 1.0F));
        ImGui::SetWindowFontScale(1.1f);
        ImGui::SetCursorPos(ImVec2(0.0F, 20.0F));
        ImGui::Indent(20.0F); 
        ImGui::Text("%s:", typeNames[static_cast<int>(currentSnackbar.type)]);
        ImGui::Text("%s", currentSnackbar.message.c_str());
        ImGui::Unindent(20.0F);
        ImGui::SetWindowFontScale(1.0F);
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos(); 

        ImVec2 closeButtonPos = ImVec2(
            snackbarPos.x + snackbarSize.x - closeButtonRadius - 10.0F, 
            snackbarPos.y + closeButtonRadius + 10.0F                  
        );
        
        if (drawCloseButton(closeButtonPos, closeButtonRadius)) {
            bool wasTutorial = currentSnackbar.isTutorial;
            snackbarStack_.pop_back();
            
            // Handle tutorial progression for tutorial snackbars
            if (wasTutorial) {
                showNextTutorialStep();
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        break; // Only show the most recent snackbar
    }
}

bool SnackbarManager::drawCloseButton(const ImVec2& position, float radius) {
    auto drawList = ImGui::GetWindowDrawList();

    // Draw circle background
    drawList->AddCircleFilled(position, radius, ImColor(1.0F, 1.0F, 1.0F, 0.9f));
    drawList->AddCircle(position, radius, ImColor(0.0F, 0.0F, 0.0F, 0.9f), 16, 1.5f);

    // Draw "X" mark 
    float lineThickness = 2.0F;
    float crossSize = radius * 0.5f;
    ImVec2 lineStart1 = ImVec2(position.x - crossSize, position.y - crossSize);
    ImVec2 lineEnd1 = ImVec2(position.x + crossSize, position.y + crossSize);
    ImVec2 lineStart2 = ImVec2(position.x - crossSize, position.y + crossSize);
    ImVec2 lineEnd2 = ImVec2(position.x + crossSize, position.y - crossSize);

    drawList->AddLine(lineStart1, lineEnd1, ImColor(0.0F, 0.0F, 0.0F, 0.9f), lineThickness);
    drawList->AddLine(lineStart2, lineEnd2, ImColor(0.0F, 0.0F, 0.0F, 0.9f), lineThickness);

    // Create invisible button for interaction
    ImGui::SetCursorScreenPos(ImVec2(position.x - radius, position.y - radius));
    ImGui::InvisibleButton("CloseButton", ImVec2(radius * 2, radius * 2));
    
    return ImGui::IsItemClicked();
}

float SnackbarManager::getDuration(SnackbarType type) const {
    switch (type) {
        case SnackbarType::Note:
            return static_cast<float>(config_.noteDurationInS);
        case SnackbarType::Success:
            return static_cast<float>(config_.successDurationInS);
        case SnackbarType::Warning:
            return static_cast<float>(config_.warningDurationInS);
        case SnackbarType::Error:
            return static_cast<float>(config_.errorDurationInS);
        default:
            return 10.0F;
    }
}

void SnackbarManager::loadConfiguration() {
    auto sections = QaplaConfiguration::Configuration::instance().
        getConfigData().getSectionList("snackbar", "snackbar").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!sections.empty()) {
        const auto& section = sections[0];
        config_.noteDurationInS = QaplaHelpers::to_uint32(section.getValue("noteduration").value_or("10")).value_or(10);
        config_.successDurationInS = QaplaHelpers::to_uint32(section.getValue("successduration").value_or("10")).value_or(10);
        config_.warningDurationInS = QaplaHelpers::to_uint32(section.getValue("warningduration").value_or("15")).value_or(15);
        config_.errorDurationInS = QaplaHelpers::to_uint32(section.getValue("errorduration").value_or("20")).value_or(20);
        config_.snackbarTutorialCounter = QaplaHelpers::to_uint32(section.getValue("tutorialcounter").value_or("0")).value_or(0);
    }
    
    showNextTutorialStep();
}

void SnackbarManager::updateConfiguration() const {
    QaplaHelpers::IniFile::Section section {
        .name = "snackbar",
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", "snackbar"},
            {"noteduration", std::to_string(config_.noteDurationInS)},
            {"successduration", std::to_string(config_.successDurationInS)},
            {"warningduration", std::to_string(config_.warningDurationInS)},
            {"errorduration", std::to_string(config_.errorDurationInS)},
            {"tutorialcounter", std::to_string(config_.snackbarTutorialCounter)}
        }
    };
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("snackbar", "snackbar", { section });
}

void SnackbarManager::showNextTutorialStep() {
    config_.snackbarTutorialCounter++;
    updateConfiguration();
    switch (config_.snackbarTutorialCounter) {
        case 1:
        showTutorial("Welcome to the Snackbar System!\n\n"
                     "Snackbars display temporary notifications in the bottom-left corner. "
                     "They automatically disappear after a few seconds. "
                     "This is a 'sticky' note - it stays until you close it manually by clicking the X button.", 
                     SnackbarType::Note, true);
        return;
        case 2:
        showTutorial("This is an example of a warning\n\n"
                    "There are 4 types of snackbars:\n"
                    "- Note, Success, Warning, and Error.\n\n"
                    "Each type has a different display duration.",
                    SnackbarType::Warning, false);
        return;
        case 3:
        showTutorial("Snackbar Configuration\n\n"
                     "You can customize the display duration for each snackbar type in the Settings window.\n"
                     "Go to the 'Settings' tab and check the 'Snackbar Settings' section.\n\n"
                     "Red dots show where to click next - setup engines now.", 
                     SnackbarType::Note, false);
        return;
        default:
        return;
    }
}

void SnackbarManager::finishTutorial() {
    config_.snackbarTutorialCounter = 4;
    updateConfiguration();
}

void SnackbarManager::resetTutorialCounter() {
    config_.snackbarTutorialCounter = 0;
    showNextTutorialStep();
    updateConfiguration();
}