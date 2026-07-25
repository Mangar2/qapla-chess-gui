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
#include <functional>
#include <memory>
#include <thread>
#include <utility>

namespace QaplaLlm {

/**
 * @brief Runs a computation on a detached worker thread; the UI thread polls
 * isReady() and consume()s the result once per frame, never blocking.
 *
 * This is the standard shape for "kick off background work, pick up the
 * result later" in this module (LmStudioLocator's detection probe,
 * LlmChatController's model-list refresh and chat turns): a
 * shared_ptr<State> with an atomic "done" flag, deliberately NOT
 * std::future as returned by std::async. A still-pending std::async
 * future blocks in its destructor -- so destroying or reassigning one
 * before the task finishes (e.g. tearing down a screen, or starting a
 * second request) would stall whichever thread does that, most likely
 * the UI thread. A shared_ptr-based result never blocks: start() simply
 * replaces the pointer, the worker keeps writing into the now-orphaned
 * State, and nothing ever reads it -- no different from any other
 * shared_ptr going out of scope.
 *
 * Not copyable (each instance owns at most one in-flight computation);
 * move is fine.
 *
 * Usage:
 *   AsyncWorkerResult<Foo> task;
 *   task.start([]() { return computeFoo(); });
 *   // once per frame:
 *   if (task.isReady()) {
 *       Foo result = task.consume();
 *   }
 */
template <typename T>
class AsyncWorkerResult {
public:
    AsyncWorkerResult() = default;
    AsyncWorkerResult(const AsyncWorkerResult&) = delete;
    AsyncWorkerResult& operator=(const AsyncWorkerResult&) = delete;
    AsyncWorkerResult(AsyncWorkerResult&&) = default;
    AsyncWorkerResult& operator=(AsyncWorkerResult&&) = default;

    /**
     * @brief Starts work on a new detached thread.
     *
     * Any previous, still-pending result is orphaned (see class docs) --
     * safe to call again while isRunning(), e.g. to supersede a stale
     * request with a fresh one.
     */
    void start(std::function<T()> work) {
        auto state = std::make_shared<State>();
        state_ = state;
        std::thread([state, work = std::move(work)]() {
            state->value = work();
            state->done.store(true, std::memory_order_release);
        }).detach();
    }

    /** @brief True from start() until consume() or reset(), regardless of completion. */
    [[nodiscard]] bool isRunning() const {
        return state_ != nullptr;
    }

    /** @brief True once the worker has finished and consume() will return immediately. */
    [[nodiscard]] bool isReady() const {
        return state_ && state_->done.load(std::memory_order_acquire);
    }

    /**
     * @brief Retrieves the result and returns to the not-running state.
     * @pre isReady() (undefined behavior otherwise -- there is no result yet to move out of).
     */
    [[nodiscard]] T consume() {
        auto state = std::move(state_);
        return std::move(state->value);
    }

    /** @brief Orphans any in-flight work; isRunning()/isReady() go back to false. */
    void reset() {
        state_.reset();
    }

private:
    struct State {
        std::atomic<bool> done{false};
        T value{};
    };

    std::shared_ptr<State> state_;
};

} // namespace QaplaLlm
