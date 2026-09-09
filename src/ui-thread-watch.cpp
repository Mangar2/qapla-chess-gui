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

#include "os-helpers.h"

#include <base-elements/string-helper.h>

#include <algorithm>
#include <utility>

namespace QaplaWindows {

namespace {

[[nodiscard]] double millisecondsSince(std::chrono::steady_clock::time_point start) {
    const auto elapsed = std::chrono::steady_clock::now() - start;
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

} // namespace

std::chrono::milliseconds UiThreadWatch::stallThreshold() {
    // Read once: it belongs to the run, and reading the environment per frame would itself cost
    // more than the thing being measured.
    static const std::chrono::milliseconds threshold = [] {
        const auto configured = QaplaHelpers::OsHelpers::getEnv("QAPLA_STALL_THRESHOLD_MS");
        if (!configured) {
            return DEFAULT_STALL_THRESHOLD;
        }
        const auto milliseconds = QaplaHelpers::to_uint32(*configured);
        if (!milliseconds || *milliseconds == 0) {
            return DEFAULT_STALL_THRESHOLD;
        }
        return std::chrono::milliseconds{*milliseconds};
    }();
    return threshold;
}

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
    waitedMsThisFrame_ = 0.0;
    unnamedMsThisFrame_ = 0.0;
    unnamedSince_ = frameStarted_;
    sectionsThisFrame_.clear();
    childMsStack_.clear();
}

void UiThreadWatch::setStallCallback(StallCallback callback) {
    std::scoped_lock lock(mutex_);
    stallCallback_ = std::move(callback);
}

void UiThreadWatch::frameEnd() {
    Stall stall;
    StallCallback callback;
    {
    std::scoped_lock lock(mutex_);
    if (!inFrame_) {
        return;
    }
    const double elapsedMs = millisecondsSince(frameStarted_);
    inFrame_ = false;
    ++frames_;

    if (depth_ == 0) {
        // The tail of the frame, after the last section closed: nobody named it either.
        unnamedMsThisFrame_ += millisecondsSince(unnamedSince_);
    }

    // What the thread spent on its own work: waiting for the window system or for the pool is
    // taken out rather than excusing the whole frame -- see Waiting.
    const double waitedMs = std::min(waitedMsThisFrame_, elapsedMs);
    const double frameMs = elapsedMs - waitedMs;
    waitedMs_ += waitedMs;
    elapsedMs_ += elapsedMs;

    // Every millisecond of the frame belongs to exactly one section or to none of them, so the
    // two must add up to the frame. What is left over says the bookkeeping is wrong -- and then
    // no name in the breakdown is worth anything.
    double accountedMs = unnamedMsThisFrame_;
    for (const auto& [name, time] : sectionsThisFrame_) {
        accountedMs += time.selfMs;
    }
    const double residualMs = elapsedMs - accountedMs;

    const double thresholdMs =
        std::chrono::duration<double, std::milli>(stallThreshold()).count();
    if (frameMs <= thresholdMs) {
        return;
    }

    ++stalls_;

    stall.frame = frames_;
    stall.frameMs = frameMs;
    stall.elapsedMs = elapsedMs;
    stall.waitedMs = waitedMs;
    stall.unnamedMs = unnamedMsThisFrame_;
    stall.residualMs = residualMs;
    // The longest section inside it, or the frame itself when nothing was named -- which is
    // an answer too: it means the time went on drawing rather than on anything identifiable.
    stall.section = worstSectionThisFrame_.empty() ? "frame" : worstSectionThisFrame_;
    // And the whole frame broken down, because one name explains a frame that one thing
    // blocked, and explains nothing about a frame in which six things were each too slow.
    stall.sections.reserve(sectionsThisFrame_.size());
    for (const auto& [name, time] : sectionsThisFrame_) {
        stall.sections.push_back(NamedSection{ .name = name, .time = time });
    }
    // By own time: that is the one that says who spent it, rather than who was around it.
    std::ranges::sort(stall.sections, [](const auto& left, const auto& right) {
        return left.time.selfMs > right.time.selfMs;
    });

    if (frameMs > worstFrameMs_) {
        worstFrameMs_ = frameMs;
        worstFrameWaitedMs_ = waitedMs;
        worstFrameUnnamedMs_ = unnamedMsThisFrame_;
        worstFrameResidualMs_ = residualMs;
        worstSection_ = stall.section;
        worstFrameSections_ = stall.sections;
    }
    callback = stallCallback_;
    }
    // Outside the lock: whoever is told about this may well ask the watch something in return.
    if (callback) {
        callback(stall);
    }
}

UiThreadWatch::Section::Section(std::string name) : started_(std::chrono::steady_clock::now()) {
    auto& watch = UiThreadWatch::instance();
    std::scoped_lock lock(watch.mutex_);
    if (watch.depth_ == 0 && watch.inFrame_) {
        // The stretch since the last section closed belonged to nobody; it ends here.
        watch.unnamedMsThisFrame_ += millisecondsSince(watch.unnamedSince_);
    }
    previousName_ = watch.currentName_;
    watch.currentName_ = std::move(name);
    depth_ = ++watch.depth_;
    watch.childMsStack_.push_back(0.0);
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

    // What this section spent on itself: its own time less everything that ran nested in it.
    // The enclosing section, if there is one, takes the whole of this one as its child time.
    double childMs = 0.0;
    if (!watch.childMsStack_.empty()) {
        childMs = watch.childMsStack_.back();
        watch.childMsStack_.pop_back();
    }
    if (!watch.childMsStack_.empty()) {
        watch.childMsStack_.back() += elapsedMs;
    }
    auto& time = watch.sectionsThisFrame_[watch.currentName_];
    time.selfMs += std::max(0.0, elapsedMs - childMs);
    time.totalMs += elapsedMs;
    if (clearlyLonger || deeperAndComparable) {
        watch.worstSectionMsThisFrame_ = elapsedMs;
        watch.worstSectionThisFrame_ = watch.currentName_;
        watch.worstSectionDepthThisFrame_ = depth_;
    }

    watch.currentName_ = std::move(previousName_);
    --watch.depth_;
    if (watch.depth_ == 0) {
        watch.unnamedSince_ = std::chrono::steady_clock::now();
    }
}

UiThreadWatch::Waiting::Waiting(std::string name)
    : section_(std::move(name)), started_(std::chrono::steady_clock::now()) {
    auto& watch = UiThreadWatch::instance();
    std::scoped_lock lock(watch.mutex_);
    // Only the outermost wait is measured: a wait inside a wait is the same time, and counting
    // it twice could take more out of the frame than the frame lasted.
    outermost_ = watch.waitDepth_ == 0;
    ++watch.waitDepth_;
}

UiThreadWatch::Waiting::~Waiting() {
    const double elapsedMs = millisecondsSince(started_);
    {
        auto& watch = UiThreadWatch::instance();
        std::scoped_lock lock(watch.mutex_);
        --watch.waitDepth_;
        if (outermost_) {
            watch.waitedMsThisFrame_ += elapsedMs;
        }
    }
    // section_ is destroyed after this body, and records the same stretch under its name -- so
    // the breakdown of a long frame still says how much of it went on waiting for what.
}

UiThreadWatch::Report UiThreadWatch::report() const {
    std::scoped_lock lock(mutex_);
    Report report;
    report.frames = frames_;
    report.stalls = stalls_;
    report.worstFrameMs = worstFrameMs_;
    report.worstFrameWaitedMs = worstFrameWaitedMs_;
    report.worstFrameUnnamedMs = worstFrameUnnamedMs_;
    report.worstFrameResidualMs = worstFrameResidualMs_;
    report.waitedMs = waitedMs_;
    report.elapsedMs = elapsedMs_;
    report.workMs = elapsedMs_ - waitedMs_;
    report.worstSection = worstSection_;
    report.worstFrameSections = worstFrameSections_;
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
    worstFrameWaitedMs_ = 0.0;
    worstFrameUnnamedMs_ = 0.0;
    worstFrameResidualMs_ = 0.0;
    waitedMs_ = 0.0;
    elapsedMs_ = 0.0;
    worstFrameSections_.clear();
    worstSection_.clear();
}

} // namespace QaplaWindows
