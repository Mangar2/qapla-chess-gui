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

#pragma once

#include <vector>
#include <string>
#include <functional>
#include "engine-config.h"
#include "engine-worker-factory.h"

namespace QaplaTester {

/**
 * @brief Function-based engine test implementations
 * 
 * This file contains test functions that can be called from various contexts.
 * Each test function takes all necessary parameters and returns results as a vector of string pairs.
 */

/**
 * @brief Result type for test functions: vector of key-value pairs describing test results
 */
using TestResult = std::vector<std::pair<std::string, std::string>>;

/**
 * @brief Runs a test with engine management
 * 
 * This function starts engines based on the provided configurations, calls the test callback
 * with the started engines, and then stops all engines. If the engine vector is empty,
 * no engines are started but the callback is still called with an empty engine list.
 * 
 * @param engineConfigs Vector of engine configurations to start
 * @param testCallback Callback function that receives the engine list and returns test result
 * @return TestResult Vector of key-value pairs with test results
 */
TestResult runTest(
    const std::vector<EngineConfig>& engineConfigs,
    const std::function<TestResult(const EngineList&)>& testCallback
);

/**
 * @brief Tests engine start and stop functionality
 * 
 * Starts an engine, measures startup time, memory usage, and shutdown time.
 * 
 * @param engineConfig Configuration for the engine to test
 * @return TestResult Vector containing timing and memory information
 */
TestResult runEngineStartStopTest(const EngineConfig& engineConfig);

/**
 * @brief Tests multiple parallel engine start and stop functionality
 * 
 * Starts multiple engines in parallel, measures startup and shutdown time.
 * 
 * @param engineConfig Configuration for the engines to test
 * @param numEngines Number of engines to start in parallel
 * @return TestResult Vector containing timing information
 */
TestResult runEngineMultipleStartStopTest(const EngineConfig& engineConfig, uint32_t numEngines);

} // namespace QaplaTester
