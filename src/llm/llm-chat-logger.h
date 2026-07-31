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

#pragma once

#include <fstream>
#include <mutex>
#include <string>

namespace QaplaLlm {

/**
 * @brief Appends one AI-chat session's full traffic to a timestamped log file, for diagnosing
 * what a model actually said/did after the fact.
 *
 * One instance per LlmChatController (i.e. per AI-chat session); the file is opened lazily, on
 * the first actually-logged line, in the same directory as the app's own .ini file, named via
 * QaplaHelpers::generateTimestampedFilename("ai-chat", ..., "log") -- the same helper/scheme
 * qapla-engine-tester's own logs use, so a chat that's opened and immediately closed without
 * saying anything leaves no empty file behind.
 *
 * Deliberately caps how much *unstructured* model output (see logUnstructured()) it will log in
 * a row: a model stuck looping malformed/free-text output could otherwise flood the file
 * unboundedly. logLine() -- used for everything the log considers well-formed, including a tool
 * call the tool itself then rejects as invalid -- always writes in full and resets that cap, so
 * the budget only ever guards against genuinely malformed output, never against a model that's
 * behaving (even if the GUI tools it calls disagree with its arguments).
 *
 * Thread-safe: LlmChatController writes from both the UI thread (the user's own messages) and
 * its worker thread (everything the model does), and a mutex guards every write.
 */
class LlmChatLogger {
public:
    /**
     * @param enabled If false, every call is a no-op and no file is ever created (see the
     *        "Log AI chat traffic to file" Settings checkbox).
     * @param logDirectory Directory the log file is created in. Empty (the default) means the
     *        app's own config directory (QaplaHelpers::OsHelpers::getConfigDirectory(), the same
     *        directory the .ini file lives in); overridable for tests so they don't write into
     *        the real user config directory.
     */
    explicit LlmChatLogger(bool enabled, std::string logDirectory = "");

    /**
     * @brief Logs one line for a well-formed event: a user message, a tool call and its result
     * (success or failure -- a tool rejecting its arguments is still well-formed output), the
     * final reply text, a tool-call-loop's connection/protocol-level error, or similar.
     *
     * Always written in full (never truncated) and resets the unstructured-output budget (see
     * logUnstructured()).
     * @param tag Short category label, e.g. "USER", "TOOL_CALL", "TOOL_RESULT", "REPLY", "ERROR".
     */
    void logLine(const std::string& tag, const std::string& text);

    /**
     * @brief Logs free-text model output that arrived instead of the required structured
     * tool-call array (see LlmChatController's "tool_choice":"required" and its
     * empty-toolCalls fallback) -- i.e. output the model was never supposed to produce.
     *
     * Capped at 1000 characters total since the last logLine() call, not per call: once the cap
     * is reached, further text is silently dropped (not even truncated) until the next
     * logLine() resets it, so a model stuck looping garbage can't flood the file.
     */
    void logUnstructured(const std::string& text);

    /** @brief The path of the log file once opened, for tests/diagnostics; empty until then. */
    [[nodiscard]] const std::string& openedFilePath() const {
        return openedFilePath_;
    }

private:
    void ensureOpen();
    void writeRaw(const std::string& text);

    std::mutex mutex_;
    bool enabled_;
    std::string logDirectory_;
    bool openAttempted_ = false;
    std::ofstream file_;
    std::string openedFilePath_;
    std::size_t unstructuredBudgetUsed_ = 0;
};

} // namespace QaplaLlm
