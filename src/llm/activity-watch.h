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

#include "actions/gui-action-activity.h"
#include "actions/gui-action-types.h"

#include <chrono>
#include <array>
#include <condition_variable>
#include <cstdint>
#include <mutex>

/**
 * @file
 * @brief Lets a caller wait for a run to end instead of asking again and again.
 *
 * This is the one thing the GUI could not do for an outside caller at all. Everything else the
 * remote control needs was already there in some form; "tell me when the SPRT is through" was
 * not, and polling for it is both expensive and imprecise -- a caller either asks too often or
 * finds out too late.
 *
 * The shape is deliberately the plainest one that works: a call that does not answer until there
 * is something to say. It needs no callback address, no subscription, and no knowledge on the
 * GUI's side of who is listening. On the caller's side it lands on the mechanism that is already
 * there -- a background process that ends, which is exactly how a run of qapla-engine-tester
 * announces itself finished today (see docs/grobplan-clop-cli-http.md, F.5).
 *
 * Free of the GUI stack on purpose: it is fed with plain values from the frame loop (see
 * initializeLlmChat) rather than reading the data singletons itself, so the waiting -- the part
 * that is genuinely concurrent and worth testing on its own -- can be tested without a window.
 */

namespace QaplaLlm {

/** @brief Why a wait ended. */
enum class WaitReason {
    /** @brief The run reached its own conclusion: a decision, or all its work done. */
    Finished,

    /** @brief The run ended without reaching it -- stopped, by whoever. */
    Stopped,

    /** @brief The caller's time limit ran out with the run still going. */
    Timeout,

    /** @brief The remote control was ended, or the application is shutting down. */
    Closed,

    /** @brief Nothing was running when the wait began; there was nothing to wait for. */
    NotRunning
};

/** @brief Names a WaitReason for a caller ("finished", "stopped", …). */
[[nodiscard]] const char* waitReasonName(WaitReason reason);

/** @brief What is known about one activity right now. */
struct ActivitySnapshot {
    Actions::ActivityProgress progress;

    /**
     * @brief Counts runs: bumped every time this activity starts. Lets a caller tell the run it
     * asked about from the next one, which matters as soon as a person can start one by hand.
     */
    uint64_t run = 0;

    /** @brief Bumped on every observed change, so a caller can detect one without comparing. */
    uint64_t revision = 0;

    /** @brief How the most recent run ended; Timeout/Closed never appear here. */
    WaitReason lastEnding = WaitReason::NotRunning;
};

/**
 * @brief Tracks the three activities and hands out waits on them.
 *
 * Fed from the UI thread, waited on from any other. Waits on different activities are
 * independent, and any number may be outstanding at once.
 */
class ActivityWatch {
public:
    [[nodiscard]] static ActivityWatch& instance();

    ActivityWatch(const ActivityWatch&) = delete;
    ActivityWatch& operator=(const ActivityWatch&) = delete;

    /** @brief Feeds in what an activity is doing. Call once per frame, from the UI thread. */
    void update(Actions::Activity activity, Actions::ActivityProgress progress);

    [[nodiscard]] ActivitySnapshot snapshot(Actions::Activity activity) const;

    /**
     * @brief Blocks until the activity is no longer running, or the time limit passes.
     *
     * Returns at once if it is not running to begin with -- there is nothing to wait for, and
     * saying so immediately is better than making the caller wait to find that out.
     *
     * @param timeout The caller's own limit. It has to come from the caller: only the caller
     *        knows how long its own patience is worth, and a limit generous enough to be
     *        invisible makes a hung wait indistinguishable from a working one.
     * @return Why the wait ended, and the state as it then stood.
     */
    struct WaitResult {
        WaitReason reason = WaitReason::Timeout;
        ActivitySnapshot snapshot;
    };
    [[nodiscard]] WaitResult waitUntilIdle(
        Actions::Activity activity, std::chrono::milliseconds timeout);

    /**
     * @brief Ends every outstanding wait with WaitReason::Closed.
     *
     * Called when the remote control is switched off or the application is going down. A waiter
     * that simply stopped being answered would look exactly like a run that is taking its time,
     * so the channel closing has to be said out loud.
     */
    void cancelWaits();

private:
    /** @brief How many Activity values there are. Update alongside Actions::Activity. */
    static constexpr std::size_t ACTIVITY_COUNT =
        static_cast<std::size_t>(Actions::Activity::Clop) + 1;

    ActivityWatch() = default;

    [[nodiscard]] static std::size_t indexOf(Actions::Activity activity);

    mutable std::mutex mutex_;
    std::condition_variable changed_;

    /** @brief Bumped by cancelWaits(); a wait started under an older value gives up. */
    uint64_t waitGeneration_ = 0;

    /**
     * @brief One slot per Activity, sized from the enum rather than by hand.
     *
     * It was a bare "[3]" until a fourth activity was added, and indexOf() then wrote past the
     * end -- silently, since nothing checks. The counters a caller reads back came out as
     * nonsense and a wait answered "nothing is running" about a run in full swing. Deriving the
     * size means the next activity cannot repeat it: the array grows with the enum, and the
     * static_assert in indexOf() catches a value nobody mapped.
     */
    std::array<ActivitySnapshot, ACTIVITY_COUNT> activities_{};
};

} // namespace QaplaLlm
