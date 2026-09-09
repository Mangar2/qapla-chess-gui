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

#include <map>
#include <utility>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
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
    /**
     * @brief A frame whose work takes longer than this counts as a stall: ten frames a second.
     *
     * Not tighter, because honest work sits just under it: starting a CLOP run reads and parses
     * the openings file on this thread and costs 53 to 63 milliseconds, measured, every time. A
     * threshold of 50 called that a fault three runs out of three. What this is meant to catch --
     * a thread held by something that should not be on it at all -- is orders of magnitude
     * larger: the frozen GUI of 2026-08-25 sat in a single frame for 35 minutes.
     */
    static constexpr std::chrono::milliseconds DEFAULT_STALL_THRESHOLD{100};

    /**
     * @brief What counts as a stall in this session, in milliseconds.
     *
     * A property of the build, not of the application: the same work takes measurably longer in
     * an unoptimised binary, and a debug run failed a test on 64 ms spent starting a tuning run
     * -- correct arithmetic, wrong conclusion. QAPLA_STALL_THRESHOLD_MS sets it; a test runner
     * gives the debug build a number that fits it, and nothing else changes.
     */
    [[nodiscard]] static std::chrono::milliseconds stallThreshold();

    /**
     * @brief The name under which the buffer swap is waited for; see Waiting.
     *
     * The buffer swap is where the window system parks the process on purpose: a fraction of a
     * frame for vsync, and minutes at a time when the window is occluded or the display has gone
     * to sleep. That is not this thread being blocked by work of ours, and counting it made an
     * unattended test run fail because of the screensaver -- frames of five minutes "in render".
     * Drawing itself is timed separately and still counts.
     */
    static constexpr const char* SWAP_SECTION = "swap";

    /** @brief The name under which the game manager pool is waited for; see Waiting. */
    static constexpr const char* POOL_SECTION = "wait:pool";

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

    /** @brief Ends the frame, and records it if it took longer than stallThreshold(). */
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

    /**
     * @brief Marks a stretch in which the thread is waiting rather than working.
     *
     * Its time is taken out of the frame before the frame is judged. Two things are waited for
     * on purpose and neither is a fault of this thread's: the window system taking the finished
     * picture, and the game manager pool letting go -- stopping engines and waiting for the
     * games in flight is exactly the sort of thing the user interface is allowed to block on,
     * because there is nothing to draw until it is done.
     *
     * Taken out rather than excused: a frame that waited a second for the pool *and* spent
     * another second on something it should not have been doing is still a stall, and still
     * names the second thing. Excusing the whole frame -- what was done for the buffer swap
     * before -- would have hidden it.
     *
     * Nesting is allowed; only the outermost one counts, so a wait inside a wait is not
     * subtracted twice. The time is still recorded under the section's name, so the breakdown of
     * a long frame says how much of it was spent waiting for what.
     */
    class Waiting {
    public:
        explicit Waiting(std::string name);
        ~Waiting();

        Waiting(const Waiting&) = delete;
        Waiting& operator=(const Waiting&) = delete;

    private:
        Section section_;
        std::chrono::steady_clock::time_point started_;
        bool outermost_ = false;
    };

    /**
     * @brief How long one section of a frame took, on its own and with what it encloses.
     */
    struct SectionTime {
        /**
         * @brief The time of this section alone, without what ran nested inside it.
         *
         * The number that can be added up: every millisecond of a frame belongs to exactly one
         * section this way, or to none. See Report::residualMs for the check that it does.
         */
        double selfMs = 0.0;

        /** @brief The time from entering to leaving, nested sections included. */
        double totalMs = 0.0;
    };

    /** @brief One named section of a frame with its two times. */
    struct NamedSection {
        std::string name;
        SectionTime time;
    };

    /**
     * @brief One frame that took too long, with everything known about it.
     *
     * Handed to whoever asked to hear about them, the moment it is recorded -- so that it lands
     * in the output between the lines of whatever else is writing there, and the surrounding
     * lines say what was going on. A summary at the end cannot do that.
     */
    struct Stall {
        /** @brief Which frame of the run it was. */
        std::uint64_t frame = 0;

        /** @brief What the thread spent on work of its own, in milliseconds -- the judged time. */
        double frameMs = 0.0;

        /** @brief The whole frame, waiting included: frameMs + waitedMs. */
        double elapsedMs = 0.0;

        /** @brief How much of it went on waiting -- see Waiting; not part of frameMs. */
        double waitedMs = 0.0;

        /**
         * @brief How much of the frame no section claimed at all.
         *
         * The hole to look for first. Time nobody named still lands in the frame, and the frame
         * is then reported against the longest section that does have a name -- which can be a
         * short one that had nothing to do with it.
         */
        double unnamedMs = 0.0;

        /**
         * @brief What is left of the frame once every section and the unnamed time is taken off.
         *
         * The proof that the breakdown is complete. Anything but a rounding error here means the
         * bookkeeping is wrong, and then no name in it can be trusted.
         */
        double residualMs = 0.0;

        /** @brief The longest named section of the frame. */
        std::string section;

        /** @brief Every section of the frame with its times, by own time, longest first. */
        std::vector<NamedSection> sections;
    };

    /** @brief Told about every stall as it is recorded. */
    using StallCallback = std::function<void(const Stall&)>;

    /**
     * @brief Asks to be told about every stall from now on.
     * @param callback What to call; an empty one stops the reporting.
     */
    void setStallCallback(StallCallback callback);

    /** @brief What the watch has seen. Safe to read from another thread. */
    struct Report {
        std::uint64_t frames = 0;

        /** @brief How many frames took longer than STALL_THRESHOLD. */
        std::uint64_t stalls = 0;

        /**
         * @brief The longest frame so far, in milliseconds, waiting not counted.
         *
         * What the thread spent on work of its own. See Waiting for what is taken out.
         */
        double worstFrameMs = 0.0;

        /** @brief How long that frame additionally spent waiting -- see Waiting. */
        double worstFrameWaitedMs = 0.0;

        /** @brief How long every frame together has spent waiting, in milliseconds. */
        double waitedMs = 0.0;

        /** @brief How much of the worst frame no section claimed; see Stall::unnamedMs. */
        double worstFrameUnnamedMs = 0.0;

        /** @brief The longest section inside that frame -- who to talk to about it. */
        std::string worstSection;

        /**
         * @brief Every section of the worst frame with its total time, longest first.
         *
         * The one name is not always the answer. A frame can be long because one thing blocked,
         * and it can be long because six things each took a little too long -- and those two
         * have nothing in common except the number. Nested sections are each counted in full,
         * so an enclosing one includes what it encloses; the names say which is which.
         */
        std::vector<NamedSection> worstFrameSections;

        /** @brief What was left of the worst frame after everything was accounted for. */
        double worstFrameResidualMs = 0.0;

        /** @brief Every frame of the run together, waiting included. */
        double elapsedMs = 0.0;

        /** @brief Of that, what was spent on work: elapsedMs - waitedMs, the judged time. */
        double workMs = 0.0;

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
    friend class Waiting;

    mutable std::mutex mutex_;

    std::uint64_t frames_ = 0;
    std::uint64_t stalls_ = 0;
    double worstFrameMs_ = 0.0;
    double worstFrameWaitedMs_ = 0.0;
    double worstFrameUnnamedMs_ = 0.0;
    double worstFrameResidualMs_ = 0.0;
    double waitedMs_ = 0.0;
    double elapsedMs_ = 0.0;
    StallCallback stallCallback_;
    std::string worstSection_;

    std::chrono::steady_clock::time_point frameStarted_{};
    bool inFrame_ = false;

    /** @brief How much of the frame being timed right now went on waiting; see Waiting. */
    double waitedMsThisFrame_ = 0.0;

    /** @brief How many Waiting scopes are open right now; only the outermost is counted. */
    int waitDepth_ = 0;

    /** @brief How much of the frame being timed right now went by with no section open. */
    double unnamedMsThisFrame_ = 0.0;

    /** @brief When the thread last had no section open; the start of a stretch nobody claims. */
    std::chrono::steady_clock::time_point unnamedSince_{};

    /** @brief The longest section seen inside the frame being timed right now, and how deep. */
    double worstSectionMsThisFrame_ = 0.0;
    std::string worstSectionThisFrame_;
    int worstSectionDepthThisFrame_ = 0;

    /** @brief Every section of the frame being timed right now, by name, totalled. */
    std::map<std::string, SectionTime> sectionsThisFrame_;

    /** @brief The same, kept from the worst frame so far. */
    std::vector<NamedSection> worstFrameSections_;

    /**
     * @brief One entry per open section: how much of it has gone on sections nested inside it.
     *
     * What makes a section's own time computable, and with it the check that every millisecond
     * of a frame is accounted for exactly once.
     */
    std::vector<double> childMsStack_;

    /** @brief How many sections are open right now. */
    int depth_ = 0;

    /** @brief What the thread is in at this instant; the innermost open Section. */
    std::string currentName_;
};

} // namespace QaplaWindows
