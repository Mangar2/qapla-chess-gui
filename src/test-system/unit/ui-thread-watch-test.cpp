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

#include <chrono>
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
