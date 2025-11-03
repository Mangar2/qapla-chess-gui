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
#include "qapla-tester/string-helper.h"

#include <optional>

bool Tutorial::allPrecedingCompleted(TutorialName name) const {
    const size_t currentIndex = toIndex(name);
    
    // Alle Tutorials vor dem aktuellen müssen beendet sein
    for (size_t i = 0; i < currentIndex; ++i) {
        if (!entries_[i].completed()) {
            return false;
        }
    }
    return true;
}

bool Tutorial::mayStart(TutorialName name) const {
    const auto& entry = entries_[toIndex(name)];
    
    // Kann nur starten, wenn autoStart gesetzt ist und noch nicht begonnen wurde
    if (!entry.autoStart || entry.getProgressCounter() != 0) {
        return false;
    }
    
    // Alle vorhergehenden Tutorials müssen abgeschlossen sein
    return allPrecedingCompleted(name);
}

void Tutorial::showNextTutorialStep(TutorialName name) {
    auto& entry = entries_[toIndex(name)];
    
    if (entry.completed()) {
        return;
    }
    
    // Prüfe, ob alle vorhergehenden Tutorials beendet sind
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
    
    // Prüfe, ob nachfolgende auto-start Tutorials nun starten können
    for (size_t i = 0; i < static_cast<size_t>(TutorialName::Count); ++i) {
        auto tutorialName = static_cast<TutorialName>(i);
        if (mayStart(tutorialName)) {
            auto& autoStartEntry = entries_[i];
            autoStartEntry.getProgressCounter() = 1;
            autoStartEntry.showNextMessage();
        }
    }
    
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

void Tutorial::loadConfiguration() {
    auto sections = QaplaConfiguration::Configuration::instance().
        getConfigData().getSectionList("tutorial", "tutorial").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!sections.empty()) {
        const auto& section = sections[0];
        
        // Lade alle Tutorial-Counter aus der Konfiguration
        for (size_t i = 0; i < static_cast<size_t>(TutorialName::Count); ++i) {
            auto name = static_cast<TutorialName>(i);
            auto& entry = entries_[i];
            
            // Nutze displayName + Index als Config-Key für Uniqueness
            auto configName = entry.displayName + std::to_string(i);
            
            auto valueOpt = section.getValue(configName).value_or("0");
            entry.counter = QaplaHelpers::to_uint32(valueOpt).value_or(0);
            entry.getProgressCounter() = entry.counter;
        }
    }
    
    // Prüfe, ob auto-start Tutorials nun starten können
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
    
    // Speichere alle Tutorial-Counter
    for (size_t i = 0; i < static_cast<size_t>(TutorialName::Count); ++i) {
        const auto& entry = entries_[i];
        
        // Nutze displayName + Index als Config-Key für Uniqueness
        auto configName = entry.displayName + std::to_string(i);
        
        section.entries.push_back({ configName, std::to_string(entry.counter) });
    }
    
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("tutorial", "tutorial", { section });
}
