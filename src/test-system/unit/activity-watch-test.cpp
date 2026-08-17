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

#include "llm/activity-watch.h"

#include <chrono>
#include <thread>

using namespace QaplaLlm;
using Actions::Activity;
using Actions::ActivityProgress;
using Actions::RunState;

namespace {

constexpr auto SPRT = Activity::Sprt;
constexpr auto TOURNAMENT = Activity::Tournament;
constexpr auto CLOP = Activity::Clop;

/** @brief Puts the watched activity back to a known idle starting point. */
void reset(Activity activity) {
    ActivityWatch::instance().update(activity, ActivityProgress{.state = RunState::Running});
    ActivityWatch::instance().update(activity, ActivityProgress{.state = RunState::Idle});
}

/** @brief Feeds a state change from another thread after a short delay, as a frame loop would. */
std::thread feedAfter(std::chrono::milliseconds delay, Activity activity, ActivityProgress progress) {
    return std::thread([delay, activity, progress]() {
        std::this_thread::sleep_for(delay);
        ActivityWatch::instance().update(activity, progress);
    });
}

} // namespace

TEST_CASE("ActivityWatch answers at once when nothing is running", "[llm][activity-watch]") {
    reset(SPRT);

    // Deliberately a long limit: if this waited at all, the test would take it, which is the
    // point -- there is nothing to wait for and saying so must not cost the caller anything.
    auto result = ActivityWatch::instance().waitUntilIdle(SPRT, std::chrono::seconds(30));

    REQUIRE(result.reason == WaitReason::NotRunning);
}

TEST_CASE("ActivityWatch wakes a waiter when the run reaches its own conclusion",
    "[llm][activity-watch]") {
    reset(SPRT);
    ActivityWatch::instance().update(SPRT, ActivityProgress{.state = RunState::Running});

    auto feeder = feedAfter(std::chrono::milliseconds(50), SPRT,
        ActivityProgress{.state = RunState::Idle, .finished = true});
    auto result = ActivityWatch::instance().waitUntilIdle(SPRT, std::chrono::seconds(5));
    feeder.join();

    REQUIRE(result.reason == WaitReason::Finished);
}

TEST_CASE("ActivityWatch tells a run that was cut short from one that finished",
    "[llm][activity-watch]") {
    reset(TOURNAMENT);
    ActivityWatch::instance().update(TOURNAMENT, ActivityProgress{.state = RunState::Running});

    // Same resulting state, opposite news: this is the distinction the whole wait exists for.
    auto feeder = feedAfter(std::chrono::milliseconds(50), TOURNAMENT,
        ActivityProgress{.state = RunState::Idle, .finished = false});
    auto result = ActivityWatch::instance().waitUntilIdle(TOURNAMENT, std::chrono::seconds(5));
    feeder.join();

    REQUIRE(result.reason == WaitReason::Stopped);
}

TEST_CASE("ActivityWatch reports a timeout without disturbing the run", "[llm][activity-watch]") {
    reset(SPRT);
    ActivityWatch::instance().update(SPRT, ActivityProgress{.state = RunState::Running});

    auto result = ActivityWatch::instance().waitUntilIdle(SPRT, std::chrono::milliseconds(80));

    REQUIRE(result.reason == WaitReason::Timeout);
    REQUIRE(result.snapshot.progress.state == RunState::Running);
    reset(SPRT);
}

TEST_CASE("ActivityWatch counts runs so a caller can tell one from the next",
    "[llm][activity-watch]") {
    reset(SPRT);
    auto before = ActivityWatch::instance().snapshot(SPRT).run;

    ActivityWatch::instance().update(SPRT, ActivityProgress{.state = RunState::Starting});
    ActivityWatch::instance().update(SPRT, ActivityProgress{.state = RunState::Running});
    auto during = ActivityWatch::instance().snapshot(SPRT).run;
    ActivityWatch::instance().update(SPRT, ActivityProgress{.state = RunState::Idle});
    ActivityWatch::instance().update(SPRT, ActivityProgress{.state = RunState::Running});
    auto next = ActivityWatch::instance().snapshot(SPRT).run;

    // One per run, not one per state change: Starting -> Running is the same run.
    REQUIRE(during == before + 1);
    REQUIRE(next == during + 1);
    reset(SPRT);
}

TEST_CASE("ActivityWatch bumps its revision on every observed change", "[llm][activity-watch]") {
    reset(SPRT);
    auto before = ActivityWatch::instance().snapshot(SPRT).revision;

    ActivityWatch::instance().update(SPRT, ActivityProgress{.state = RunState::Running});
    auto afterChange = ActivityWatch::instance().snapshot(SPRT).revision;
    // An update that says nothing new must not count as a change.
    ActivityWatch::instance().update(SPRT, ActivityProgress{.state = RunState::Running});
    auto afterRepeat = ActivityWatch::instance().snapshot(SPRT).revision;

    REQUIRE(afterChange > before);
    REQUIRE(afterRepeat == afterChange);
    reset(SPRT);
}

TEST_CASE("ActivityWatch ends every wait with a reason when the channel closes",
    "[llm][activity-watch]") {
    reset(SPRT);
    ActivityWatch::instance().update(SPRT, ActivityProgress{.state = RunState::Running});

    std::thread closer([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ActivityWatch::instance().cancelWaits();
    });
    // A long limit again: the point is that closing wakes it, not that the limit does. Going
    // silent instead would look exactly like a run still taking its time.
    auto result = ActivityWatch::instance().waitUntilIdle(SPRT, std::chrono::seconds(30));
    closer.join();

    REQUIRE(result.reason == WaitReason::Closed);
    reset(SPRT);
}

TEST_CASE("ActivityWatch keeps waits on different activities independent",
    "[llm][activity-watch]") {
    reset(SPRT);
    reset(TOURNAMENT);
    ActivityWatch::instance().update(SPRT, ActivityProgress{.state = RunState::Running});
    ActivityWatch::instance().update(TOURNAMENT, ActivityProgress{.state = RunState::Running});

    // Only the tournament ends; the SPRT waiter must not be woken by it.
    auto feeder = feedAfter(std::chrono::milliseconds(50), TOURNAMENT,
        ActivityProgress{.state = RunState::Idle, .finished = true});
    auto tournamentResult =
        ActivityWatch::instance().waitUntilIdle(TOURNAMENT, std::chrono::seconds(5));
    auto sprtResult = ActivityWatch::instance().waitUntilIdle(SPRT, std::chrono::milliseconds(80));
    feeder.join();

    REQUIRE(tournamentResult.reason == WaitReason::Finished);
    REQUIRE(sprtResult.reason == WaitReason::Timeout);
    reset(SPRT);
    reset(TOURNAMENT);
}

TEST_CASE("ActivityWatch keeps a slot for every activity there is", "[llm][activity-watch]") {
    // The storage was a hand-written "[3]" while the enum grew to four, so the last activity
    // wrote past the end: its counters came back as whatever happened to be there, and a wait
    // answered "nothing is running" about a run that was going. Every value has to round-trip.
    for (auto activity : {Activity::Tournament, Activity::Sprt, Activity::Epd, Activity::Clop}) {
        reset(activity);
        ActivityWatch::instance().update(activity, ActivityProgress{.state = RunState::Running});
    }
    for (auto activity : {Activity::Tournament, Activity::Sprt, Activity::Epd, Activity::Clop}) {
        REQUIRE(ActivityWatch::instance().snapshot(activity).progress.state == RunState::Running);
    }

    // And they must not share one: ending the last must leave the first alone.
    ActivityWatch::instance().update(CLOP,
        ActivityProgress{.state = RunState::Idle, .finished = true});
    REQUIRE(ActivityWatch::instance().snapshot(TOURNAMENT).progress.state == RunState::Running);
    REQUIRE(ActivityWatch::instance().snapshot(SPRT).progress.state == RunState::Running);

    for (auto activity : {Activity::Tournament, Activity::Sprt, Activity::Epd, Activity::Clop}) {
        reset(activity);
    }
}
