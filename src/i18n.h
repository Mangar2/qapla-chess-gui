#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

#include <ini-file.h>

#include "autosavable.h"

class Translator : public QaplaHelpers::Autosavable {
public:
    /**
     * @brief Gets the singleton instance of the Translator.
     * @return The singleton instance.
     */
    [[nodiscard]] static Translator& instance();

    /**
     * @brief Translates a key with a topic. Returns the key itself if no translation is found.
     * @param topic The topic category (e.g., "Button", "Tab").
     * @param key The key to translate.
     * @return The translated string or the key if not found.
     */
    [[nodiscard]] std::string translate(const std::string& topic, const std::string& key);

    /**
     * @brief Loads translations from an INI file.
     * @param filepath The path to the language file.
     */
    void loadLanguageFile(const std::string& filepath);

    /**
     * @brief Adds a translation programmatically.
     * @param topic The topic category.
     * @param key The key to translate.
     * @param value The translated value.
     */
    void addTranslation(const std::string& topic, const std::string& key, const std::string& value);

    /**
     * @brief Sets the directory where language files are located relative to the main data directory.
     * @param directory The directory path (default: "lang").
     */
    void setLanguageDirectory(const std::string& directory);

    /**
     * @brief Sets the active language.
     * Clears current translations and attempts to load the new language file.
     * @param language The language code (e.g. "de").
     */
    void setLanguageCode(const std::string& language);

    /**
     * @brief Gets the currently active language.
     * @return The language code.
     */
    [[nodiscard]] std::string getLanguageCode() const;

    /**
     * @brief Converts text to file format (escapes newlines).
     * @param text The text to escape.
     * @return The escaped text.
     */
    [[nodiscard]] static std::string toFileFormat(const std::string& text);

    /**
     * @brief Converts text from file format (trims and unescapes newlines).
     * @param text The text to unescape.
     * @return The unescaped and trimmed text.
     */
    [[nodiscard]] static std::string fromFileFormat(const std::string& text);

protected:
    void saveData(std::ofstream& out) override;
    void loadData(std::ifstream& in) override;

private:
    Translator();
    ~Translator();
    Translator(const Translator&) = delete;
    Translator& operator=(const Translator&) = delete;

    using TranslationMap = std::unordered_map<std::string, std::string>;
    using TopicMap = std::unordered_map<std::string, TranslationMap>;
    
    TopicMap translations;
    mutable QaplaHelpers::ConfigData missingKeys_;
    mutable std::mutex languageMutex;
    std::string languageDirectory = "lang";
    std::string currentLanguage = "eng";
    std::vector<std::string> loadedLanguages;
};

/**
 * @brief Global helper function for easy access to translation.
 * @param topic The topic category.
 * @param text The text key to translate.
 * @return The translated text.
 */
[[nodiscard]] inline std::string tr(const std::string& topic, const std::string& text) {
    return Translator::instance().translate(topic, text);
}
