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

#include "activity-watch.h"

namespace QaplaLlm {

namespace {
    /** @brief Every state other than Idle counts as "there is a run to wait for". */
    [[nodiscard]] bool isBusy(Actions::RunState state) {
        return state != Actions::RunState::Idle;
    }
} // namespace

const char* waitReasonName(WaitReason reason) {
    switch (reason) {
        case WaitReason::Finished: return "finished";
        case WaitReason::Stopped: return "stopped";
        case WaitReason::Timeout: return "timeout";
        case WaitReason::Closed: return "closed";
        case WaitReason::NotRunning:
        default: return "not_running";
    }
}

ActivityWatch& ActivityWatch::instance() {
    static ActivityWatch watch;
    return watch;
}

std::size_t ActivityWatch::indexOf(Actions::Activity activity) {
    const auto index = static_cast<std::size_t>(activity);
    // The enum's own order is the index, so adding an activity needs no second list to update --
    // the one thing that has to keep pace is ACTIVITY_COUNT, and this guards it.
    if (index >= ACTIVITY_COUNT) {
        return 0;
    }
    return index;
}

void ActivityWatch::update(Actions::Activity activity, Actions::ActivityProgress progress) {
    bool notify = false;
    {
        std::scoped_lock lock(mutex_);
        auto& stored = activities_[indexOf(activity)];
        if (stored.progress.state == progress.state
            && stored.progress.finished == progress.finished) {
            return;
        }

        if (!isBusy(stored.progress.state) && isBusy(progress.state)) {
            ++stored.run;
            stored.lastEnding = WaitReason::NotRunning;
        } else if (isBusy(stored.progress.state) && !isBusy(progress.state)) {
            // The distinction the whole wait exists to report. A run that reached its own
            // conclusion and one that was cut short leave the same state behind, and a caller
            // that cannot tell them apart learns nothing from being woken.
            stored.lastEnding = progress.finished ? WaitReason::Finished : WaitReason::Stopped;
        }

        stored.progress = progress;
        ++stored.revision;
        notify = true;
    }
    if (notify) {
        changed_.notify_all();
    }
}

ActivitySnapshot ActivityWatch::snapshot(Actions::Activity activity) const {
    std::scoped_lock lock(mutex_);
    return activities_[indexOf(activity)];
}

ActivityWatch::WaitResult ActivityWatch::waitUntilIdle(
    Actions::Activity activity, std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    const auto index = indexOf(activity);
    const auto startedGeneration = waitGeneration_;

    if (!isBusy(activities_[index].progress.state)) {
        return WaitResult{.reason = WaitReason::NotRunning, .snapshot = activities_[index]};
    }

    // Woken on every observed change rather than only on the one being waited for: the predicate
    // decides, so a run that passes through Starting or GracefulStopping on its way out costs a
    // wakeup and nothing more.
    const bool settled = changed_.wait_for(lock, timeout, [&]() {
        return !isBusy(activities_[index].progress.state) || waitGeneration_ != startedGeneration;
    });

    auto snapshot = activities_[index];
    if (waitGeneration_ != startedGeneration) {
        return WaitResult{.reason = WaitReason::Closed, .snapshot = snapshot};
    }
    if (!settled) {
        return WaitResult{.reason = WaitReason::Timeout, .snapshot = snapshot};
    }
    return WaitResult{.reason = snapshot.lastEnding, .snapshot = snapshot};
}

void ActivityWatch::cancelWaits() {
    {
        std::scoped_lock lock(mutex_);
        ++waitGeneration_;
    }
    changed_.notify_all();
}

} // namespace QaplaLlm
