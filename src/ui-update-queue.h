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

#include <functional>
#include <mutex>
#include <vector>

/**
 * @file
 * @brief How work done on another thread gets its results into the GUI's data.
 *
 * The rule this exists to serve: the GUI's data is changed in the frame loop and nowhere else.
 * Not because changing it is slow -- it is a handful of assignments -- but because that is the
 * thread that reads it, every frame, to draw. One writer, no locks, nothing to get wrong.
 *
 * Work, on the other hand, does not belong there: starting engines, waiting for them, anything
 * that takes longer than a frame is somebody else's thread. What comes back is a change, and a
 * change goes through here.
 *
 * So the shape of an asynchronous operation in this application is:
 *
 *     [its own thread]   do the work, gather the result
 *     post(...)        ->  [frame loop]  write it into the data
 *
 * What must NOT go in here is an action. A change posted from here runs inside the frame, so
 * anything that waits, spawns or blocks would stop the window drawing -- which is the thing this
 * was built to prevent.
 */

namespace QaplaWindows {

/**
 * @brief Changes waiting to be applied in the frame loop.
 */
class UiUpdateQueue {
public:
    [[nodiscard]] static UiUpdateQueue& instance();

    UiUpdateQueue(const UiUpdateQueue&) = delete;
    UiUpdateQueue& operator=(const UiUpdateQueue&) = delete;

    /**
     * @brief Hands one change over to the frame loop. Callable from any thread.
     *
     * Returns at once; the change happens on one of the next frames. A caller that has to know
     * when waits for whatever the work itself reports as finished -- and must not be the frame
     * loop, which would then be waiting for itself.
     */
    void post(std::function<void()> change);

    /**
     * @brief Applies everything posted so far. Called once per frame, on the UI thread.
     *
     * A change that posts another one is fine: the new one waits for the next frame rather than
     * extending this one.
     */
    void applyAll();

    /** @brief Whether anything is waiting. */
    [[nodiscard]] bool empty() const;

private:
    UiUpdateQueue() = default;

    mutable std::mutex mutex_;
    std::vector<std::function<void()>> pending_;
};

} // namespace QaplaWindows
