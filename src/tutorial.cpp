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
#include "engine-setup-window.h"
#include "engine-window.h"
#include "qapla-tester/string-helper.h"

bool Tutorial::isCompleted(Topic topic) const {
    switch (topic) {
        case Topic::EngineWindow:
            return QaplaWindows::EngineWindow::getTutorialCounter() >= getCompletionThreshold(topic);
        default:
            return false;
    }
}

void Tutorial::restartTopic(Topic topic) {
    switch (topic) {
        case Topic::EngineWindow:
            QaplaWindows::EngineWindow::resetTutorialCounter();
            break;
        default:
            break;
    }
    saveConfiguration();
}

std::string Tutorial::getTopicName(Topic topic) {
    switch (topic) {
        case Topic::EngineWindow:
            return "Engine Window";
        default:
            return "Unknown";
    }
}

uint32_t Tutorial::getCompletionThreshold(Topic topic) {
    switch (topic) {
        case Topic::EngineWindow:
            return 3;
        default:
            return 1;
    }
}

void Tutorial::showNextTutorialStep(const std::string& topicName) {
    for (auto& entry : entries_) {
        if (entry.name == topicName) {
            if (entry.completed()) {
                return;
            }
            if (!entry.dependsOn.empty()) {
                auto it = std::ranges::find_if(entries_,
                    [&entry](const Entry& e) { return e.name == entry.dependsOn; });
                if (it != entries_.end() && !it->completed()) {
                    return;
                }
            }
            entry.getProgressCounter()++;
            entry.showNextMessage();
            break;
        }
    }
    saveConfiguration();
}

 void Tutorial::finishTutorial(const std::string& topicName) {
    for (auto& entry : entries_) {
        if (entry.name == topicName) {
            entry.finish();
            break;
        }
    }
    saveConfiguration();
}

void Tutorial::loadConfiguration() {
    auto sections = QaplaConfiguration::Configuration::instance().
        getConfigData().getSectionList("tutorial", "tutorial").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!sections.empty()) {
        const auto& section = sections[0];
        QaplaWindows::EngineWindow::setTutorialCounter(
            QaplaHelpers::to_uint32(section.getValue("enginewindow").value_or("0")).value_or(0));
        for (auto& entry: entries_) {
            auto valueOpt = sections[0].getValue(entry.name).value_or("0");
            entry.counter = QaplaHelpers::to_uint32(valueOpt).value_or(0);
        }
    }
}

void Tutorial::saveConfiguration() const {
    QaplaHelpers::IniFile::Section section {
        .name = "tutorial",
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", "tutorial"},
            {"enginewindow", std::to_string(QaplaWindows::EngineWindow::getTutorialCounter())}
        }
    };
    for (const auto& entry: entries_) {
        section.entries.push_back({ entry.name, std::to_string(entry.counter) });
    }
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("tutorial", "tutorial", { section });
}
