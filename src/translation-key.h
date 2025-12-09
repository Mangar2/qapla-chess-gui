#pragma once

#include <string>
#include <cstdint>

namespace QaplaWindows {

/**
 * @brief Represents a translation key with optional hash for long strings.
 * 
 * For short strings (< 80 chars), the string itself is used as the key.
 * For long strings, a CRC32 hash is computed and the string is truncated.
 */
class TranslationKey {
public:
    /**
     * @brief Creates a translation key from a normalized string.
     * @param normalizedString The normalized translation key.
     */
    explicit TranslationKey(const std::string& normalizedString);

    /**
     * @brief Gets the key string for file storage and lookup.
     * Format: {s:"text",t:"YYYY-MM-DD"} or {h:"hash",s:"truncated",t:"YYYY-MM-DD"}
     * @param timestamp The last seen timestamp (YYYY-MM-DD format).
     * @return The formatted key string.
     */
    [[nodiscard]] std::string getKeyString(const std::string& timestamp) const;

    /**
     * @brief Gets the lookup key for finding translations.
     * This is either the original string or hash-based key.
     * @return The lookup key.
     */
    [[nodiscard]] std::string getLookupKey() const;

    /**
     * @brief Checks if this is a long string (uses hash).
     * @return True if hash is used.
     */
    [[nodiscard]] bool usesHash() const { return usesHash_; }

    /**
     * @brief Parses a key string from a .lang file.
     * Supports both old format ("text") and new format ({s:"text",t:"..."}).
     * @param keyString The key string to parse.
     * @return Pair of (lookup key, timestamp). Timestamp is empty for old format.
     */
    [[nodiscard]] static std::pair<std::string, std::string> parseKeyString(const std::string& keyString);

    /**
     * @brief Computes CRC32 hash of a string.
     * @param str The string to hash.
     * @return The CRC32 hash as hex string (8 characters).
     */
    [[nodiscard]] static std::string computeHash(const std::string& str);

    /**
     * @brief Gets current date in YYYY-MM-DD format.
     * @return The current date string.
     */
    [[nodiscard]] static std::string getCurrentDate();

private:
    static constexpr size_t MAX_STRING_LENGTH = 80;
    static constexpr size_t TRUNCATE_LENGTH = 50;

    std::string originalString_;
    std::string hash_;
    bool usesHash_;
};

} // namespace QaplaWindows
