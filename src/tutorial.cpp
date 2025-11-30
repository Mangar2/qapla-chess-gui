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

#include "tutorial.h"
#include "configuration.h"
#include "snackbar.h"
#include "string-helper.h"

#include <optional>

namespace QaplaWindows {

bool Tutorial::allPrecedingCompleted(TutorialName name) const {
    const size_t currentIndex = toIndex(name);
    
    for (size_t i = 0; i < currentIndex; ++i) {
        if (!entries_[i].completed()) {
            return false;
        }
    }
    return true;
}

bool Tutorial::mayStart(TutorialName name) const {
    const auto& entry = entries_[toIndex(name)];
    
    // Auto-start starts only tutorials that are set to autoStart and have not yet begun
    if (!entry.autoStart || entry.getProgressCounter() != 0) {
        return false;
    }
    
    return allPrecedingCompleted(name);
}

void Tutorial::showLastTutorialStep(TutorialName name) {
    auto& entry = entries_[toIndex(name)];
    uint32_t messageIndex = entry.counter > 0 ? entry.counter - 1 : 0;
    if (messageIndex >= entry.messages.size()) {
        return;
    }
    
    if (!allPrecedingCompleted(name)) {
        return;
    }
    
    const auto& msg = entry.messages[messageIndex];
    SnackbarManager::instance().showTutorial(msg.text, msg.type, msg.sticky);
}

void Tutorial::showNextTutorialStep(TutorialName name) {
    auto& entry = entries_[toIndex(name)];
    
    if (entry.completed()) {
        return;
    }
    
    if (!allPrecedingCompleted(name)) {
        return;
    }
    
    entry.getProgressCounter()++;
    entry.showNextMessage();
    saveConfiguration();
}

void Tutorial::finishTutorial(TutorialName name) {
    auto& entry = entries_[toIndex(name)];
    
    if (entry.completed()) {
        return;
    }
    
    entry.finish();

    startNextTutorialIfAllowed();
    saveConfiguration();
}

void Tutorial::startNextTutorialIfAllowed()
{
    for (size_t i = 0; i < static_cast<size_t>(TutorialName::Count); ++i)
    {
        auto tutorialName = static_cast<TutorialName>(i);
        if (mayStart(tutorialName))
        {
            auto &autoStartEntry = entries_[i];
            autoStartEntry.getProgressCounter() = 1;
            autoStartEntry.showNextMessage();
        }
    }
}

void Tutorial::startTutorial(TutorialName name) {
    auto& entry = entries_[toIndex(name)];
    entry.reset();
    entry.getProgressCounter() = 1;
    entry.showNextMessage();
    saveConfiguration();
}

void Tutorial::restartTutorial(TutorialName name) {
    auto& entry = entries_[toIndex(name)];
    entry.reset();
    
    if (mayStart(name)) {
        entry.getProgressCounter() = 1;
        entry.showNextMessage();
    }
    
    saveConfiguration();
}

void Tutorial::resetAll() {
    for (size_t i = 0; i < static_cast<size_t>(TutorialName::Count); ++i) {
        auto& entry = entries_[i];
        entry.reset();
    }
    
    startNextTutorialIfAllowed();
    saveConfiguration();
}

void Tutorial::loadConfiguration() {
    auto sections = QaplaConfiguration::Configuration::instance().
        getConfigData().getSectionList("tutorial", "tutorial").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!sections.empty()) {
        const auto& section = sections[0];
        
        for (size_t i = 0; i < static_cast<size_t>(TutorialName::Count); ++i) {
            auto& entry = entries_[i];

            // Use displayName + Index as Config-Key for Uniqueness
            auto configName = entry.displayName + std::to_string(i);
            
            auto valueOpt = section.getValue(configName).value_or("0");
            entry.counter = QaplaHelpers::to_uint32(valueOpt).value_or(0);
            entry.getProgressCounter() = entry.counter;
        }
    }

    for (size_t i = 0; i < static_cast<size_t>(TutorialName::Count); ++i) {
        auto name = static_cast<TutorialName>(i);
        if (mayStart(name)) {
            entries_[i].getProgressCounter() = 1;
        }
    }
}

void Tutorial::saveConfiguration() const {
    QaplaHelpers::IniFile::Section section {
        .name = "tutorial",
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", "tutorial"}
        }
    };
    
    for (size_t i = 0; i < static_cast<size_t>(TutorialName::Count); ++i) {
        const auto& entry = entries_[i];
        auto configName = entry.displayName + std::to_string(i);
        section.entries.emplace_back(std::move(configName), std::to_string(entry.counter));
    }
    
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("tutorial", "tutorial", { section });
}

Tutorial::TutorialName Tutorial::stringToTutorialName(const std::string& name) {
    if (name == "Tournament") {
        return TutorialName::Tournament;
    }
    if (name == "EngineSetup") {
        return TutorialName::EngineSetup;
    }
    if (name == "BoardEngines") {
        return TutorialName::BoardEngines;
    }
    if (name == "BoardWindow") {
        return TutorialName::BoardWindow;
    }
    if (name == "BoardCutPaste") {
        return TutorialName::BoardCutPaste;
    }
    if (name == "Epd") {
        return TutorialName::Epd;
    }
    if (name == "Snackbar") {
        return TutorialName::Snackbar;
    }
    return TutorialName::Count;
}

} // namespace QaplaWindows
