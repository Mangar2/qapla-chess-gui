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

#include "llm/actions/gui-action-types.h"

using namespace QaplaLlm::Actions;

namespace {

/** @brief The sentence save_results/load_results answer with while a run holds the file. */
std::string locked(RunState state, const ActivityNames& names, FileAccess access) {
    return fileLockedSentence(lockOf(state), names, access);
}

bool mentions(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

} // namespace

TEST_CASE("An idle activity's file may be written and read", "[llm][gui-action-types]") {
    REQUIRE(locked(RunState::Idle, TOURNAMENT_NAMES, FileAccess::Save).empty());
    REQUIRE(locked(RunState::Idle, TOURNAMENT_NAMES, FileAccess::Load).empty());
    REQUIRE(locked(RunState::Idle, EPD_NAMES, FileAccess::Save).empty());
}

TEST_CASE("A running activity refuses both directions", "[llm][gui-action-types]") {
    // Both states that count as "running" have to refuse, not just the obvious one: Starting is
    // where the games are being laid out, which is exactly when a written file would be torn.
    for (auto state : {RunState::Starting, RunState::Running}) {
        auto save = locked(state, TOURNAMENT_NAMES, FileAccess::Save);
        auto load = locked(state, TOURNAMENT_NAMES, FileAccess::Load);
        REQUIRE_FALSE(save.empty());
        REQUIRE_FALSE(load.empty());

        // The refusal has to say nothing happened, or a caller reads it as a partial write.
        REQUIRE(mentions(save, "No file was written"));
        REQUIRE(mentions(load, "No file was read"));

        // A run that can still be stopped is told so; the user decides how.
        REQUIRE(mentions(save, "stop gracefully or abruptly"));
    }
}

TEST_CASE("A stopping activity is told to wait, not to stop it again", "[llm][gui-action-types]") {
    // The distinction RunLock draws: a run already on its way down cannot be stopped a second
    // time, and advising it is what used to send callers round a stop/start loop.
    for (auto state : {RunState::FinishingAfterGracefulStop, RunState::Aborting}) {
        auto save = locked(state, SPRT_NAMES, FileAccess::Save);
        REQUIRE_FALSE(save.empty());
        REQUIRE(mentions(save, "Wait"));
        REQUIRE_FALSE(mentions(save, "stop gracefully or abruptly"));
    }
}

TEST_CASE("The refusal names the activity it is about", "[llm][gui-action-types]") {
    // Three activities look alike from outside, so a refusal that doesn't name one is read as
    // being about whichever the caller had in mind.
    REQUIRE(mentions(locked(RunState::Running, SPRT_NAMES, FileAccess::Save), "SPRT test"));
    REQUIRE(mentions(locked(RunState::Running, EPD_NAMES, FileAccess::Load), "EPD analysis"));
    REQUIRE(mentions(locked(RunState::Aborting, TOURNAMENT_NAMES, FileAccess::Load), "tournament"));
}
