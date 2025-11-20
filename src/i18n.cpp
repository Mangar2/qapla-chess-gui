#include "i18n.h"

#include <string-helper.h>
#include <logger.h>

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

std::string Translator::translate(const std::string& key) {
    // Detect leading/trailing whitespace to preserve it
    size_t first = key.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return key;
    }
    size_t last = key.find_last_not_of(" \t\r\n0123456789");
    
    std::string prefix = key.substr(0, first);
    std::string suffix = key.substr(last + 1);
    std::string trimmedKey = key.substr(first, last - first + 1);

    std::lock_guard<std::mutex> lock(languageMutex);
    if (auto it = translations.find(trimmedKey); it != translations.end()) {
        return prefix + it->second + suffix;
    }

    if (std::ranges::find(missingKeys, trimmedKey) == missingKeys.end()) {
        missingKeys.push_back(trimmedKey);
        setModified();
    }
    
    return key;
}

void Translator::loadLanguageFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(languageMutex);
    std::ifstream file(filepath);
    if (!file.is_open()) {
        QaplaTester::Logger::reportLogger().log(std::string("Error opening language - file: ") + filepath, 
            TraceLevel::error);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);
            
            translations[fromFileFormat(key)] = fromFileFormat(value);
        }
    }
}

void Translator::addTranslation(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(languageMutex);
    translations[key] = value;
}

void Translator::setLanguageDirectory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(languageMutex);
    languageDirectory = directory;
}

void Translator::setLanguageCode(const std::string& language) {
    {
        std::lock_guard<std::mutex> lock(languageMutex);
        if (currentLanguage == language && std::ranges::find(loadedLanguages, language) != loadedLanguages.end()) {
            return; 
        }
        currentLanguage = language;
    }

    std::filesystem::path dir = getDirectory();
    std::filesystem::path langPath = dir / "i18n" / (language + ".lang");
    
    if (std::filesystem::exists(langPath)) {
        loadLanguageFile(langPath.string());
        std::lock_guard<std::mutex> lock(languageMutex);
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
    std::lock_guard<std::mutex> lock(languageMutex);
    return currentLanguage;
}

std::vector<std::string> Translator::getMissingTranslations() const {
    std::lock_guard<std::mutex> lock(languageMutex);
    return missingKeys;
}

void Translator::saveData(std::ofstream& out) {
    std::lock_guard<std::mutex> lock(languageMutex);
    for (const auto& key : missingKeys) {
        if (translations.find(key) == translations.end()) {
            out << toFileFormat(key) << "=" << std::endl;
        }
    }
}

void Translator::loadData(std::ifstream& in) {
    std::lock_guard<std::mutex> lock(languageMutex);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        size_t pos = line.find('=');
        std::string key = (pos != std::string::npos) ? line.substr(0, pos) : line;
        std::string processedKey = fromFileFormat(key);
        
        if (!processedKey.empty() && std::ranges::find(missingKeys, processedKey) == missingKeys.end()) {
             missingKeys.push_back(processedKey);
        }
    }
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
