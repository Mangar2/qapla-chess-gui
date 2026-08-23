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

#include "ui-thread-watch.h"

#include <algorithm>
#include <utility>

namespace QaplaWindows {

namespace {

[[nodiscard]] double millisecondsSince(std::chrono::steady_clock::time_point start) {
    const auto elapsed = std::chrono::steady_clock::now() - start;
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

} // namespace

UiThreadWatch& UiThreadWatch::instance() {
    static UiThreadWatch instance;
    return instance;
}

void UiThreadWatch::frameBegin() {
    std::scoped_lock lock(mutex_);
    frameStarted_ = std::chrono::steady_clock::now();
    inFrame_ = true;
    worstSectionMsThisFrame_ = 0.0;
    worstSectionThisFrame_.clear();
    worstSectionDepthThisFrame_ = 0;
}

void UiThreadWatch::frameEnd() {
    std::scoped_lock lock(mutex_);
    if (!inFrame_) {
        return;
    }
    const double frameMs = millisecondsSince(frameStarted_);
    inFrame_ = false;
    ++frames_;

    const double thresholdMs =
        std::chrono::duration<double, std::milli>(STALL_THRESHOLD).count();
    if (frameMs <= thresholdMs) {
        return;
    }

    ++stalls_;
    if (frameMs > worstFrameMs_) {
        worstFrameMs_ = frameMs;
        // The longest section inside it, or the frame itself when nothing was named -- which is
        // an answer too: it means the time went on drawing rather than on anything identifiable.
        worstSection_ = worstSectionThisFrame_.empty() ? "frame" : worstSectionThisFrame_;
    }
}

UiThreadWatch::Section::Section(std::string name) : started_(std::chrono::steady_clock::now()) {
    auto& watch = UiThreadWatch::instance();
    std::scoped_lock lock(watch.mutex_);
    previousName_ = watch.currentName_;
    watch.currentName_ = std::move(name);
    depth_ = ++watch.depth_;
}

UiThreadWatch::Section::~Section() {
    const double elapsedMs = millisecondsSince(started_);
    auto& watch = UiThreadWatch::instance();
    std::scoped_lock lock(watch.mutex_);

    // Which section "explains" a long frame is not simply the longest one: an enclosing section
    // is always at least as long as what it encloses, so plain comparison would always answer
    // with the outermost -- "poll", every time, for a frame that was in fact spent inside one
    // named tool. So a section only displaces a deeper one by being *clearly* longer; when the
    // two account for much the same time, the inner one is the address worth reporting.
    constexpr double CLEARLY_LONGER = 1.05;
    const bool clearlyLonger = elapsedMs > watch.worstSectionMsThisFrame_ * CLEARLY_LONGER;
    const bool deeperAndComparable = depth_ > watch.worstSectionDepthThisFrame_
        && elapsedMs >= watch.worstSectionMsThisFrame_;
    if (clearlyLonger || deeperAndComparable) {
        watch.worstSectionMsThisFrame_ = elapsedMs;
        watch.worstSectionThisFrame_ = watch.currentName_;
        watch.worstSectionDepthThisFrame_ = depth_;
    }

    watch.currentName_ = std::move(previousName_);
    --watch.depth_;
}

UiThreadWatch::Report UiThreadWatch::report() const {
    std::scoped_lock lock(mutex_);
    Report report;
    report.frames = frames_;
    report.stalls = stalls_;
    report.worstFrameMs = worstFrameMs_;
    report.worstSection = worstSection_;
    report.currentSection = currentName_;
    // Read while the UI thread may be stuck inside the frame this is timing -- which is the one
    // moment the number matters most, so it is computed here rather than published per frame.
    report.currentFrameMs = inFrame_ ? millisecondsSince(frameStarted_) : 0.0;
    return report;
}

void UiThreadWatch::reset() {
    std::scoped_lock lock(mutex_);
    frames_ = 0;
    stalls_ = 0;
    worstFrameMs_ = 0.0;
    worstSection_.clear();
}

} // namespace QaplaWindows
