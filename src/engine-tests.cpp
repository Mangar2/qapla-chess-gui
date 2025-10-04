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

#include "engine-tests.h"
#include "qapla-tester/engine-test-functions.h"
#include "snackbar.h"

using namespace QaplaWindows;

EngineTests::EngineTests()
    : state_(State::Cleared)
{
    // Initialize results table with 3 columns
    std::vector<ImGuiTable::ColumnDef> columns = {
        {"Engine", ImGuiTableColumnFlags_None, 150.0f},
        {"Test", ImGuiTableColumnFlags_None, 200.0f},
        {"Result", ImGuiTableColumnFlags_None, 0.0f}
    };
    
    resultsTable_ = std::make_unique<ImGuiTable>(
        "EngineTestResults",
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX,
        columns
    );
    resultsTable_->setClickable(false);
    resultsTable_->setSortable(false);
    resultsTable_->setFilterable(false);
}

EngineTests::~EngineTests()
{
    // Wait for test thread to finish if running
    if (testThread_ && testThread_->joinable()) {
        state_ = State::Stopping;
        testThread_->join();
    }
}

EngineTests& EngineTests::instance()
{
    static EngineTests instance;
    return instance;
}

void EngineTests::addResult(const std::string& engineName, QaplaTester::TestResult result)
{
    std::lock_guard<std::mutex> lock(tableMutex_);
    for (const auto& [testName, testResult] : result) {
        resultsTable_->push({engineName, testName, testResult});
    }
}

void EngineTests::testEngineStartStop(const EngineConfig& config)
{
    // Run single start/stop test
    if (state_ == State::Stopping) return;
    addResult(config.getName(), QaplaTester::runEngineStartStopTest(config));

    // Run multiple start/stop test (20 engines in parallel)
    if (state_ == State::Stopping) return;
    addResult(config.getName(), QaplaTester::runEngineMultipleStartStopTest(config, 20));
}

void EngineTests::testHashTableMemory(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    addResult(config.getName(), QaplaTester::runHashTableMemoryTest(config));
}

void EngineTests::testLowerCaseOption(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    addResult(config.getName(), QaplaTester::runLowerCaseOptionTest(config));
}

void EngineTests::testEngineOptions(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    addResult(config.getName(), QaplaTester::runEngineOptionTests(config));
}

void EngineTests::runTestsThreaded(std::vector<EngineConfig> engineConfigs, TestSelection testSelection)
{
    state_ = State::Running;
    
    // Loop over all engines once for all tests
    for (const auto& config : engineConfigs) {
        if (state_ == State::Stopping) break;
        
        // Run selected tests for this engine
        if (testSelection.testStartStop) {
            testEngineStartStop(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection.testHashTableMemory) {
            testHashTableMemory(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection.testLowerCaseOption) {
            testLowerCaseOption(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection.testEngineOptions) {
            testEngineOptions(config);
        }
    }
    
    state_ = State::Stopped;
}

void EngineTests::runTests(const std::vector<EngineConfig>& engineConfigs, const TestSelection& testSelection)
{
    if (!mayRun(true)) {
        return;
    }
    
    // Wait for previous thread to finish if it exists
    if (testThread_ && testThread_->joinable()) {
        testThread_->join();
    }
    
    // Start tests in separate thread to avoid blocking draw loop
    testThread_ = std::make_unique<std::thread>(&EngineTests::runTestsThreaded, this, engineConfigs, testSelection);
}

void EngineTests::setEngineConfigurations(const std::vector<EngineConfig>& configs)
{
    engineConfigs_ = configs;
}

void EngineTests::clear()
{
    if (!mayClear(true)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(tableMutex_);
    resultsTable_->clear();
    state_ = State::Cleared;
}

void EngineTests::stop()
{
    if (state_ == State::Running) {
        state_ = State::Stopping;
    }
}

bool EngineTests::mayRun(bool sendMessage) const
{
    if (state_ == State::Running) {
        if (sendMessage) {
            SnackbarManager::instance().showError("Tests already running");
        }
        return false;
    }
    
    if (state_ == State::Stopping) {
        if (sendMessage) {
            SnackbarManager::instance().showError("Tests are stopping, please wait");
        }
        return false;
    }
    
    return true;
}

bool EngineTests::mayClear(bool sendMessage) const
{
    if (state_ == State::Running) {
        if (sendMessage) {
            SnackbarManager::instance().showError("Cannot clear while tests are running");
        }
        return false;
    }
    
    if (state_ == State::Stopping) {
        if (sendMessage) {
            SnackbarManager::instance().showError("Cannot clear while tests are stopping");
        }
        return false;
    }
    
    if (state_ == State::Cleared && resultsTable_->size() == 0) {
        if (sendMessage) {
            SnackbarManager::instance().showNote("Nothing to clear");
        }
        return false;
    }
    
    return true;
}

std::optional<size_t> EngineTests::drawTable(const ImVec2& size)
{
    std::lock_guard<std::mutex> lock(tableMutex_);
    return resultsTable_->draw(size);
}
