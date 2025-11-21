#include "i18n.h"

#include <string-helper.h>
#include <logger.h>
#include <ini-file.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <ranges>
#include <filesystem>

using QaplaTester::Logger;
using QaplaTester::TraceLevel;

Translator& Translator::instance() {
    static Translator instance;
    return instance;
}

Translator::Translator() 
    : QaplaHelpers::Autosavable("missing_translations.txt", ".bak", 60000, []() { return Autosavable::getConfigDirectory(); }) 
{
    loadFile();
}

Translator::~Translator() {
    saveFile();
}

std::string Translator::translate(const std::string& topic, const std::string& key) {
    // Detect leading/trailing whitespace to preserve it
    size_t first = key.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return key;
    }
    size_t last = key.find_last_not_of(" \t\r\n0123456789");
    
    std::string prefix = key.substr(0, first);
    std::string suffix = key.substr(last + 1);
    std::string trimmedKey = key.substr(first, last - first + 1);

    std::scoped_lock lock(languageMutex);
    
    auto topicIt = translations.find(topic);
    if (topicIt != translations.end()) {
        auto keyIt = topicIt->second.find(trimmedKey);
        if (keyIt != topicIt->second.end()) {
            return prefix + keyIt->second + suffix;
        }
    }

    QaplaHelpers::IniFile::Section section;
    section.name = "Translation";
    section.addEntry("id", topic);
    section.addEntry(toFileFormat(trimmedKey), "");
    missingKeys_.addSection(section);
    setModified();
    
    return key;
}

void Translator::loadLanguageFile(const std::string& filepath) {
    std::scoped_lock lock(languageMutex);
    std::ifstream file(filepath);
    if (!file.is_open()) {
        QaplaTester::Logger::reportLogger().log(std::string("Error opening language - file: ") + filepath, 
            TraceLevel::error);
        return;
    }

    auto sectionList = QaplaHelpers::IniFile::load(file);
    
    for (const auto& section : sectionList) {
        if (section.name != "Translation") {
            continue;
        }
        
        auto topicOpt = section.getValue("id");
        if (!topicOpt.has_value()) {
            continue;
        }
        
        const auto& topic = topicOpt.value();
        auto& topicTranslations = translations[topic];
        
        for (const auto& [key, value] : section.entries) {
            if (key == "id") {
                continue;
            }
            topicTranslations[fromFileFormat(key)] = fromFileFormat(value);
        }
    }
}

void Translator::addTranslation(const std::string& topic, const std::string& key, const std::string& value) {
    std::scoped_lock lock(languageMutex);
    translations[topic][key] = value;
}

void Translator::setLanguageDirectory(const std::string& directory) {
    std::scoped_lock lock(languageMutex);
    languageDirectory = directory;
}

void Translator::setLanguageCode(const std::string& language) {
    {
        std::scoped_lock lock(languageMutex);
        if (currentLanguage == language && std::ranges::find(loadedLanguages, language) != loadedLanguages.end()) {
            return; 
        }
        currentLanguage = language;
    }

    std::filesystem::path dir = getDirectory();
    std::filesystem::path langPath = dir / "i18n" / (language + ".lang");
    
    if (std::filesystem::exists(langPath)) {
        loadLanguageFile(langPath.string());
        std::scoped_lock lock(languageMutex);
        if (std::ranges::find(loadedLanguages, language) == loadedLanguages.end()) {
            loadedLanguages.push_back(language);
        }
        QaplaTester::Logger::reportLogger().log(std::string("Loaded language file: ") + langPath.string(), 
            TraceLevel::info);
    } else {
        QaplaTester::Logger::reportLogger().log(std::string("Language file not found: ") + langPath.string(), 
            TraceLevel::warning);
    }
}

std::string Translator::getLanguageCode() const {
    std::scoped_lock lock(languageMutex);
    return currentLanguage;
}

void Translator::saveData(std::ofstream& out) {
    std::scoped_lock lock(languageMutex);
    missingKeys_.save(out);
}

void Translator::loadData(std::ifstream& in) {
    std::scoped_lock lock(languageMutex);
    missingKeys_.load(in);
}

std::string Translator::toFileFormat(const std::string& text) {
    std::string result = text;
    size_t pos = 0;
    while ((pos = result.find('\n', pos)) != std::string::npos) {
        result.replace(pos, 1, "\\n");
        pos += 2;
    }
    return result;
}

std::string Translator::fromFileFormat(const std::string& text) {
    std::string result = QaplaHelpers::trim(text);
    size_t pos = 0;
    while ((pos = result.find("\\n", pos)) != std::string::npos) {
        result.replace(pos, 2, "\n");
        pos += 1;
    }
    return result;
}
