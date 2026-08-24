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

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

/**
 * @file
 * @brief Watches how long the UI thread goes without finishing a frame, and says what it was doing.
 *
 * A GUI that stops drawing has stopped being a GUI. It also stops answering: the tool queue is
 * drained once per frame, so a frame that takes half a minute is half a minute in which nothing
 * a caller asks for happens -- including asking it to close. Every one of those is a piece of
 * work that ought to have been handed to a thread of its own and was not.
 *
 * The point of measuring it here rather than eyeballing it: a stall that only shows up now and
 * then, on a loaded machine, is exactly the kind of thing nobody manages to reproduce on demand.
 * A number that a test can read afterwards turns it into an ordinary failing test.
 *
 * Naming the culprit is the other half. Sections mark what the thread is busy with -- the tool
 * being executed, the drawing pass, the per-frame polling -- and a long frame is reported
 * together with the longest section inside it. "43 seconds, in tool:manage_engines" is an
 * address; "the GUI hung" is not.
 */

namespace QaplaWindows {

/**
 * @brief Per-frame timing of the UI thread, readable from any thread.
 */
class UiThreadWatch {
public:
    /** @brief A frame whose work takes longer than this counts as a stall: 20 frames a second. */
    static constexpr std::chrono::milliseconds STALL_THRESHOLD{50};

    /**
     * @brief The section a frame may spend any amount of time in without counting as a stall.
     *
     * The buffer swap is where the window system parks the process on purpose: a fraction of a
     * frame for vsync, and minutes at a time when the window is occluded or the display has gone
     * to sleep. That is not this thread being blocked by work of ours, and counting it made an
     * unattended test run fail because of the screensaver -- frames of five minutes "in render".
     * Drawing itself is timed separately and still counts.
     */
    static constexpr const char* SWAP_SECTION = "swap";

    [[nodiscard]] static UiThreadWatch& instance();

    UiThreadWatch(const UiThreadWatch&) = delete;
    UiThreadWatch& operator=(const UiThreadWatch&) = delete;

    /**
     * @brief Starts timing the working part of one frame.
     *
     * Called after the frame rate limiter has finished waiting, so that the deliberate idling
     * that keeps the GUI from spinning at 500 frames a second is not counted as work.
     */
    void frameBegin();

    /** @brief Ends the frame, and records it if it took longer than STALL_THRESHOLD. */
    void frameEnd();

    /**
     * @brief Names what the UI thread is doing, for the length of its own scope.
     *
     * Nesting is allowed and the innermost one wins, which is what makes the answer useful: a
     * long frame spent inside a tool is reported as that tool, not as "drawing".
     */
    class Section {
    public:
        explicit Section(std::string name);
        ~Section();

        Section(const Section&) = delete;
        Section& operator=(const Section&) = delete;

    private:
        std::string previousName_;
        std::chrono::steady_clock::time_point started_;
        int depth_ = 0;
    };

    /** @brief What the watch has seen. Safe to read from another thread. */
    struct Report {
        std::uint64_t frames = 0;

        /** @brief How many frames took longer than STALL_THRESHOLD. */
        std::uint64_t stalls = 0;

        /** @brief The longest frame so far, in milliseconds. */
        double worstFrameMs = 0.0;

        /** @brief The longest section inside that frame -- who to talk to about it. */
        std::string worstSection;

        /** @brief How long the current frame has been running, for a caller watching live. */
        double currentFrameMs = 0.0;

        /** @brief What the UI thread is in right now, or empty between frames. */
        std::string currentSection;
    };

    [[nodiscard]] Report report() const;

    /** @brief Forgets what it has seen. For a caller that wants to measure one stretch. */
    void reset();

private:
    UiThreadWatch() = default;

    friend class Section;

    mutable std::mutex mutex_;

    std::uint64_t frames_ = 0;
    std::uint64_t stalls_ = 0;
    double worstFrameMs_ = 0.0;
    std::string worstSection_;

    std::chrono::steady_clock::time_point frameStarted_{};
    bool inFrame_ = false;

    /** @brief The longest section seen inside the frame being timed right now, and how deep. */
    double worstSectionMsThisFrame_ = 0.0;
    std::string worstSectionThisFrame_;
    int worstSectionDepthThisFrame_ = 0;

    /** @brief How many sections are open right now. */
    int depth_ = 0;

    /** @brief What the thread is in at this instant; the innermost open Section. */
    std::string currentName_;
};

} // namespace QaplaWindows
