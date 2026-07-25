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

#include <catch2/catch_test_macros.hpp>

#include "llm/async-worker-result.h"

#include <chrono>
#include <string>
#include <thread>

using namespace QaplaLlm;

namespace {
    void waitUntilReady(auto& task, std::chrono::seconds timeout = std::chrono::seconds(5)) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!task.isReady() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

TEST_CASE("AsyncWorkerResult starts not running", "[llm][async-worker-result]") {
    AsyncWorkerResult<int> task;
    REQUIRE_FALSE(task.isRunning());
    REQUIRE_FALSE(task.isReady());
}

TEST_CASE("AsyncWorkerResult delivers the worker's return value", "[llm][async-worker-result]") {
    AsyncWorkerResult<std::string> task;
    task.start([]() { return std::string("hello from worker"); });

    REQUIRE(task.isRunning());
    waitUntilReady(task);
    REQUIRE(task.isReady());

    auto result = task.consume();
    REQUIRE(result == "hello from worker");

    // consume() returns to the not-running state.
    REQUIRE_FALSE(task.isRunning());
    REQUIRE_FALSE(task.isReady());
}

TEST_CASE("AsyncWorkerResult::reset orphans an in-flight computation without blocking", "[llm][async-worker-result]") {
    AsyncWorkerResult<int> task;
    task.start([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return 42;
    });

    REQUIRE(task.isRunning());

    auto before = std::chrono::steady_clock::now();
    task.reset(); // must return immediately, not block for the worker's 200ms
    auto elapsed = std::chrono::steady_clock::now() - before;

    REQUIRE(elapsed < std::chrono::milliseconds(50));
    REQUIRE_FALSE(task.isRunning());
    REQUIRE_FALSE(task.isReady());
}

TEST_CASE("AsyncWorkerResult::start supersedes a still-pending previous computation", "[llm][async-worker-result]") {
    AsyncWorkerResult<int> task;
    task.start([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return 1;
    });
    REQUIRE(task.isRunning());

    // Starting again while the first computation is still in flight must not
    // block on it (same orphaning guarantee as reset()).
    task.start([]() { return 2; });

    waitUntilReady(task);
    REQUIRE(task.consume() == 2);
}
