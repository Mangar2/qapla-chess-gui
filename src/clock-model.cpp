/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "clock-model.h"

#include <base-elements/string-helper.h>
#include <base-elements/time-control.h>
#include <base-elements/timer.h>
#include <chess-game/game-record.h>
#include <chess-game/move-record.h>

#include <algorithm>

using QaplaTester::GameRecord;
using QaplaTester::GoLimits;
using QaplaTester::MoveRecord;

using namespace QaplaWindows;

namespace {

/**
 * @brief Prepares the time strings for display.
 * @param totalMs Total remaining time in milliseconds.
 * @param moveMs Time for the current move in milliseconds.
 * @param analyze Whether in analyze mode.
 * @return Pair of formatted time strings (total, move).
 */
std::pair<std::string, std::string> prepareTimeStrings(uint64_t totalMs, uint64_t moveMs, bool analyze) {
    uint64_t adjustedTotal = totalMs - std::min(totalMs, moveMs);
    if (!analyze) {
        adjustedTotal += 999; // Add 999ms to compensate for formatMs truncating to full seconds
    } else {
        adjustedTotal = moveMs;
    }
    return {
        QaplaHelpers::formatMs(adjustedTotal, 0),
        QaplaHelpers::formatMs(moveMs, 0)
    };
}

} // anonymous namespace

ClockModel::ClockModel()
    : ClockModel(&QaplaHelpers::Timer::getCurrentTimeMs) {
}

ClockModel::ClockModel(NowFn now)
    : now_(std::move(now)) {
}

void ClockModel::setFromGameRecord(const GameRecord& gameRecord) {
    auto [modification, update] = gameRecordTracker_.checkModification(gameRecord.getChangeTracker());
    if (!update) {
        return;
    }
    gameRecordTracker_.updateFrom(gameRecord.getChangeTracker());

    const auto& wtc = gameRecord.getWhiteTimeControl();
    const auto& btc = gameRecord.getBlackTimeControl();
    if (!wtc.isValid() || !btc.isValid()) {
        return;
    }
    auto [whiteTime, blackTime] = gameRecord.timeUsed();
    auto nextMoveIndex = gameRecord.nextMoveIndex();
    // createGoLimits counts the half moves already played, which is the ply index; the
    // half-move number would be one too high and would credit the wrong side with an
    // increment. The engine side passes the same value, see GameManager::computeNextMove.
    GoLimits goLimits = createGoLimits(wtc, btc,
        nextMoveIndex, whiteTime, blackTime, gameRecord.isWhiteToMove());

    if (modification) {
        whiteEngineName_ = gameRecord.getWhiteEngineName();
        blackEngineName_ = gameRecord.getBlackEngineName();
    }
    whiteTimeLeftMs_ = goLimits.wtimeMs;
    blackTimeLeftMs_ = goLimits.btimeMs;
    whiteTimeCurMoveMs_ = 0;
    blackTimeCurMoveMs_ = 0;
    whiteToMove_ = gameRecord.isWhiteToMove();
    whiteStopwatch_.reset();
    blackStopwatch_.reset();
    nextHalfmoveNo_ = gameRecord.halfmoveNoAtPly(nextMoveIndex);

    if (nextMoveIndex > 0) {
        setFromHistoryMove(gameRecord.history()[nextMoveIndex - 1]);
    }
}

void ClockModel::setFromHistoryMove(const MoveRecord& moveRecord) {
    // A move played by an engine names it; a move read from a PGN does not, and there the
    // name comes from the game's tags. Keeping the tag name is what lets both clocks stay
    // labelled while stepping through a loaded game.
    const bool moveNamesItsEngine = !moveRecord.engineName_.empty();

    // if white is to move, then black just moved (moveRecord is black's move)
    if (whiteToMove_) {
        if (!stopped_) {
            whiteStopwatch_.start(now_());
        } else {
            blackTimeCurMoveMs_ = moveRecord.timeMs;
            // blackTimeLeftMs_ has the time after current move.
            blackTimeLeftMs_ += moveRecord.timeMs;
            if (moveNamesItsEngine) {
                blackEngineName_ = moveRecord.engineName_;
            }
        }
    } else {
        if (!stopped_) {
            blackStopwatch_.start(now_());
        } else {
            whiteTimeCurMoveMs_ = moveRecord.timeMs;
            // whiteTimeLeftMs_ has the time after current move.
            whiteTimeLeftMs_ += moveRecord.timeMs;
            if (moveNamesItsEngine) {
                whiteEngineName_ = moveRecord.engineName_;
            }
        }
    }
}

void ClockModel::setFromMoveRecord(const MoveRecord& moveRecord, uint32_t playerIndex) {
    if (stopped_) {
        return;
    }
    if (!moveRecord.ponderMove.empty()) {
        // Time used from pondering is not relevant
        return;
    }
    auto halfmoveNo = moveRecord.halfmoveNo_;
    if (infoCnt_.size() <= playerIndex) {
        infoCnt_.resize(playerIndex + 1, 0);
        displayedMoveNo_.resize(playerIndex + 1, 0);
    }
    // A search info always belongs to the side to move. The player index says which engine
    // sent it (0 = white, 1 = black, already corrected for switched sides), so once both
    // players report, an info of the other engine is a ponder search or a late info of the
    // previous move and must not touch the clock. It cannot be used to pick the side: in
    // analyze mode every engine searches the same position, and a single engine analysing an
    // EPD position searches for whichever side is to move.
    if (!analyze_ && infoCnt_.size() > 1 && (playerIndex == 0) != whiteToMove_) {
        return;
    }
    if (halfmoveNo != nextHalfmoveNo_) {
        return;
    }
    if (moveRecord.infoUpdateCount == infoCnt_[playerIndex] && halfmoveNo == displayedMoveNo_[playerIndex]) {
        return;
    }
    infoCnt_[playerIndex] = moveRecord.infoUpdateCount;
    displayedMoveNo_[playerIndex] = halfmoveNo;

    uint64_t cur = moveRecord.timeMs;

    // Who is playing comes from the game; a search info may say so as well, and then it is the
    // better answer -- but only when it says anything. A game read from a PGN and recomputed
    // move by move names no engine on its moves, and taking that empty name left the side that
    // was actually thinking as the only one without a name under its clock. Same rule as in
    // setFromHistoryMove(), for the same reason.
    const std::string* name = nullptr;
    static const std::string analyzeName = "Analyze";
    if (analyze_) {
        name = &analyzeName;
    } else if (!moveRecord.engineName_.empty()) {
        name = &moveRecord.engineName_;
    }

    if (whiteToMove_) {
        if (cur > whiteTimeCurMoveMs_) {
            whiteStopwatch_.start(now_());
        }
        whiteTimeCurMoveMs_ = cur;
        if (name != nullptr) {
            whiteEngineName_ = *name;
        }
    }
    else {
        if (cur > blackTimeCurMoveMs_) {
            blackStopwatch_.start(now_());
        }
        blackTimeCurMoveMs_ = cur;
        if (name != nullptr) {
            blackEngineName_ = *name;
        }
    }
}

void ClockModel::setStopped(bool stopped) {
    if (stopped && !stopped_) {
        const uint64_t nowMs = now_();
        whiteStopwatch_.stop(nowMs);
        blackStopwatch_.stop(nowMs);
    }
    stopped_ = stopped;
}

void ClockModel::setAnalyze(bool analyze) {
    analyze_ = analyze;
}

ClockView ClockModel::view() const {
    const uint64_t nowMs = now_();

    ClockView view;
    view.whiteToMove = whiteToMove_;
    view.analyze = analyze_;
    view.whiteEngineName = whiteEngineName_;
    view.blackEngineName = blackEngineName_;
    view.whiteLeftMs = whiteTimeLeftMs_;
    view.blackLeftMs = blackTimeLeftMs_;
    view.whiteCurMoveMs = whiteTimeCurMoveMs_ + whiteStopwatch_.elapsedMs(nowMs);
    view.blackCurMoveMs = blackTimeCurMoveMs_ + blackStopwatch_.elapsedMs(nowMs);

    std::tie(view.whiteTotal, view.whiteMove) =
        prepareTimeStrings(view.whiteLeftMs, view.whiteCurMoveMs, analyze_);
    std::tie(view.blackTotal, view.blackMove) =
        prepareTimeStrings(view.blackLeftMs, view.blackCurMoveMs, analyze_);

    return view;
}

uint64_t ClockModel::currentTimerMs() const {
    const uint64_t nowMs = now_();
    return whiteToMove_ ? whiteStopwatch_.elapsedMs(nowMs) : blackStopwatch_.elapsedMs(nowMs);
}
