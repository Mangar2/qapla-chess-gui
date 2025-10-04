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
        
        return {TestResultEntry("Error", std::string(e.what()), false)};
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
                return {TestResultEntry("Start/Stop timing", "No engine started", false)};
            }
            
            auto* engine = engineList[0].get();
            
            // Verify engine responds to UCI isReady command
            bool isReady = engine->requestReady();
            checklist->logReport("starts-and-stops-cleanly", isReady, 
                "  engine did not respond to isReady after startup in time");
            
            if (!isReady) {
                Logger::testLogger().log("Engine " + engineConfig.getName() + 
                    " did not start successfully", TraceLevel::error);
                return {TestResultEntry("Start/Stop timing", "Engine did not respond to isReady", false)};
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
        
        return {TestResultEntry("Start/Stop timing", timingInfo, true)};
    }
    catch (const std::exception& e) {
        Logger::testLogger().log("Exception during start/stop test: " + std::string(e.what()), 
            TraceLevel::error);
        return {TestResultEntry("Start/Stop timing", std::string(e.what()), false)};
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
        
        return {TestResultEntry("Parallel start/stop (" + std::to_string(numEngines) + ")", timingInfo, success)};
    }
    catch (const std::exception& e) {
        Logger::testLogger().log("Exception during multiple start/stop test: " + std::string(e.what()), 
            TraceLevel::error);
        return {TestResultEntry("Parallel start/stop (" + std::to_string(numEngines) + ")", std::string(e.what()), false)};
    }
}

TestResult runHashTableMemoryTest(const EngineConfig& engineConfig)
{
    return runTest({engineConfig}, [&engineConfig](const EngineList& engines) -> TestResult {
        if (engines.empty()) {
            return {TestResultEntry("Hash table memory test", "No engine started", false)};
        }
        
        auto* engine = engines[0].get();
        auto* checklist = EngineReport::getChecklist(engineConfig.getName());
        
        // Set Hash to 512MB and measure memory
        engine->setOption("Hash", "512");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        std::size_t memHigh = engine->getEngineMemoryUsage();
        
        // Set Hash to 16MB and measure memory
        engine->setOption("Hash", "16");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        std::size_t memLow = engine->getEngineMemoryUsage();
        
        // Verify memory shrinkage of at least 400MB
        bool success = memLow + 400'000'000 < memHigh;
        
        // Reset to default
        engine->setOption("Hash", "32");
        
        std::string resultMsg = "Usage with 512MB hash " + bytesToMB(memHigh) + 
            " MB and with 16MB hash " + bytesToMB(memLow) + " MB" +
            (success ? " (shrinked)" : " (did not shrink enough)");
        
        Logger::testLogger().logAligned("Hash table memory test:", resultMsg);
        checklist->logReport("shrinks-with-hash", success, "  " + resultMsg);
        
        return {TestResultEntry("Hash table memory test", resultMsg, success)};
    });
}

TestResult runLowerCaseOptionTest(const EngineConfig& engineConfig)
{
    return runTest({engineConfig}, [&engineConfig](const EngineList& engines) -> TestResult {
        if (engines.empty()) {
            return {TestResultEntry("Lowercase option test", "No engine started", false)};
        }
        
        auto* engine = engines[0].get();
        auto* checklist = EngineReport::getChecklist(engineConfig.getName());
        
        // Set lowercase 'hash' to 512 and measure memory
        engine->setOption("hash", "512");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        std::size_t lMem = engine->getEngineMemoryUsage();
        
        // Set uppercase 'Hash' to 512 and measure memory
        engine->setOption("Hash", "512");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        std::size_t uMem = engine->getEngineMemoryUsage();
        
        // Verify memory is similar (within 1000 bytes)
        bool success = (lMem + 1000 > uMem && lMem - 1000 < uMem);
        
        std::string resultMsg = std::string("Tried \"setoption name hash value 512\", ") +
            (success ? "lowercase option is accepted" : "lowercase option is not accepted");
        
        Logger::testLogger().logAligned("Lowercase option test:", resultMsg);
        checklist->logReport("lower-case-option", success, "  " + resultMsg);
        
        return {TestResultEntry("Lowercase option test", resultMsg, success)};
    });
}

// Helper functions for generating test values for different option types
namespace {

std::vector<std::string> generateCheckValues() {
    return { "true", "false" };
}

std::vector<std::string> generateSpinValues(const EngineOption& opt) {
    std::vector<std::string> values;
    if (opt.min && opt.max) {
        int min = *opt.min;
        int max = *opt.max;
        int mid = min + (max - min) / 2;
        values = {
            std::to_string(min),
            std::to_string(max),
            std::to_string(mid)
        };
    }
    else {
        values = { "0", "100", "-1" };
    }
    return values;
}

std::vector<std::string> generateComboValues(const EngineOption& opt) {
    std::vector<std::string> values = opt.vars;
    values.push_back("invalid_option");
    return values;
}

std::vector<std::string> generateStringValues() {
    return {
        "",
        "öäüß",               
        "C:\\invalid\\path",
        std::string(1024, 'A'), 
        "\x01\x02\x03\xFF"      
    };
}

} // anonymous namespace

// Helper function to test setting an option with proper error checking
static std::pair<bool, std::string> testSetOption(EngineWorker* engine, const std::string& name, const std::string& value) {
    bool success = engine->setOption(name, value);

    if (success) {
        return { true, "" };
    }

    bool failure = engine->failure();
    if (!failure && !engine->requestReady(std::chrono::seconds{ 10 })) {
        failure = true;
    }

    if (failure) {
        return { false, "Engine crashed or became unresponsive after setting option '" + name + "' to '" + value + "'" };
    }

    return { false, "Engine timed out after setting option '" + name + "' to '" + value + "'" };
}

TestResult runEngineOptionTests(const EngineConfig& engineConfig)
{
    return runTest({engineConfig}, [&engineConfig](const EngineList& engines) -> TestResult {
        if (engines.empty()) {
            return {TestResultEntry("Engine option tests", "No engine started", false)};
        }
        
        auto* engine = engines[0].get();
        auto* checklist = EngineReport::getChecklist(engineConfig.getName());
        const EngineOptions& options = engine->getSupportedOptions();
        
        int errors = 0;
        
        std::cout << "Randomizing engine settings, please wait...\r";
        for (const auto& opt : options) {
            if (opt.name == "Hash" || opt.type == EngineOption::Type::Button) {
                continue;
            }

            std::vector<std::string> testValues;

            switch (opt.type) {
            case EngineOption::Type::Check:
                testValues = generateCheckValues();
                break;
            case EngineOption::Type::Spin:
                testValues = generateSpinValues(opt);
                break;
            case EngineOption::Type::Combo:
                testValues = generateComboValues(opt);
                break;
            case EngineOption::Type::String:
                testValues = generateStringValues();
                break;
            default:
                continue;
            }

            for (const auto& value : testValues) {
                const std::string testName = "Option '" + opt.name + "' = '" + value + "'";
                const auto [success, message] = testSetOption(engine, opt.name, value);
                
                // Log individual test result
                checklist->logReport("options-safe", success, message);
                
                if (!success) {
                    errors++;
                    Logger::testLogger().log("Option test failed: " + testName + " - " + message, TraceLevel::error);
                }

                if (errors > 5) {
                    Logger::testLogger().log("Too many errors occurred, stopping further setoption tests.", TraceLevel::error);
                    std::string errorMsg = "Too many errors (" + std::to_string(errors) + 
                        ") after testing option values";
                    return {TestResultEntry("Engine option tests", errorMsg, false)};
                }
            }

            // Test resetting to default value
            if (!opt.defaultValue.empty()) {
                const std::string testName = "Option '" + opt.name + "' reset to default";
                const auto [success, message] = testSetOption(engine, opt.name, opt.defaultValue);
                
                // Log individual test result
                checklist->logReport("options-safe", success, message);
                
                if (!success) {
                    errors++;
                    Logger::testLogger().log("Option reset test failed: " + testName + " - " + message, TraceLevel::error);
                }

                if (errors > 5) {
                    Logger::testLogger().log("Too many errors occurred, stopping further setoption tests.", TraceLevel::error);
                    std::string errorMsg = "Too many errors (" + std::to_string(errors) + 
                        ") after testing option values";
                    return {TestResultEntry("Engine option tests", errorMsg, false)};
                }
            }
        }

        bool success = errors == 0;
        std::string resultMsg = (success ? 
            "No issues encountered." : 
            std::to_string(errors) + " failures detected. See log for details.");
        
        Logger::testLogger().logAligned("Edge case options:", resultMsg);
        
        return {TestResultEntry("Engine option tests", resultMsg, success)};
    });
}

} // namespace QaplaTester
