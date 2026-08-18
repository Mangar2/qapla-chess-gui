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

#include "gui-action-types.h"

#include <cctype>
#include <format>

namespace QaplaLlm::Actions {

namespace {
    /** @brief "a tournament" -> "A tournament", for a name that starts a sentence. */
    std::string capitalized(std::string_view text) {
        std::string result{text};
        if (!result.empty()) {
            result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
        }
        return result;
    }
} // namespace

std::string runStateSentence(RunState state, const ActivityNames& names) {
    switch (state) {
        case RunState::Starting:
            return std::format("{} is currently starting.", capitalized(names.withArticle));
        case RunState::Running:
            return std::format("{} is currently running.", capitalized(names.withArticle));
        case RunState::FinishingAfterGracefulStop:
            // Never phrased as "running": in this state it accepts no new work and can neither be
            // reconfigured nor restarted, so calling it running invites the reader to treat it
            // like a live run.
            return std::format("{} is finishing its {} after a graceful stop. No new {} start.",
                capitalized(names.withArticle), names.workItems, names.workItems);
        case RunState::Aborting:
            return std::format("{} is being aborted.", capitalized(names.withArticle));
        case RunState::Idle:
        default:
            return std::format("No {} is currently running.", names.bare);
    }
}

std::string runStatePhrase(RunState state, const ActivityNames& names, unsigned int concurrency) {
    switch (state) {
        // Concurrency belongs to the two states where it is actually in force. A run on its way
        // out starts nothing new, so naming a parallelism it is no longer using would describe
        // work that isn't happening -- the same reason concurrencySentence() reports those two
        // states as applying to the next run rather than this one.
        case RunState::Starting:
            return std::format(
                "{} is starting with concurrency {}", names.withArticle, concurrency);
        case RunState::Running:
            return std::format("{} is running with concurrency {}", names.withArticle, concurrency);
        case RunState::FinishingAfterGracefulStop:
            return std::format(
                "{} is finishing its {} after a graceful stop", names.withArticle, names.workItems);
        case RunState::Aborting: return std::format("{} is being aborted", names.withArticle);
        case RunState::Idle:
        default: return "";
    }
}

RunLock lockOf(RunState state) {
    switch (state) {
        case RunState::Starting:
        case RunState::Running:
            return RunLock::Running;
        case RunState::FinishingAfterGracefulStop:
        case RunState::Aborting:
            return RunLock::Stopping;
        case RunState::Idle:
        default:
            return RunLock::None;
    }
}

std::string settingsLockedSentence(RunLock lock, const ActivityNames& names) {
    switch (lock) {
        case RunLock::Running:
            return std::format(
                "Settings and engine selection are locked while {} runs. Nothing changed. Ask the "
                "user: stop gracefully or abruptly? Then set the values and start. Only "
                "concurrency can be changed while running.",
                names.withArticle);
        case RunLock::Stopping:
            return std::format(
                "Settings and engine selection are locked until the {} has stopped. Nothing "
                "changed. Wait, then set the values and start.",
                names.bare);
        case RunLock::None:
        default:
            return "";
    }
}

std::string settingsLockNote(RunLock lock) {
    switch (lock) {
        case RunLock::Running:
            return "Settings and engine selection are locked right now; only concurrency can be "
                   "changed.";
        case RunLock::Stopping:
            return "Settings and engine selection are locked until it has stopped.";
        case RunLock::None:
        default:
            return "";
    }
}

std::string fileLockedSentence(RunLock lock, const ActivityNames& names, FileAccess access) {
    if (lock == RunLock::None) {
        return "";
    }
    const std::string_view what = access == FileAccess::Save ? "written" : "read";
    if (lock == RunLock::Running) {
        return std::format(
            "No file was {}: {} is running, and its file can only be {} once it has stopped. Ask "
            "the user: stop gracefully or abruptly? Then try again.",
            what, names.withArticle, what);
    }
    return std::format(
        "No file was {}: the {} is still stopping, and its file can only be {} once it has. Wait, "
        "then try again.",
        what, names.bare, what);
}

std::string readyToStartSentence() {
    return "Everything it needs is configured; it can be started exactly as it is.";
}

std::string concurrencySentence(
    RunState state, const ActivityNames& names, unsigned int count) {
    switch (state) {
        case RunState::Running:
            return std::format("{} is now running with concurrency {}. Nothing else needed.",
                capitalized(names.withArticle), count);
        case RunState::Starting:
            return std::format("{} is starting with concurrency {}. Nothing else needed.",
                capitalized(names.withArticle), count);
        // Includes both pending-stop states: a run on its way out starts no new work, so the new
        // value reaches nothing that is currently playing -- applySettings() pushes it to the pool
        // under exactly the same condition, so text and behaviour cannot drift apart.
        case RunState::FinishingAfterGracefulStop:
        case RunState::Aborting:
        case RunState::Idle:
        default:
            return std::format(
                "Concurrency is now {}. The next {} will use it.", count, names.bare);
    }
}

std::string joinList(const std::vector<std::string>& values) {
    std::string joined;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            joined += ", ";
        }
        joined += values[i];
    }
    return joined;
}

void applyAdjudicationMode(AdjudicationMode mode, bool& active, bool& testOnly) {
    active = mode != AdjudicationMode::Off;
    testOnly = mode == AdjudicationMode::Test;
}

AdjudicationMode adjudicationModeOf(bool active, bool testOnly) {
    if (!active) {
        return AdjudicationMode::Off;
    }
    return testOnly ? AdjudicationMode::Test : AdjudicationMode::Active;
}

std::string adjudicationModeName(AdjudicationMode mode) {
    switch (mode) {
        case AdjudicationMode::Test: return "test";
        case AdjudicationMode::Active: return "active";
        case AdjudicationMode::Off:
        default: return "off";
    }
}

} // namespace QaplaLlm::Actions
