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

#include "engine-test-functions.h"
#include "engine-worker-factory.h"
#include "engine-report.h"
#include "logger.h"
#include "timer.h"

#include <memory>
#include <sstream>
#include <iomanip>

namespace QaplaTester {

static std::string bytesToMB(uint64_t bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / (1024.0 * 1024.0));
    return oss.str();
}

TestResult runTest(
    const std::vector<EngineConfig>& engineConfigs,
    const std::function<TestResult(const EngineList&)>& testCallback
)
{
    EngineList engines;
    
    try {
        // Start engines if configurations are provided
        if (!engineConfigs.empty()) {
            for (const auto& config : engineConfigs) {
                auto engineList = EngineWorkerFactory::createEngines(config, 1);
                for (auto& engine : engineList) {
                    engines.push_back(std::move(engine));
                }
            }
            
            // Wait for all engines to be ready
            for (const auto& engine : engines) {
                auto* checklist = EngineReport::getChecklist(engine->getEngineName());
                bool isReady = engine->requestReady();
                checklist->logReport("starts-and-stops-cleanly", isReady, 
                    "  engine did not respond to isReady after startup in time");
                if (!isReady) {
                    Logger::testLogger().log("Engine " + engine->getEngineName() + 
                        " did not start successfully", TraceLevel::error);
                }
            }
        }
        
        // Execute test callback with engine list (may be empty)
        TestResult result = testCallback(engines);
        
        // Stop all engines
        for (auto& engine : engines) {
            engine->stop();
        }
        
        return result;
    }
    catch (const std::exception& e) {
        Logger::testLogger().log("Exception during test execution: " + std::string(e.what()), 
            TraceLevel::error);
        
        // Try to stop engines on error
        for (auto& engine : engines) {
            try {
                engine->stop();
            }
            catch (...) {
                // Ignore errors during cleanup
            }
        }
        
        return {{"Error", std::string(e.what())}};
    }
}

TestResult runEngineStartStopTest(const EngineConfig& engineConfig)
{
    return runTest({engineConfig}, [&engineConfig](const EngineList& engines) -> TestResult {
        if (engines.empty()) {
            return {{"Error", "No engine started"}};
        }
        
        Timer timer;
        timer.start();
        
        auto* engine = engines[0].get();
        auto* checklist = EngineReport::getChecklist(engineConfig.getName());
        
        // Measure startup time (timer started before engine creation in runTest)
        auto startTime = timer.elapsedMs();
        
        // Get engine information
        std::string engineName = engine->getEngineName();
        std::string engineAuthor = engine->getEngineAuthor();
        uint64_t memoryInBytes = engine->getEngineMemoryUsage();
        
        // Set author in checklist
        checklist->setAuthor(engineAuthor);
        
        // Log engine information
        Logger::testLogger().logAligned("Engine startup test:",
            "Name: " + engineName + ", Author: " + engineAuthor);
        
        // Measure stop time
        timer.start();
        engine->stop();
        auto stopTime = timer.elapsedMs();
        
        // Create result
        std::string timingInfo = "Started in " + std::to_string(startTime) + " ms, shutdown in " +
            std::to_string(stopTime) + " ms, memory usage " + bytesToMB(memoryInBytes) + " MB";
        
        Logger::testLogger().logAligned("Start/Stop timing:", timingInfo);
        
        return {{"Start/Stop timing:", timingInfo}};
    });
}

} // namespace QaplaTester
