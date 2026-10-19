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
#include "tutorial.h"
#include "i18n.h"

#include <base-elements/string-helper.h>

#include "ui/qapla-ui.h"

namespace QaplaWindows {

SnackbarManager& SnackbarManager::instance() {
    static SnackbarManager instance;
    return instance;
}

void SnackbarManager::show(const std::string& message, SnackbarType type, bool sticky, bool isTutorial, const std::string& topic) {
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
    
    entry.topic = topic;
    entry.startTime = std::chrono::steady_clock::now();
    entry.duration = getDuration(type);
    entry.type = type;
    entry.sticky = sticky;
    entry.isTutorial = isTutorial;
    
    // Add to history (limit to MAX_HISTORY_SIZE entries)
    history_.push_back(entry);
    while (history_.size() > MAX_HISTORY_SIZE) {
        history_.pop_front();
    }
    
    // Call all filter callbacks - if ANY returns false, don't display
    // invokeEvery returns true only if ALL callbacks return true (or no callbacks registered)
    if (!filterCallbacks_.invokeEvery(entry)) {
        return;
    }
    
    snackbarStack_.emplace_back(std::move(entry)); 
}

void SnackbarManager::showTutorial(const std::string& message, SnackbarType type, bool sticky) {
    show(message, type, sticky, true, "tutorial");
}

bool SnackbarManager::isTutorialMessageVisible() const {
    // Prüfe, ob aktuell eine Tutorial-Message im Stack ist
    if (!snackbarStack_.empty()) {
        return snackbarStack_.back().isTutorial;
    }
    return false;
}

/**
 * @brief Draws a circular close button with an "X" mark
 * @param position The center position of the button
 * @param radius The radius of the button circle
 * @return true if the button was clicked, false otherwise
 */
static bool drawCloseButton(const QaplaUi::Vec2& position, float radius) {
    auto* drawList = &QaplaUi::windowDrawList();

    // Draw circle background
    drawList->addCircleFilled(position, radius, QaplaUi::toColor32(QaplaUi::Vec4(1.0F, 1.0F, 1.0F, 0.9F)));
    drawList->addCircle(position, radius, QaplaUi::toColor32(QaplaUi::Vec4(0.0F, 0.0F, 0.0F, 0.9F)), 16, 1.5F);

    // Draw "X" mark 
    float lineThickness = 2.0F;
    float crossSize = radius * 0.5F;
    auto lineStart1 = QaplaUi::Vec2(position.x - crossSize, position.y - crossSize);
    auto lineEnd1 = QaplaUi::Vec2(position.x + crossSize, position.y + crossSize);
    auto lineStart2 = QaplaUi::Vec2(position.x - crossSize, position.y + crossSize);
    auto lineEnd2 = QaplaUi::Vec2(position.x + crossSize, position.y - crossSize);

    drawList->addLine(lineStart1, lineEnd1, QaplaUi::toColor32(QaplaUi::Vec4(0.0F, 0.0F, 0.0F, 0.9F)), lineThickness);
    drawList->addLine(lineStart2, lineEnd2, QaplaUi::toColor32(QaplaUi::Vec4(0.0F, 0.0F, 0.0F, 0.9F)), lineThickness);

    // Create invisible button for interaction
    QaplaUi::setCursorScreenPos(QaplaUi::Vec2(position.x - radius, position.y - radius));
    QaplaUi::invisibleButton("CloseButton", QaplaUi::Vec2(radius * 2, radius * 2));
    
    return QaplaUi::isItemClicked();
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

        QaplaUi::Vec4 bgColor = colors[static_cast<int>(currentSnackbar.type)];
        QaplaUi::Vec4 borderColor = QaplaUi::Vec4(bgColor.x + 0.2F, bgColor.y + 0.2F, bgColor.z + 0.2F, 1.0F);

        // Calculate required height for text
        QaplaUi::pushStyleVar(QaplaUi::StyleVar::WindowPadding, QaplaUi::Vec2(padding, padding));
        
        float availableWidth = snackbarWidth - 2 * padding;
        
        // Calculate text size with wrapping
        std::string fullText = std::string(typeNames[static_cast<int>(currentSnackbar.type)]) + ":\n" + currentSnackbar.message;
        QaplaUi::Vec2 textSize = QaplaUi::calcTextSize(fullText, availableWidth);
        
        // Calculate required height (text + padding + close button space)
        float requiredHeight = textSize.y + 2 * padding + 20.0F;
        float snackbarHeight = std::max(minSnackbarHeight, std::min(requiredHeight, maxSnackbarHeight));
        
        auto snackbarSize = QaplaUi::Vec2(snackbarWidth, snackbarHeight);
        
        QaplaUi::popStyleVar();

        QaplaUi::pushStyleColor(QaplaUi::StyleColor::WindowBg, bgColor);
        QaplaUi::pushStyleVar(QaplaUi::StyleVar::WindowRounding, 10.0F);

        QaplaUi::Vec2 windowPos = QaplaUi::getViewportPos();
        QaplaUi::Vec2 windowSize = QaplaUi::getViewportSize();
        auto snackbarPos = QaplaUi::Vec2(windowPos.x + 20.0F, windowPos.y + windowSize.y - snackbarSize.y - 20.0F);

        QaplaUi::beginOverlay("##Snackbar", snackbarPos, snackbarSize, QaplaUi::WindowFlags::NoDecoration);

        // Draw border
        auto* drawList = &QaplaUi::windowDrawList();
        auto p1 = QaplaUi::Vec2(snackbarPos.x, snackbarPos.y);
        auto p2 = QaplaUi::Vec2(snackbarPos.x + snackbarSize.x, snackbarPos.y + snackbarSize.y);
        drawList->addRect(p1, p2, QaplaUi::toColor32(borderColor), 10.0F, borderThickness);

        QaplaUi::pushTextWrapPos(QaplaUi::getContentRegionAvail().x); 
        QaplaUi::pushStyleColor(QaplaUi::StyleColor::Text, QaplaUi::Vec4(0.0F, 0.0F, 0.0F, 1.0F));
        QaplaUi::setWindowFontScale(1.1F);
        QaplaUi::setCursorPos(QaplaUi::Vec2(0.0F, 20.0F));
        QaplaUi::indent(20.0F); 
        auto translatedType = Translator::instance().translate("Snackbar",
            typeNames[static_cast<int>(currentSnackbar.type)]);
        QaplaUi::text(translatedType + ":");
        auto translatedMessage = Translator::instance().translate("Snackbar",
            currentSnackbar.message);
        QaplaUi::text(translatedMessage);
        QaplaUi::unindent(20.0F);
        QaplaUi::setWindowFontScale(1.0F);
        QaplaUi::popStyleColor();
        QaplaUi::popTextWrapPos(); 

        auto closeButtonPos = QaplaUi::Vec2(
            snackbarPos.x + snackbarSize.x - closeButtonRadius - 10.0F, 
            snackbarPos.y + closeButtonRadius + 10.0F                  
        );
        
        if (drawCloseButton(closeButtonPos, closeButtonRadius)) {
            bool wasTutorial = currentSnackbar.isTutorial;
            snackbarStack_.pop_back();
            
            // Handle tutorial progression for tutorial snackbars
            if (wasTutorial) {
                showNextTutorialStep(true);
            }
        }

        QaplaUi::endOverlay();
        QaplaUi::popStyleVar();
        QaplaUi::popStyleColor();

        break; // Only show the most recent snackbar
    }
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
            {"errorduration", std::to_string(config_.errorDurationInS)}
        }
    };
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("snackbar", "snackbar", { section });
}

void SnackbarManager::showNextTutorialStep(bool endSticky) const {
    if (tutorialProgress_ == 1 && !endSticky) {
        Tutorial::instance().showLastTutorialStep(Tutorial::TutorialName::Snackbar);
        return;
    }
    if (tutorialProgress_ == 5) {
        Tutorial::instance().finishTutorial(Tutorial::TutorialName::Snackbar);
        return; 
    }
    Tutorial::instance().requestNextTutorialStep(Tutorial::TutorialName::Snackbar);
}

bool SnackbarManager::tutorialInitialized_ = []() {
    Tutorial::instance().setEntry({
        .name = Tutorial::TutorialName::Snackbar,
        .displayName = "Snackbar",
        .messages = {
            { .text = "Welcome to the Snackbar System!\n\n"
              "Snackbars display temporary notifications in the bottom-left corner. "
              "They automatically disappear after a few seconds. "
              "This is a 'sticky' note - it stays until you close it manually by clicking the X button.", 
              .type = SnackbarType::Note,
              .sticky = true },
            { .text = "This is an example of a warning\n\n"
              "There are 4 types of snackbars:\n"
              "- Note, Success, Warning, and Error.\n\n"
              "Each type has a different display duration.",
              .type = SnackbarType::Warning },
            { .text = 
              "You can customize the display duration for each snackbar type in the Settings window.\n"
              "Go to the 'Settings' tab and check the 'Snackbar Settings' section.\n"
              "If the tutorial is too fast, adjust the duration of the messages.", 
              .type = SnackbarType::Note },
            { .text = 
              "You can restart any tutorial from the beginning if needed.\n"
              "Go to the 'Settings' tab and uncheck the tutorial you want to restart.\n",
              .type = SnackbarType::Note },
            { .text = 
              "Now we start with the functionality.\n\n"
              "Red dots show where to click next - setup engines.", 
              .type = SnackbarType::Note }
        },
        .getProgressCounter = []() -> uint32_t& {
            return SnackbarManager::instance().tutorialProgress_;
        },
        .autoStart = true
    });
    return true;
}();

} // namespace QaplaWindows
