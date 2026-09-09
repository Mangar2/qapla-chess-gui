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

#include "ui-thread-watch.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

using QaplaWindows::UiThreadWatch;

namespace {

void spend(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

} // namespace

TEST_CASE("UiThreadWatch leaves a quick frame alone", "[ui-thread-watch]") {
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    watch.frameBegin();
    {
        UiThreadWatch::Section section("draw");
    }
    watch.frameEnd();

    auto report = watch.report();
    REQUIRE(report.frames == 1);
    REQUIRE(report.stalls == 0);
    watch.reset();
}

TEST_CASE("UiThreadWatch counts a frame that took too long", "[ui-thread-watch]") {
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    watch.frameBegin();
    spend(UiThreadWatch::stallThreshold() + std::chrono::milliseconds(20));
    watch.frameEnd();

    auto report = watch.report();
    REQUIRE(report.frames == 1);
    REQUIRE(report.stalls == 1);
    REQUIRE(report.worstFrameMs > static_cast<double>(UiThreadWatch::stallThreshold().count()));
    watch.reset();
}

TEST_CASE("UiThreadWatch blames the innermost section, not the one around it",
    "[ui-thread-watch]") {
    // The whole point of the attribution. An enclosing section is always at least as long as what
    // it encloses, so comparing durations alone would answer "poll" for every frame -- which
    // names the frame loop for work done by one identifiable tool inside it.
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    watch.frameBegin();
    {
        UiThreadWatch::Section outer("poll");
        {
            UiThreadWatch::Section inner("tool:manage_engines");
            spend(UiThreadWatch::stallThreshold() + std::chrono::milliseconds(20));
        }
    }
    watch.frameEnd();

    auto report = watch.report();
    REQUIRE(report.stalls == 1);
    REQUIRE(report.worstSection == "tool:manage_engines");
    watch.reset();
}

TEST_CASE("UiThreadWatch names the enclosing section when the time really was spent there",
    "[ui-thread-watch]") {
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    watch.frameBegin();
    {
        UiThreadWatch::Section outer("draw");
        {
            UiThreadWatch::Section inner("tool:get_status");
        }
        spend(UiThreadWatch::stallThreshold() + std::chrono::milliseconds(20));
    }
    watch.frameEnd();

    auto report = watch.report();
    REQUIRE(report.stalls == 1);
    REQUIRE(report.worstSection == "draw");
    watch.reset();
}

TEST_CASE("UiThreadWatch does not blame a frame for waiting", "[ui-thread-watch]") {
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    watch.frameBegin();
    {
        // What waiting for the game manager pool looks like: engines being stopped, nothing to
        // draw until they are, and none of it work of this thread's.
        UiThreadWatch::Waiting waiting(UiThreadWatch::POOL_SECTION);
        spend(UiThreadWatch::stallThreshold() + std::chrono::milliseconds(40));
    }
    watch.frameEnd();

    auto report = watch.report();
    CHECK(report.frames == 1);
    CHECK(report.stalls == 0);
    CHECK(report.waitedMs > 0.0);
    watch.reset();
}

TEST_CASE("UiThreadWatch still blames what a waiting frame did besides waiting",
    "[ui-thread-watch]") {
    // The reason the waiting is subtracted rather than the frame excused: a frame that waited
    // and *also* did something it should not have must still be reported, and by name.
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    watch.frameBegin();
    {
        UiThreadWatch::Waiting waiting(UiThreadWatch::POOL_SECTION);
        spend(UiThreadWatch::stallThreshold());
    }
    {
        UiThreadWatch::Section section("tool:something-slow");
        spend(UiThreadWatch::stallThreshold() + std::chrono::milliseconds(40));
    }
    watch.frameEnd();

    auto report = watch.report();
    CHECK(report.stalls == 1);
    CHECK(report.worstSection == "tool:something-slow");
    // The frame is judged by its work alone, so the waiting is not in this number.
    CHECK(report.worstFrameMs > 0.0);
    CHECK(report.worstFrameWaitedMs > 0.0);
    watch.reset();
}

TEST_CASE("UiThreadWatch counts a wait inside a wait only once", "[ui-thread-watch]") {
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    watch.frameBegin();
    const auto waited = UiThreadWatch::stallThreshold() + std::chrono::milliseconds(40);
    {
        UiThreadWatch::Waiting outer(UiThreadWatch::POOL_SECTION);
        {
            UiThreadWatch::Waiting inner(UiThreadWatch::POOL_SECTION);
            spend(waited);
        }
    }
    watch.frameEnd();

    auto report = watch.report();
    CHECK(report.stalls == 0);
    // Twice the wait would be more than the frame lasted, and the frame would come out negative.
    const double waitedMs = std::chrono::duration<double, std::milli>(waited).count();
    CHECK(report.waitedMs < waitedMs * 1.5);
    watch.reset();
}

TEST_CASE("UiThreadWatch says how much of a long frame went on waiting", "[ui-thread-watch]") {
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    watch.frameBegin();
    {
        UiThreadWatch::Waiting waiting(UiThreadWatch::POOL_SECTION);
        spend(std::chrono::milliseconds(30));
    }
    {
        UiThreadWatch::Section section("draw");
        spend(UiThreadWatch::stallThreshold() + std::chrono::milliseconds(40));
    }
    watch.frameEnd();

    auto report = watch.report();
    REQUIRE(report.stalls == 1);
    // The breakdown still names the waiting, so a long frame says what it was waiting for.
    const bool namesTheWait = std::ranges::any_of(report.worstFrameSections,
        [](const auto& entry) { return entry.name == UiThreadWatch::POOL_SECTION; });
    CHECK(namesTheWait);
    watch.reset();
}

TEST_CASE("UiThreadWatch measures the time no section claimed", "[ui-thread-watch]") {
    // The hole that hid a stall once: time inside a frame that no section covers still lands in
    // the frame, and the frame is then reported against the longest section that has a name --
    // which can be a short one that had nothing to do with it.
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    UiThreadWatch::Stall seen;
    watch.setStallCallback([&seen](const UiThreadWatch::Stall& stall) { seen = stall; });

    watch.frameBegin();
    {
        UiThreadWatch::Section section("draw");
        spend(std::chrono::milliseconds(10));
    }
    spend(UiThreadWatch::stallThreshold() + std::chrono::milliseconds(40));
    watch.frameEnd();
    watch.setStallCallback(nullptr);

    CHECK(watch.report().stalls == 1);
    CHECK(seen.section == "draw");
    // Almost all of it belonged to nobody, and that is what says where to look.
    CHECK(seen.unnamedMs > seen.frameMs / 2.0);
    watch.reset();
}

TEST_CASE("UiThreadWatch claims no unnamed time when every part is named", "[ui-thread-watch]") {
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    UiThreadWatch::Stall seen;
    watch.setStallCallback([&seen](const UiThreadWatch::Stall& stall) { seen = stall; });

    watch.frameBegin();
    {
        UiThreadWatch::Section section("draw");
        spend(UiThreadWatch::stallThreshold() + std::chrono::milliseconds(40));
    }
    watch.frameEnd();
    watch.setStallCallback(nullptr);

    REQUIRE(watch.report().stalls == 1);
    CHECK(seen.section == "draw");
    CHECK(seen.unnamedMs < 10.0);
    watch.reset();
}

TEST_CASE("UiThreadWatch accounts for every millisecond of a frame", "[ui-thread-watch]") {
    // The check that makes the rest worth reading: own times plus the time nobody claimed have
    // to add up to the frame. If they do not, no name in the breakdown means anything.
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    UiThreadWatch::Stall seen;
    watch.setStallCallback([&seen](const UiThreadWatch::Stall& stall) { seen = stall; });

    watch.frameBegin();
    {
        UiThreadWatch::Section outer("draw");
        spend(std::chrono::milliseconds(20));
        {
            UiThreadWatch::Section inner("draw:something");
            spend(UiThreadWatch::stallThreshold());
        }
    }
    spend(std::chrono::milliseconds(20));
    watch.frameEnd();
    watch.setStallCallback(nullptr);

    REQUIRE(watch.report().stalls == 1);
    CHECK(std::abs(seen.residualMs) < 1.0);
    CHECK(seen.elapsedMs > seen.frameMs - 1.0);

    // The enclosing section spent its own twenty milliseconds; the rest was the one inside it.
    const auto outer = std::ranges::find_if(seen.sections,
        [](const auto& entry) { return entry.name == "draw"; });
    const auto inner = std::ranges::find_if(seen.sections,
        [](const auto& entry) { return entry.name == "draw:something"; });
    REQUIRE(outer != seen.sections.end());
    REQUIRE(inner != seen.sections.end());
    CHECK(outer->time.totalMs > inner->time.totalMs);
    CHECK(outer->time.selfMs < inner->time.selfMs);
}

TEST_CASE("UiThreadWatch reports a frame that has not ended yet", "[ui-thread-watch]") {
    // Read from another thread while the UI thread is stuck inside the frame -- the one moment
    // the number matters most, and the reason /health carries it.
    auto& watch = UiThreadWatch::instance();
    watch.reset();

    watch.frameBegin();
    UiThreadWatch::Section section("tool:manage_engines");
    spend(std::chrono::milliseconds(30));

    auto report = watch.report();
    REQUIRE(report.currentFrameMs >= 25.0);
    REQUIRE(report.currentSection == "tool:manage_engines");

    watch.frameEnd();
    watch.reset();
}
