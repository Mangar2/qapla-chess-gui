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
#include <memory>
#include <mutex>
#include <thread>
#include "qapla-tester/engine-config.h"
#include "qapla-tester/engine-test-functions.h"
#include "imgui-table.h"

namespace QaplaWindows
{
    /**
     * @brief Singleton class for executing various engine tests
     */
    class EngineTests
    {
    public:
        /**
         * @brief Struct to hold test selection flags
         */
        struct TestSelection {
            bool testStartStop = true;
            bool testHashTableMemory = true;
            bool testLowerCaseOption = true;
            bool testEngineOptions = true;
            bool testAnalyze = true;
            bool testImmediateStop = true;
            bool testInfiniteAnalyze = true;
            bool testGoLimits = true;
            bool testEpFromFen = true;
            bool testComputeGame = true;
            bool testPonder = true;
            bool testEpd = true;
        };
    public:
        enum class State {
            Cleared,
            Running,
            Stopping,
            Stopped
        };

        /**
         * @brief Get the singleton instance
         */
        static EngineTests& instance();
        
        /**
         * @brief Run all tests on selected engines
         * @param engineConfigs Vector of engine configurations to test
         */
        void runTests(const std::vector<EngineConfig>& engineConfigs);
        
        /**
         * @brief Set the selected engines for testing
         * @param configs Vector of engine configurations
         */
        void setEngineConfigurations(const std::vector<EngineConfig>& configs);
        
        /**
         * @brief Clear all test results and reset state
         */
        void clear();
        
        /**
         * @brief Stop running tests
         */
        void stop();
        
        /**
         * @brief Get current state
         */
        State getState() const { return state_; }
        
        /**
         * @brief Get the test selection (modifiable)
         * @return Reference to test selection
         */
        TestSelection& getTestSelection() { return testSelection_; }
        
        /**
         * @brief Check if tests may run (with optional message)
         * @param sendMessage If true, shows a message if tests cannot run
         * @return True if tests may run
         */
        bool mayRun(bool sendMessage = false) const;
        
        /**
         * @brief Check if results may be cleared (with optional message)
         * @param sendMessage If true, shows a message if results cannot be cleared
         * @return True if results may be cleared
         */
        bool mayClear(bool sendMessage = false) const;
        
        /**
         * @brief Draw the results table
         * @param size Size of the table area
         * @return Optional row index if a row was clicked
         */
        std::optional<size_t> drawTable(const ImVec2& size);
        
        /**
         * @brief Update configuration to INI file
         */
        void updateConfiguration() const;

    private:
        EngineTests();
        ~EngineTests();
        EngineTests(const EngineTests&) = delete;
        EngineTests& operator=(const EngineTests&) = delete;
        
        void init();

        void addResult(const std::string& engineName, QaplaTester::TestResult result);
        void testEngineStartStop(const EngineConfig& engineConfig);
        void testHashTableMemory(const EngineConfig& engineConfig);
        void testLowerCaseOption(const EngineConfig& engineConfig);
        void testEngineOptions(const EngineConfig& engineConfig);
        void testAnalyze(const EngineConfig& engineConfig);
        void testImmediateStop(const EngineConfig& engineConfig);
        void testInfiniteAnalyze(const EngineConfig& engineConfig);
        void testGoLimits(const EngineConfig& engineConfig);
        void testEpFromFen(const EngineConfig& engineConfig);
        void testComputeGame(const EngineConfig& engineConfig);
        void testPonder(const EngineConfig& engineConfig);
        void testEpd(const EngineConfig& engineConfig);
        void runTestsThreaded(std::vector<EngineConfig> engineConfigs);
        
        std::vector<EngineConfig> engineConfigs_;
        std::unique_ptr<ImGuiTable> resultsTable_;
        State state_;
        std::mutex tableMutex_;
        std::unique_ptr<std::thread> testThread_;
        TestSelection testSelection_;
    };

} // namespace QaplaWindows

