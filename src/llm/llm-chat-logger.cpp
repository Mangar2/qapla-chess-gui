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

#include "llm-chat-logger.h"
#include "../os-helpers.h"

#include <base-elements/file-helper.h>
#include <base-elements/string-helper.h>

#include <chrono>

namespace QaplaLlm {

namespace {
    // Total characters of *unstructured* model output logUnstructured() will write between two
    // logLine() calls -- see the class doc comment for why this only guards malformed output.
    constexpr std::size_t MAX_UNSTRUCTURED_BUDGET = 1000;

    std::string timestampPrefix() {
        return "[" + QaplaHelpers::formatTimeOfDay(std::chrono::system_clock::now()) + "] ";
    }
}

LlmChatLogger::LlmChatLogger(bool enabled, std::string logDirectory)
    : enabled_(enabled), logDirectory_(std::move(logDirectory)) {
}

void LlmChatLogger::ensureOpen() {
    if (openAttempted_) {
        return;
    }
    openAttempted_ = true;

    auto directory = logDirectory_.empty() ? QaplaHelpers::OsHelpers::getConfigDirectory() : logDirectory_;
    openedFilePath_ = QaplaHelpers::generateTimestampedFilename("ai-chat", directory, "log");
    file_.open(openedFilePath_, std::ios::app);
}

void LlmChatLogger::writeRaw(const std::string& text) {
    ensureOpen();
    if (file_.is_open()) {
        file_ << text << "\n" << std::flush;
    }
}

void LlmChatLogger::logLine(const std::string& tag, const std::string& text) {
    if (!enabled_) {
        return;
    }
    std::scoped_lock lock(mutex_);
    unstructuredBudgetUsed_ = 0;
    writeRaw(timestampPrefix() + tag + ": " + text);
}

void LlmChatLogger::logUnstructured(const std::string& text) {
    if (!enabled_) {
        return;
    }
    std::scoped_lock lock(mutex_);
    if (unstructuredBudgetUsed_ >= MAX_UNSTRUCTURED_BUDGET) {
        return;
    }

    const std::size_t remaining = MAX_UNSTRUCTURED_BUDGET - unstructuredBudgetUsed_;
    const std::string toWrite = text.substr(0, remaining);
    unstructuredBudgetUsed_ += toWrite.size();
    writeRaw(timestampPrefix() + "UNSTRUCTURED: " + toWrite);
}

void LlmChatLogger::logSystemPromptAndToolsOnce(const std::string& systemPromptText, const std::string& toolsJsonPretty) {
    if (!enabled_) {
        return;
    }
    std::scoped_lock lock(mutex_);
    if (systemPromptLogged_) {
        return;
    }
    systemPromptLogged_ = true;
    // Inlined logLine()'s body (not called directly) since mutex_ is already held here and
    // std::mutex isn't recursive.
    unstructuredBudgetUsed_ = 0;
    writeRaw(timestampPrefix() + "SYSTEM_PROMPT: " + systemPromptText);
    writeRaw(timestampPrefix() + "TOOLS: " + toolsJsonPretty);
}

} // namespace QaplaLlm
