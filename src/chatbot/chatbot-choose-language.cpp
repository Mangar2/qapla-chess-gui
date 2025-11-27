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

#include "chatbot-choose-language.h"
#include "chatbot-step-select-option.h"
#include "chatbot-step-finish.h"
#include "configuration.h"
#include "i18n.h"
#include <map>

namespace QaplaWindows::ChatBot {

void ChatbotChooseLanguage::start() {
    std::map<std::string, std::string> langMap = {
        {"English", "eng"},
        {"Deutsch", "deu"},
        {"Français", "fra"}
    };
    std::vector<std::string> languageNames;
    for (const auto& [name, code] : langMap) {
        languageNames.push_back(name);
    }

    steps_.push_back(std::make_unique<ChatbotStepSelectOption>(
        "Please select your preferred language:",
        languageNames,
        [langMap](int index) {
            auto it = langMap.begin();
            std::advance(it, index);
            const auto& [name, code] = *it;
            QaplaConfiguration::Configuration::updateLanguageConfiguration(code);
            Translator::instance().setLanguageCode(code);
        }
    ));

    steps_.push_back(std::make_unique<ChatbotStepFinish>(
        "Thank you! Your language has been set. You can now continue using the application."
    ));

    currentStep_ = 0;
}

bool ChatbotChooseLanguage::draw() {
    bool contentChanged = false;
    
    if (currentStep_ < steps_.size()) {
        static_cast<void>(steps_[currentStep_]->draw());
        if (steps_[currentStep_]->isFinished()) {
            ++currentStep_;
            contentChanged = true;
        }
    }
    
    return contentChanged;
}

bool ChatbotChooseLanguage::isFinished() const {
    return currentStep_ >= steps_.size();
}

std::unique_ptr<ChatbotThread> ChatbotChooseLanguage::clone() const {
    return std::make_unique<ChatbotChooseLanguage>();
}

} // namespace QaplaWindows::ChatBot
