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

#include "llm-finetuning-writer.h"
#include "../os-helpers.h"

#include <filesystem>

namespace QaplaLlm {

LlmFineTuningWriter::LlmFineTuningWriter(std::string logDirectory)
    : logDirectory_(std::move(logDirectory)) {
}

void LlmFineTuningWriter::ensureOpen() {
    if (openAttempted_) {
        return;
    }
    openAttempted_ = true;

    auto directory = logDirectory_.empty() ? QaplaHelpers::OsHelpers::getConfigDirectory() : logDirectory_;
    auto path = std::filesystem::path(directory) / "finetuning.json";
    file_.open(path.string(), std::ios::app);
}

void LlmFineTuningWriter::appendTurn(const std::vector<ChatMessage>& messages) {
    if (messages.empty()) {
        return;
    }

    std::string line = R"({"messages":[)";
    for (std::size_t i = 0; i < messages.size(); ++i) {
        if (i > 0) {
            line += ",";
        }
        line += chatMessageToJson(messages[i]);
    }
    line += "]}";

    std::scoped_lock lock(mutex_);
    ensureOpen();
    if (file_.is_open()) {
        file_ << line << "\n" << std::flush;
    }
}

} // namespace QaplaLlm
