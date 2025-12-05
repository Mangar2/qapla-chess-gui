#include "translation-normalizer.h"

#include <sstream>
#include <algorithm>

namespace {

constexpr char PLACEHOLDER_MARKER = '\\';

/**
 * @brief Escapes existing backslash sequences to avoid conflicts.
 * @param str The string to process.
 * @return The string with backslashes escaped.
 */
[[nodiscard]] std::string escapeBackslashes(const std::string& str) {
    std::string result;
    result.reserve(str.size() * 2);

    for (size_t idx = 0; idx < str.size(); ++idx) {
        if (str[idx] == PLACEHOLDER_MARKER) {
            // Check if followed by a digit - this would conflict with our placeholders
            if (idx + 1 < str.size() && std::isdigit(static_cast<unsigned char>(str[idx + 1])) != 0) {
                result += PLACEHOLDER_MARKER;
                result += PLACEHOLDER_MARKER;
            } else {
                result += str[idx];
            }
        } else {
            result += str[idx];
        }
    }
    return result;
}

/**
 * @brief Unescapes backslash sequences after placeholder restoration.
 * @param str The string to process.
 * @return The string with escaped backslashes restored.
 */
[[nodiscard]] std::string unescapeBackslashes(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (size_t idx = 0; idx < str.size(); ++idx) {
        if (str[idx] == PLACEHOLDER_MARKER && idx + 1 < str.size() && str[idx + 1] == PLACEHOLDER_MARKER) {
            // Double backslash followed by digit -> single backslash + digit
            result += PLACEHOLDER_MARKER;
            ++idx; // Skip the second backslash
        } else {
            result += str[idx];
        }
    }
    return result;
}

/**
 * @brief Extracts prefix (leading whitespace) from the input.
 * @param input The input string.
 * @return Pair of prefix and remaining string.
 */
[[nodiscard]] std::pair<std::string, std::string> extractPrefix(const std::string& input) {
    size_t first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {input, ""};
    }
    return {input.substr(0, first), input.substr(first)};
}

/**
 * @brief Extracts suffix (trailing whitespace, numbers, ImGui ID) from the input.
 * @param input The input string.
 * @return Pair of remaining string and suffix.
 */
[[nodiscard]] std::pair<std::string, std::string> extractSuffix(const std::string& input) {
    // Find the position of ## (ImGui ID) if present
    size_t imguiIdPos = input.find("##");
    
    std::string workStr = input;
    std::string imguiId;
    
    if (imguiIdPos != std::string::npos && imguiIdPos != 0) {
        imguiId = input.substr(imguiIdPos);
        workStr = input.substr(0, imguiIdPos);
    }

    // Find trailing whitespace and numbers
    size_t last = workStr.find_last_not_of(" \t\r\n0123456789");
    
    if (last == std::string::npos) {
        // Entire string is whitespace/numbers - keep it all as suffix
        return {"", workStr + imguiId};
    }

    std::string middle = workStr.substr(0, last + 1);
    std::string trailingSuffix = workStr.substr(last + 1);
    
    return {middle, trailingSuffix + imguiId};
}

} // unnamed namespace

namespace QaplaWindows {

TranslationNormalizer::TranslationNormalizer(const std::string& input) {
    // Step 1: Extract prefix (leading whitespace)
    auto [prefix, afterPrefix] = ::extractPrefix(input);
    prefix_ = prefix;

    if (afterPrefix.empty()) {
        isEmpty_ = true;
        normalizedKey_ = "";
        return;
    }

    // Step 2: Extract suffix (trailing whitespace, numbers, ImGui ID)
    auto [middle, suffix] = ::extractSuffix(afterPrefix);
    suffix_ = suffix;

    if (middle.empty()) {
        isEmpty_ = true;
        normalizedKey_ = "";
        return;
    }

    // Step 3: Escape existing backslash sequences to avoid conflicts
    std::string escaped = ::escapeBackslashes(middle);

    // Step 4: Replace critical elements (numbers, equals signs, etc.) with placeholders
    normalizedKey_ = replaceCriticalElements(escaped);
}

std::string TranslationNormalizer::restorePlaceholders(const std::string& translated) const {
    if (isEmpty_) {
        return prefix_ + suffix_;
    }

    std::string result = translated;

    // Replace placeholders with original values (in reverse order to handle multi-digit correctly)
    for (size_t idx = replacements_.size(); idx > 0; --idx) {
        std::string placeholder = std::string(1, PLACEHOLDER_MARKER) + std::to_string(idx);
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), replacements_[idx - 1]);
            pos += replacements_[idx - 1].length();
        }
    }

    // Unescape backslashes
    result = ::unescapeBackslashes(result);

    // Restore prefix and suffix
    return prefix_ + result + suffix_;
}

std::string TranslationNormalizer::replaceCriticalElements(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    size_t idx = 0;
    while (idx < str.size()) {
        char chr = str[idx];

        // Check for number sequences
        if (std::isdigit(static_cast<unsigned char>(chr)) != 0) {
            size_t numStart = idx;
            while (idx < str.size() && std::isdigit(static_cast<unsigned char>(str[idx])) != 0) {
                ++idx;
            }
            std::string numStr = str.substr(numStart, idx - numStart);
            result += addReplacement(numStr);
            continue;
        }

        // Check for equals sign
        if (chr == '=') {
            result += addReplacement("=");
            ++idx;
            continue;
        }

        // Regular character - just copy it
        result += chr;
        ++idx;
    }

    return result;
}

std::string TranslationNormalizer::addReplacement(const std::string& value) {
    replacements_.push_back(value);
    return std::string(1, PLACEHOLDER_MARKER) + std::to_string(replacements_.size());
}

} // namespace QaplaWindows
