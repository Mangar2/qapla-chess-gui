#pragma once

#include <string>
#include <vector>

namespace QaplaWindows {

/**
 * @brief Normalizes strings for translation by extracting variable parts.
 * 
 * This class handles the extraction of "critical" elements from strings that would
 * otherwise cause issues with the translation system:
 * - Numbers (which would create many redundant translation entries)
 * - Equals signs (which conflict with the key=value format in .lang files)
 * - Leading/trailing whitespace
 * - ImGui IDs (##identifier)
 * - Trailing numbers (for backward compatibility)
 * 
 * The extracted parts are replaced with placeholders (\1, \2, etc.) and can be
 * restored after translation using restorePlaceholders().
 */
class TranslationNormalizer {
public:
    /**
     * @brief Constructs a normalizer and processes the input string.
     * @param input The original string to normalize.
     */
    explicit TranslationNormalizer(const std::string& input);

    /**
     * @brief Gets the normalized string with placeholders.
     * @return The string with critical elements replaced by placeholders.
     */
    [[nodiscard]] std::string getNormalizedKey() const { return normalizedKey_; }

    /**
     * @brief Restores the placeholders in a translated string.
     * @param translated The translated string containing placeholders.
     * @return The final string with placeholders replaced by original values.
     */
    [[nodiscard]] std::string restorePlaceholders(const std::string& translated) const;

    /**
     * @brief Gets the leading whitespace/prefix that was stripped.
     * @return The prefix string.
     */
    [[nodiscard]] std::string getPrefix() const { return prefix_; }

    /**
     * @brief Gets the trailing part (whitespace, numbers, ImGui ID) that was stripped.
     * @return The suffix string.
     */
    [[nodiscard]] std::string getSuffix() const { return suffix_; }

    /**
     * @brief Checks if the input was empty or whitespace-only.
     * @return True if no translatable content exists.
     */
    [[nodiscard]] bool isEmpty() const { return isEmpty_; }

private:
    /**
     * @brief Replaces critical elements with placeholders.
     * @param str The string to process.
     * @return The string with placeholders.
     */
    [[nodiscard]] std::string replaceCriticalElements(const std::string& str);

    /**
     * @brief Adds a replacement and returns the placeholder string.
     * @param value The original value being replaced.
     * @return The placeholder string (e.g., "\1").
     */
    [[nodiscard]] std::string addReplacement(const std::string& value);

    std::string prefix_;
    std::string suffix_;
    std::string normalizedKey_;
    std::vector<std::string> replacements_;
    bool isEmpty_ = false;

};

} // namespace QaplaWindows
