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
 * @author Volker Böhm
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include <catch2/catch_test_macros.hpp>

#include "llm/llm-chat-logger.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace QaplaLlm;

namespace {
    // A real (system temp, not the user's actual config directory) directory dedicated to this
    // test -- LlmChatLogger's directory is injectable exactly so tests never touch the real
    // ~/.qapla-chess-gui.
    std::filesystem::path testLogDirectory() {
        return std::filesystem::temp_directory_path() / "qapla-llm-chat-logger-test";
    }

    std::string readFile(const std::string& path) {
        std::ifstream in(path);
        std::ostringstream out;
        out << in.rdbuf();
        return out.str();
    }

    // Total characters logged with the "UNSTRUCTURED: " tag, ignoring the timestamp/tag prefix
    // and any other (unrelated) line -- i.e. what logUnstructured() actually got to write.
    std::size_t unstructuredCharsIn(const std::string& fileContent) {
        constexpr std::string_view marker = "UNSTRUCTURED: ";
        std::size_t total = 0;
        std::size_t pos = 0;
        while ((pos = fileContent.find(marker, pos)) != std::string::npos) {
            pos += marker.size();
            std::size_t lineEnd = fileContent.find('\n', pos);
            if (lineEnd == std::string::npos) {
                lineEnd = fileContent.size();
            }
            total += lineEnd - pos;
            pos = lineEnd;
        }
        return total;
    }
}

TEST_CASE("LlmChatLogger caps cumulative unstructured output at 1000 chars until the next logLine",
    "[llm][llm-chat-logger]") {
    std::filesystem::remove_all(testLogDirectory());
    std::filesystem::create_directories(testLogDirectory());

    LlmChatLogger logger(/*enabled=*/true, testLogDirectory().string());

    logger.logUnstructured(std::string(600, 'A'));
    logger.logUnstructured(std::string(600, 'B')); // 1200 requested total, cap is 1000
    REQUIRE_FALSE(logger.openedFilePath().empty());

    auto content = readFile(logger.openedFilePath());
    REQUIRE(unstructuredCharsIn(content) == 1000);

    // Further unstructured output is dropped entirely (not truncated-and-appended) once
    // the budget is exhausted.
    logger.logUnstructured(std::string(50, 'X'));
    content = readFile(logger.openedFilePath());
    REQUIRE(unstructuredCharsIn(content) == 1000);

    // A well-formed event resets the budget for the next batch of unstructured output.
    logger.logLine("USER", "hello");
    logger.logUnstructured(std::string(500, 'C'));
    content = readFile(logger.openedFilePath());
    REQUIRE(unstructuredCharsIn(content) == 1500);
    REQUIRE(content.find("USER: hello") != std::string::npos);

    std::filesystem::remove_all(testLogDirectory());
}

TEST_CASE("LlmChatLogger writes nothing and creates no file when disabled", "[llm][llm-chat-logger]") {
    std::filesystem::remove_all(testLogDirectory());
    std::filesystem::create_directories(testLogDirectory());

    LlmChatLogger logger(/*enabled=*/false, testLogDirectory().string());
    logger.logLine("USER", "hello");
    logger.logUnstructured("garbage");

    REQUIRE(logger.openedFilePath().empty());
    REQUIRE(std::filesystem::is_empty(testLogDirectory()));

    std::filesystem::remove_all(testLogDirectory());
}

TEST_CASE("LlmChatLogger logLine writes the tag and full text, unbounded", "[llm][llm-chat-logger]") {
    std::filesystem::remove_all(testLogDirectory());
    std::filesystem::create_directories(testLogDirectory());

    LlmChatLogger logger(/*enabled=*/true, testLogDirectory().string());
    std::string longText(2000, 'Z'); // well past the 1000-char unstructured cap -- must NOT apply here
    logger.logLine("TOOL_RESULT", longText);

    auto content = readFile(logger.openedFilePath());
    REQUIRE(content.find("TOOL_RESULT: " + longText) != std::string::npos);

    std::filesystem::remove_all(testLogDirectory());
}

TEST_CASE("LlmChatLogger logSystemPromptAndToolsOnce writes only on its first call", "[llm][llm-chat-logger]") {
    std::filesystem::remove_all(testLogDirectory());
    std::filesystem::create_directories(testLogDirectory());

    LlmChatLogger logger(/*enabled=*/true, testLogDirectory().string());
    logger.logSystemPromptAndToolsOnce("be helpful", "{\n  \"a\": 1\n}");
    logger.logSystemPromptAndToolsOnce("a different prompt", "{\n  \"b\": 2\n}");

    auto content = readFile(logger.openedFilePath());
    REQUIRE(content.find("SYSTEM_PROMPT: be helpful") != std::string::npos);
    REQUIRE(content.find("TOOLS: {\n  \"a\": 1\n}") != std::string::npos);
    // The second call must be a no-op -- only the first prompt/tools pair is ever logged.
    REQUIRE(content.find("a different prompt") == std::string::npos);
    REQUIRE(content.find("\"b\": 2") == std::string::npos);

    std::filesystem::remove_all(testLogDirectory());
}
