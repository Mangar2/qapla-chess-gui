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
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include <catch2/catch_test_macros.hpp>

#include "clock-model.h"

#include <base-elements/time-control.h>
#include <chess-game/game-record.h>
#include <chess-game/move-record.h>

#include <cstdint>
#include <string>

using QaplaTester::GameRecord;
using QaplaTester::MoveRecord;
using QaplaTester::TimeControl;
using QaplaTester::TimeSegment;
using QaplaWindows::ClockModel;
using QaplaWindows::ClockView;

namespace {

constexpr uint64_t BASE_TIME_MS = 60000;

/**
 * @brief Builds a sudden death time control.
 */
TimeControl suddenDeath(uint64_t baseTimeMs, uint64_t incrementMs = 0) {
    TimeControl tc;
    tc.addTimeSegment(TimeSegment{ .movesToPlay = 0, .baseTimeMs = baseTimeMs, .incrementMs = incrementMs });
    return tc;
}

/**
 * @brief Drives a ClockModel with a clock the test moves by hand.
 *
 * The fixture mirrors the four events the GUI feeds into the model: a changed game record,
 * a search info of one player, and the stopped and analyze flags. No real time is involved,
 * so every assertion is exact.
 */
struct ClockFixture {
    uint64_t nowMs = 0;
    ClockModel model{ [this] { return nowMs; } };
    GameRecord record;

    ClockFixture() {
        record.setStartPosition(true, "", true, 0, "White Engine", "Black Engine");
        record.setTimeControl(suddenDeath(BASE_TIME_MS), suddenDeath(BASE_TIME_MS));
    }

    /** @brief Moves the fake clock forward. */
    void advance(uint64_t ms) { nowMs += ms; }

    /** @brief Hands the current game record to the model, as the draw loop does every frame. */
    void sync() { model.setFromGameRecord(record); }

    /** @brief Appends a finished move to the game record. */
    void playMove(uint64_t timeMs, const std::string& engineName) {
        MoveRecord move(record.halfmoveNoAtPly(record.nextMoveIndex()));
        move.timeMs = timeMs;
        move.engineName_ = engineName;
        record.addMove(move);
    }

    /** @brief Builds a search info for the move currently being computed. */
    [[nodiscard]] MoveRecord searchInfo(uint64_t timeMs, uint32_t infoUpdateCount) const {
        MoveRecord move(record.halfmoveNoAtPly(record.nextMoveIndex()));
        move.timeMs = timeMs;
        move.infoUpdateCount = infoUpdateCount;
        return move;
    }

    /** @brief Feeds a search info of one player into the model. */
    void info(uint32_t playerIndex, uint64_t timeMs, uint32_t infoUpdateCount,
              const std::string& engineName = "White Engine") {
        MoveRecord move = searchInfo(timeMs, infoUpdateCount);
        move.engineName_ = engineName;
        model.setFromMoveRecord(move, playerIndex);
    }
};

} // anonymous namespace

TEST_CASE_METHOD(ClockFixture, "A fresh game shows both base times with white to move", "[gui][clock]") {
    sync();

    const ClockView view = model.view();
    CHECK(view.whiteToMove);
    CHECK(view.whiteLeftMs == BASE_TIME_MS);
    CHECK(view.blackLeftMs == BASE_TIME_MS);
    CHECK(view.whiteCurMoveMs == 0);
    CHECK(view.blackCurMoveMs == 0);
    CHECK(view.whiteEngineName == "White Engine");
    CHECK(view.blackEngineName == "Black Engine");
    // The remaining time is padded by 999 ms so that a full minute is shown as such.
    CHECK(view.whiteTotal == "01:00");
    CHECK(view.whiteMove == "00:00");
}

TEST_CASE_METHOD(ClockFixture, "A search info that names its engine puts that name under the clock",
    "[gui][clock]") {
    sync();
    info(0, 500, 1, "Searching Engine");

    const ClockView view = model.view();
    CHECK(view.whiteEngineName == "Searching Engine");
    CHECK(view.blackEngineName == "Black Engine");
}

TEST_CASE_METHOD(ClockFixture, "A search info that names no engine leaves the game's players alone",
    "[gui][clock]") {
    // What a recomputed game looks like: the moves come from a PGN and name no engine, and the
    // engine doing the recomputing is not one of the two players. The side that is thinking used
    // to end up as the only one without a name under its clock.
    sync();
    info(0, 500, 1, "");

    const ClockView view = model.view();
    CHECK(view.whiteEngineName == "White Engine");
    CHECK(view.blackEngineName == "Black Engine");
}

TEST_CASE_METHOD(ClockFixture, "Both players keep their names while a recomputed game is walked",
    "[gui][clock]") {
    // Move by move, as a backward analysis does it: every position is searched, no move names an
    // engine, and both clocks have to stay labelled the whole way.
    playMove(1000, "");
    playMove(1200, "");
    record.setNextMoveIndex(2);
    sync();
    info(1, 400, 1, "");

    const ClockView view = model.view();
    CHECK(view.whiteEngineName == "White Engine");
    CHECK(view.blackEngineName == "Black Engine");
}

TEST_CASE_METHOD(ClockFixture, "An analysis says so instead of naming a player", "[gui][clock]") {
    sync();
    model.setAnalyze(true);
    info(0, 500, 1, "Searching Engine");

    const ClockView view = model.view();
    CHECK(view.whiteEngineName == "Analyze");
}

TEST_CASE_METHOD(ClockFixture, "Nothing runs before the first search info arrives", "[gui][clock]") {
    sync();
    advance(5000);

    const ClockView view = model.view();
    CHECK(view.whiteCurMoveMs == 0);
    CHECK(view.blackCurMoveMs == 0);
}

TEST_CASE_METHOD(ClockFixture, "A search info starts the running time of the side to move", "[gui][clock]") {
    sync();
    info(0, 500, 1);
    advance(2000);

    const ClockView view = model.view();
    CHECK(view.whiteCurMoveMs == 2500);
    CHECK(view.blackCurMoveMs == 0);
    CHECK(view.whiteMove == "00:02");
    // 60000 - 2500 + 999 ms, truncated to full seconds.
    CHECK(view.whiteTotal == "00:58");
}

TEST_CASE_METHOD(ClockFixture, "Only the side to move counts down", "[gui][clock]") {
    sync();
    info(0, 1000, 1);
    advance(3000);
    const uint64_t blackBefore = model.view().blackCurMoveMs;
    advance(3000);

    const ClockView view = model.view();
    CHECK(view.whiteCurMoveMs == 7000);
    CHECK(view.blackCurMoveMs == blackBefore);
}

TEST_CASE_METHOD(ClockFixture, "A played move moves the running time to the other side", "[gui][clock]") {
    sync();
    info(0, 1000, 1);
    advance(3000);
    playMove(4000, "White Engine");
    sync();
    advance(1500);

    const ClockView view = model.view();
    CHECK_FALSE(view.whiteToMove);
    CHECK(view.whiteLeftMs == BASE_TIME_MS - 4000);
    CHECK(view.blackLeftMs == BASE_TIME_MS);
    CHECK(view.whiteCurMoveMs == 0);
    CHECK(view.blackCurMoveMs == 1500);
}

TEST_CASE_METHOD(ClockFixture, "Stopping freezes the display against the passing time", "[gui][clock]") {
    sync();
    info(0, 1000, 1);
    advance(2000);
    model.setStopped(true);
    const uint64_t frozen = model.view().whiteCurMoveMs;
    advance(30000);

    CHECK(frozen == 3000);
    CHECK(model.view().whiteCurMoveMs == frozen);
    CHECK(model.isStopped());
}

TEST_CASE_METHOD(ClockFixture, "A search info while stopped is ignored", "[gui][clock]") {
    sync();
    info(0, 1000, 1);
    advance(2000);
    model.setStopped(true);
    info(0, 9000, 2);

    CHECK(model.view().whiteCurMoveMs == 3000);
}

TEST_CASE_METHOD(ClockFixture, "A repeated search info does not restart the running time", "[gui][clock]") {
    sync();
    info(0, 1000, 1);
    advance(1000);
    // Same update count and same half-move: the engine has not reported anything new.
    info(0, 9999, 1);
    advance(1000);

    CHECK(model.view().whiteCurMoveMs == 3000);
}

TEST_CASE_METHOD(ClockFixture, "A search info for another half-move is ignored", "[gui][clock]") {
    sync();
    MoveRecord stale = searchInfo(5000, 1);
    stale.halfmoveNo_ += 4;
    model.setFromMoveRecord(stale, 0);

    const ClockView view = model.view();
    CHECK(view.whiteCurMoveMs == 0);
    CHECK(view.whiteEngineName == "White Engine");
}

TEST_CASE_METHOD(ClockFixture, "A ponder info does not touch the clock", "[gui][clock]") {
    sync();
    MoveRecord ponder = searchInfo(5000, 1);
    ponder.ponderMove = "e7e5";
    ponder.engineName_ = "Pondering Engine";
    model.setFromMoveRecord(ponder, 0);

    const ClockView view = model.view();
    CHECK(view.whiteCurMoveMs == 0);
    CHECK(view.whiteEngineName == "White Engine");
}

TEST_CASE_METHOD(ClockFixture, "A rising move time keeps the running time monotone", "[gui][clock]") {
    sync();
    info(0, 1000, 1);
    advance(500);
    CHECK(model.view().whiteCurMoveMs == 1500);
    // The engine reports a higher time: the stopwatch restarts on top of the reported value.
    info(0, 2000, 2);
    CHECK(model.view().whiteCurMoveMs == 2000);
    advance(700);
    CHECK(model.view().whiteCurMoveMs == 2700);
}

TEST_CASE_METHOD(ClockFixture, "Analyze mode shows the search time instead of a remaining time", "[gui][clock]") {
    model.setAnalyze(true);
    sync();
    info(0, 2500, 1);

    const ClockView view = model.view();
    CHECK(view.analyze);
    CHECK(view.whiteTotal == "00:02");
    CHECK(view.whiteMove == "00:02");
    CHECK(view.whiteEngineName == "Analyze");
}

TEST_CASE_METHOD(ClockFixture, "The running time of the side to move stamps a human move", "[gui][clock]") {
    sync();
    info(0, 1000, 1);
    advance(1200);

    // Only the stopwatch counts here, not the time the engine already reported.
    CHECK(model.currentTimerMs() == 1200);
}

TEST_CASE_METHOD(ClockFixture, "A game record without a time control leaves the display alone", "[gui][clock]") {
    sync();
    info(0, 1000, 1);
    advance(500);
    const ClockView before = model.view();

    GameRecord untimed;
    untimed.setStartPosition(true, "", true, 0, "Other White", "Other Black");
    model.setFromGameRecord(untimed);

    const ClockView view = model.view();
    CHECK(view.whiteLeftMs == before.whiteLeftMs);
    CHECK(view.whiteEngineName == before.whiteEngineName);
}

TEST_CASE_METHOD(ClockFixture, "An unchanged game record does not reset the running time", "[gui][clock]") {
    sync();
    info(0, 1000, 1);
    advance(2000);
    // The draw loop hands over the same record on every frame.
    sync();
    sync();

    CHECK(model.view().whiteCurMoveMs == 3000);
}

TEST_CASE_METHOD(ClockFixture, "Rewinding into the history shows the time of the move played there", "[gui][clock]") {
    sync();
    playMove(4000, "White Engine");
    playMove(3000, "Black Engine");
    sync();
    model.setStopped(true);
    record.setNextMoveIndex(1);
    sync();

    const ClockView view = model.view();
    CHECK_FALSE(view.whiteToMove);
    // White's move is the last one played before the shown position, so its time is on display.
    CHECK(view.whiteCurMoveMs == 4000);
    CHECK(view.whiteEngineName == "White Engine");
}

TEST_CASE_METHOD(ClockFixture,
    "A search info of the side that is not to move does not touch the clock", "[gui][clock]") {
    sync();
    // White is to move; both engines report, so an info of black's engine can only be a
    // ponder search or a late info of the previous move.
    info(0, 1000, 1);
    info(1, 3000, 1, "Black Engine");

    const ClockView view = model.view();
    CHECK(view.whiteCurMoveMs == 1000);
    CHECK(view.whiteEngineName == "White Engine");
    CHECK(view.blackCurMoveMs == 0);
    CHECK(view.blackEngineName == "Black Engine");
}

TEST_CASE_METHOD(ClockFixture,
    "A single engine analysing for black drives black's clock", "[gui][clock]") {
    // An EPD position with black to move, analysed by one engine, which reports as player 0.
    record.setStartPosition(true, "", false, 0, "White Engine", "Black Engine");
    record.setTimeControl(suddenDeath(BASE_TIME_MS), suddenDeath(BASE_TIME_MS));
    sync();
    info(0, 2000, 1, "Analysing Engine");
    advance(500);

    const ClockView view = model.view();
    CHECK_FALSE(view.whiteToMove);
    CHECK(view.blackCurMoveMs == 2500);
    CHECK(view.blackEngineName == "Analysing Engine");
    CHECK(view.whiteCurMoveMs == 0);
}

TEST_CASE_METHOD(ClockFixture,
    "In analyze mode both engines feed the clock of the side to move", "[gui][clock]") {
    model.setAnalyze(true);
    sync();
    info(0, 1000, 1);
    // In analyze mode every engine searches the same position, so the second engine's info
    // belongs to the same side.
    info(1, 2000, 1, "Second Engine");

    const ClockView view = model.view();
    CHECK(view.whiteCurMoveMs == 2000);
    CHECK(view.whiteEngineName == "Analyze");
    CHECK(view.blackCurMoveMs == 0);
}

TEST_CASE_METHOD(ClockFixture, "An increment is credited per move actually played", "[gui][clock]") {
    constexpr uint64_t INCREMENT_MS = 1000;
    record.setTimeControl(suddenDeath(BASE_TIME_MS, INCREMENT_MS), suddenDeath(BASE_TIME_MS, INCREMENT_MS));
    sync();

    SECTION("Before the first move neither side has earned an increment") {
        const ClockView view = model.view();
        CHECK(view.whiteLeftMs == BASE_TIME_MS);
        CHECK(view.blackLeftMs == BASE_TIME_MS);
    }

    SECTION("After white's move only white has earned one") {
        playMove(4000, "White Engine");
        sync();

        const ClockView view = model.view();
        CHECK(view.whiteLeftMs == BASE_TIME_MS + INCREMENT_MS - 4000);
        CHECK(view.blackLeftMs == BASE_TIME_MS);
    }

    SECTION("After both sides moved each has earned one") {
        playMove(4000, "White Engine");
        playMove(3000, "Black Engine");
        sync();

        const ClockView view = model.view();
        CHECK(view.whiteLeftMs == BASE_TIME_MS + INCREMENT_MS - 4000);
        CHECK(view.blackLeftMs == BASE_TIME_MS + INCREMENT_MS - 3000);
    }
}

TEST_CASE_METHOD(ClockFixture,
    "Stepping through a loaded game keeps the name of the side that just moved", "[gui][clock]") {
    // Moves read from a PGN carry no engine name -- the names come from the game's tags.
    playMove(4000, "");
    playMove(3000, "");
    sync();
    model.setStopped(true);
    record.setNextMoveIndex(1);
    sync();

    const ClockView view = model.view();
    CHECK_FALSE(view.whiteToMove);
    CHECK(view.whiteCurMoveMs == 4000);
    CHECK(view.whiteEngineName == "White Engine");
    CHECK(view.blackEngineName == "Black Engine");
}

TEST_CASE_METHOD(ClockFixture,
    "Loading a game without engine names clears the names of the previous game", "[gui][clock]") {
    // Two moves played by two engines, which name themselves in their search infos.
    sync();
    info(0, 1000, 1, "Spike");
    playMove(4000, "Spike");
    sync();
    info(1, 900, 1, "Qapla");
    playMove(3000, "Qapla");
    sync();
    REQUIRE(model.view().whiteEngineName == "Spike");
    REQUIRE(model.view().blackEngineName == "Qapla");

    // A PGN without engine tags replaces the game, and stepping through it is done stopped.
    model.setStopped(true);
    GameRecord loaded;
    loaded.setStartPosition(true, "", true, 0, "", "");
    loaded.setTimeControl(suddenDeath(BASE_TIME_MS), suddenDeath(BASE_TIME_MS));
    MoveRecord pgnMove(loaded.halfmoveNoAtPly(loaded.nextMoveIndex()));
    pgnMove.timeMs = 2000;
    loaded.addMove(pgnMove);
    model.setFromGameRecord(loaded);

    const ClockView view = model.view();
    CHECK(view.whiteEngineName.empty());
    CHECK(view.blackEngineName.empty());
    CHECK(view.whiteCurMoveMs == 2000);
}
