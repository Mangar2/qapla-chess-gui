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

#include "lm-studio-client.h"

#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace QaplaLlm {

/**
 * @brief Appends one successful AI-chat turn per line to finetuning.json, in the OpenAI
 * chat-fine-tuning JSONL convention: one compact `{"messages":[...]}` object per line, using
 * the exact same message shape (see chatMessageToJson()) chatCompletion() itself sends on the
 * wire -- directly usable as a fine-tuning dataset for a tool-calling chat model.
 *
 * Deliberately different from LlmChatLogger: always on (no Settings toggle -- there is nothing
 * to configure), and one single file that grows across the app's entire lifetime rather than a
 * new timestamped file per session, since the whole point is to accumulate a dataset over time.
 * Only turns where every real tool call succeeded are recorded at all -- a connection/timeout
 * failure or a tool reporting failure excludes the whole turn, and hitting the
 * tool-call-iteration limit means there was no final reply to record in the first place (see
 * LlmChatController::runAgentLoop) -- since a fine-tuning example should only ever demonstrate
 * correct tool use. If the model's *final* reply ignored "tool_choice":"required" and came back
 * as plain text instead of a reply_to_user call, the turn is still recorded, but with that
 * reply corrected into the reply_to_user call the model should have made (see
 * makeSyntheticReplyMessage() in llm-chat-controller.cpp) -- so the dataset never reinforces
 * the exact plain-text fallback behavior it exists to train away. The live chat and
 * LlmChatLogger's log are never corrected this way; only this file is.
 *
 * Each record covers exactly one turn: the system prompt, the user's message, and everything
 * the model/tools produced in response (tool calls, their results, and the final reply) --
 * deliberately not the rest of the conversation history, so records stay focused, comparable
 * training examples rather than growing duplicates of earlier turns.
 *
 * ".json" extension despite being JSONL (one object per line, not one top-level array): JSONL
 * is what fine-tuning tooling (OpenAI's own fine-tuning API included) actually expects, and is
 * trivially appendable (O(1), crash-safe) unlike a single JSON array, which would need the
 * whole file read, parsed, and rewritten on every turn.
 *
 * Thread-safe (writes happen from LlmChatController's worker thread); a mutex guards the file.
 */
class LlmFineTuningWriter {
public:
    /**
     * @param logDirectory Directory finetuning.json is created in. Empty (the default) means
     *        the app's own config directory (the same one the .ini file and, if enabled, the
     *        LlmChatLogger's log live in); overridable for tests so they don't write into the
     *        real user config directory.
     */
    explicit LlmFineTuningWriter(std::string logDirectory = "");

    /**
     * @brief Appends one line for a fully successful turn: `{"messages":[...]}`.
     * @param messages The turn's messages in order (system, user, ..., final reply), serialized
     *        via chatMessageToJson(). No-op if empty.
     */
    void appendTurn(const std::vector<ChatMessage>& messages);

private:
    void ensureOpen();

    std::mutex mutex_;
    std::string logDirectory_;
    bool openAttempted_ = false;
    std::ofstream file_;
};

} // namespace QaplaLlm
