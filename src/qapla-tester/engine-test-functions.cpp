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
            
            // Verify engines respond to UCI isReady command to ensure they are operational
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
        
        // Engines will be stopped automatically by their destructors when leaving scope
        
        return result;
    }
    catch (const std::exception& e) {
        Logger::testLogger().log("Exception during test execution: " + std::string(e.what()), 
            TraceLevel::error);
        
        // Engines will be stopped automatically by destructors during stack unwinding
        
        return {{"Error", std::string(e.what())}};
    }
}

TestResult runEngineStartStopTest(const EngineConfig& engineConfig)
{
    // Cannot use runTest() here because we need to measure startup time before engine is ready
    auto* checklist = EngineReport::getChecklist(engineConfig.getName());
    
    Timer timer;
    timer.start();
    uint64_t startTime = 0;
    uint64_t stopTime = 0;
    uint64_t memoryInBytes = 0;
    std::string engineName;
    std::string engineAuthor;
    
    try {
        {
            auto engineList = EngineWorkerFactory::createEngines(engineConfig, 1);
            startTime = timer.elapsedMs();
            
            if (engineList.empty()) {
                return {{"Error", "No engine started"}};
            }
            
            auto* engine = engineList[0].get();
            
            // Verify engine responds to UCI isReady command
            bool isReady = engine->requestReady();
            checklist->logReport("starts-and-stops-cleanly", isReady, 
                "  engine did not respond to isReady after startup in time");
            
            if (!isReady) {
                Logger::testLogger().log("Engine " + engineConfig.getName() + 
                    " did not start successfully", TraceLevel::error);
                return {{"Error", "Engine did not respond to isReady"}};
            }
            
            // Get engine information
            engineName = engine->getEngineName();
            engineAuthor = engine->getEngineAuthor();
            memoryInBytes = engine->getEngineMemoryUsage();
            
            checklist->setAuthor(engineAuthor);
            
            Logger::testLogger().logAligned("Engine startup test:",
                "Name: " + engineName + ", Author: " + engineAuthor);
            
            // Measure shutdown time by letting engineList go out of scope
            timer.start();
        }
        stopTime = timer.elapsedMs();
        
        std::string timingInfo = "Started in " + std::to_string(startTime) + " ms, shutdown in " +
            std::to_string(stopTime) + " ms, memory usage " + bytesToMB(memoryInBytes) + " MB";
        
        Logger::testLogger().logAligned("Start/Stop timing:", timingInfo);
        
        return {{"Start/Stop timing:", timingInfo}};
    }
    catch (const std::exception& e) {
        Logger::testLogger().log("Exception during start/stop test: " + std::string(e.what()), 
            TraceLevel::error);
        return {{"Error", std::string(e.what())}};
    }
}

TestResult runEngineMultipleStartStopTest(const EngineConfig& engineConfig, uint32_t numEngines)
{
    auto* checklist = EngineReport::getChecklist(engineConfig.getName());
    
    Timer timer;
    timer.start();
    uint64_t startTime = 0;
    uint64_t stopTime = 0;
    
    try {
        {
            EngineList engines = EngineWorkerFactory::createEngines(engineConfig, numEngines);
            startTime = timer.elapsedMs();
            
            // Engines stop automatically when leaving scope via destructors
        }
        stopTime = timer.elapsedMs();
        
        std::string timingInfo = "Started in " + std::to_string(startTime) + " ms, shutdown in " +
            std::to_string(stopTime) + " ms";
        
        Logger::testLogger().logAligned("Parallel start/stop (" + std::to_string(numEngines) + "):", timingInfo);
        
        bool success = startTime < 2000 && stopTime < 5000;
        if (!success) {
            checklist->logReport("starts-and-stops-cleanly", false,
                "  Start/Stop takes too long, started in: " + std::to_string(startTime) + 
                " ms, shutdown in " + std::to_string(stopTime) + " ms");
        }
        
        return {{"Parallel start/stop (" + std::to_string(numEngines) + "):", timingInfo}};
    }
    catch (const std::exception& e) {
        Logger::testLogger().log("Exception during multiple start/stop test: " + std::string(e.what()), 
            TraceLevel::error);
        return {{"Error", std::string(e.what())}};
    }
}

} // namespace QaplaTester
