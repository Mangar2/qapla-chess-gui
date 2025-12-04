#include "i18n.h"
#include "callback-manager.h"

#include <string-helper.h>
#include <logger.h>
#include <ini-file.h>

#ifndef QAPLA_DEBUG_I18N
#include <deu-lang.h>
#include <eng-lang.h>
#include <fra-lang.h>
#endif

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <ranges>
#include <filesystem>

using QaplaTester::Logger;
using QaplaTester::TraceLevel;

namespace QaplaWindows {

Translator& Translator::instance() {
    static Translator instance;
    return instance;
}

Translator::Translator() 
    : QaplaHelpers::Autosavable("missing_translations.txt", ".bak", 60000, []() { 
        return Autosavable::getConfigDirectory(); 
    }) 
{
    loadFile();
    saveCallbackHandle_ = StaticCallbacks::save().registerCallback([this]() {
        this->saveFile();
    });
}

Translator::~Translator() {
}

std::string Translator::translate(const std::string& topic, const std::string& key) {
    // Detect leading/trailing whitespace to preserve it
    size_t first = key.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return key;
    }
    // Remove ## and everything after it for calculating last
    size_t last = key.find_last_not_of(" \t\r\n0123456789");
    size_t imguiIdPos = key.find("##");
    if (imguiIdPos != std::string::npos && imguiIdPos != 0) {
        last = std::min(last, imguiIdPos - 1);
    }
    
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

#ifdef QAPLA_DEBUG_I18N
    // In debug mode: Add missing translations directly to all language files
    auto trimmedKeyFileFormat = toFileFormat(trimmedKey);
    auto i18nDir = getI18nSourceDirectory();
    
    for (const auto& lang : {"deu", "eng", "fra"}) {
        auto langFile = i18nDir / (std::string(lang) + ".lang");
        addMissingTranslationToFile(langFile, topic, trimmedKeyFileFormat, trimmedKeyFileFormat);
    }
    
    // Also add to current translations so we don't write it again
    translations[topic][trimmedKey] = trimmedKey;
    
    QaplaTester::Logger::reportLogger().log(
        std::string("Added missing translation [") + topic + "] " + trimmedKey, 
        TraceLevel::info);
#else
    // In release mode: Log missing keys to separate file
    auto trimmedKeyFileFormat = toFileFormat(trimmedKey);
    auto sectionListOpt = missingKeys_.getSectionList("Translation", topic);
    if (!sectionListOpt || sectionListOpt->empty()) {
        missingKeys_.addSection({
            .name = "Translation",
            .entries = { {"id", topic} }
        });
        setModified();
        return key;
    }
    // There is only one section per name/id in this use case
    auto& section = sectionListOpt->front();
    if (!section.getValue(trimmedKeyFileFormat)) {
        section.addEntry(trimmedKeyFileFormat, "");
        missingKeys_.setSectionList("Translation", topic, *sectionListOpt);
    }
    setModified();
#endif
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
    loadLanguageFromStream(file);
}

void Translator::loadLanguageFromStream(std::istream& stream) {
    auto sectionList = QaplaHelpers::IniFile::load(stream);
    
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
        translations.clear();
        loadedLanguages.clear();
    }

#ifdef QAPLA_DEBUG_I18N
    // In debug mode: Always load from source .lang files
    auto i18nDir = getI18nSourceDirectory();
    std::filesystem::path langPath = i18nDir / (language + ".lang");
    
    if (std::filesystem::exists(langPath)) {
        loadLanguageFile(langPath.string());
        std::scoped_lock lock(languageMutex);
        if (std::ranges::find(loadedLanguages, language) == loadedLanguages.end()) {
            loadedLanguages.push_back(language);
        }
        QaplaTester::Logger::reportLogger().log(
            std::string("Debug mode: Loaded language file from source: ") + langPath.string(), 
            TraceLevel::info);
    } else {
        QaplaTester::Logger::reportLogger().log(
            std::string("Debug mode: Language file not found: ") + langPath.string(), 
            TraceLevel::warning);
    }
#else
    // Release mode: Try loading from embedded data first
    const uint32_t* data = nullptr;
    uint32_t size = 0;

    if (language == "deu") {
        data = deu_lang;
        size = deu_langSize;
    } else if (language == "eng") {
        data = eng_lang;
        size = eng_langSize;
    } else if (language == "fra") {
        data = fra_lang;
        size = fra_langSize;
    }

    if (data) {
        std::string content(reinterpret_cast<const char*>(data), size);
        std::stringstream ss(content);
        
        std::scoped_lock lock(languageMutex);
        loadLanguageFromStream(ss);
        
        if (std::ranges::find(loadedLanguages, language) == loadedLanguages.end()) {
            loadedLanguages.push_back(language);
        }
        QaplaTester::Logger::reportLogger().log(std::string("Loaded embedded language: ") + language, 
            TraceLevel::info);
        return;
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
#endif
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

#ifdef QAPLA_DEBUG_I18N
std::filesystem::path Translator::getI18nSourceDirectory() const {
    // Get the executable path and navigate to the source i18n directory
    // Assuming the build directory is under the project root (e.g., build/default/)
    std::filesystem::path exePath = std::filesystem::current_path();
    
    // Try different possible locations relative to where the exe might run from
    std::vector<std::filesystem::path> possiblePaths = {
        exePath / "i18n",                           // Running from project root
        exePath / ".." / ".." / "i18n",             // Running from build/default/
        exePath / ".." / "i18n",                    // Running from build/
        exePath.parent_path() / ".." / ".." / "i18n" // Exe in build/default/
    };
    
    for (const auto& path : possiblePaths) {
        auto normalizedPath = std::filesystem::weakly_canonical(path);
        if (std::filesystem::exists(normalizedPath) && 
            std::filesystem::exists(normalizedPath / "eng.lang")) {
            return normalizedPath;
        }
    }
    
    // Fallback: try to find i18n relative to current directory
    QaplaTester::Logger::reportLogger().log(
        "Warning: Could not find i18n source directory, using current path", 
        TraceLevel::warning);
    return exePath / "i18n";
}

void Translator::addMissingTranslationToFile(const std::filesystem::path& filepath, 
                                             const std::string& topic, 
                                             const std::string& key, 
                                             const std::string& value) {
    if (!std::filesystem::exists(filepath)) {
        QaplaTester::Logger::reportLogger().log(
            std::string("Cannot add translation - file not found: ") + filepath.string(), 
            TraceLevel::warning);
        return;
    }

    // Load the existing file
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        return;
    }
    
    auto sectionList = QaplaHelpers::IniFile::load(inFile);
    inFile.close();
    
    // Find or create the section for this topic
    bool foundSection = false;
    for (auto& section : sectionList) {
        if (section.name != "Translation") {
            continue;
        }
        auto topicOpt = section.getValue("id");
        if (!topicOpt.has_value() || topicOpt.value() != topic) {
            continue;
        }
        
        // Check if key already exists
        if (section.getValue(key).has_value()) {
            return; // Key already exists, nothing to do
        }
        
        // Add the new key=value
        section.addEntry(key, value);
        foundSection = true;
        break;
    }
    
    // If section doesn't exist, create it
    if (!foundSection) {
        QaplaHelpers::IniFile::Section newSection;
        newSection.name = "Translation";
        newSection.addEntry("id", topic);
        newSection.addEntry(key, value);
        sectionList.push_back(newSection);
    }
    
    // Write back to file
    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        QaplaTester::Logger::reportLogger().log(
            std::string("Cannot write to language file: ") + filepath.string(), 
            TraceLevel::error);
        return;
    }
    
    QaplaHelpers::IniFile::saveSections(outFile, sectionList);
}
#endif

} // namespace QaplaWindows