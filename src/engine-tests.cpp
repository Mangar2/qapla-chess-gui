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
#include "qapla-tester/string-helper.h"
#include "qapla-tester/engine-report.h"
#include "snackbar.h"
#include "configuration.h"

using namespace QaplaWindows;

EngineTests::EngineTests()
    : state_(State::Cleared)
{
    // Initialize results table with 4 columns
    std::vector<ImGuiTable::ColumnDef> columns = {
        {"Engine", ImGuiTableColumnFlags_None, 150.0f},
        {"Status", ImGuiTableColumnFlags_None, 80.0f},
        {"Test", ImGuiTableColumnFlags_None, 200.0f},
        {"Result", ImGuiTableColumnFlags_None, 0.0f}
    };
    
    resultsTable_ = std::make_unique<ImGuiTable>(
        "EngineTestResults",
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX,
        columns
    );
    resultsTable_->setClickable(false);
    resultsTable_->setSortable(false);
    resultsTable_->setFilterable(false);
    init();
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
    resultsTable_->pop_back(); // Remove "Running" entry
    for (const auto& entry : result) {
        std::string statusText = entry.success ? "Success" : "Fail";
        resultsTable_->push({engineName, statusText, entry.testName, entry.result});
    }
}

void EngineTests::testEngineStartStop(const EngineConfig& config)
{
    // Run single start/stop test
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Start/Stop tests", ""});
    }
    addResult(config.getName(), QaplaTester::runEngineStartStopTest(config));

    // Run multiple start/stop test (20 engines in parallel)
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Multiple Start/Stop tests", ""});
    }
    addResult(config.getName(), QaplaTester::runEngineMultipleStartStopTest(config, 20));
}

void EngineTests::testHashTableMemory(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Hash table memory test", ""});
    }
    addResult(config.getName(), QaplaTester::runHashTableMemoryTest(config));
}

void EngineTests::testLowerCaseOption(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Lowercase option test", ""});
    }
    addResult(config.getName(), QaplaTester::runLowerCaseOptionTest(config));
}

void EngineTests::testEngineOptions(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Engine option tests", ""});
    }
    addResult(config.getName(), QaplaTester::runEngineOptionTests(config));
}

void EngineTests::testAnalyze(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Analyze test", ""});
    }
    addResult(config.getName(), QaplaTester::runAnalyzeTest(config));
}

void EngineTests::testImmediateStop(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Immediate stop test", ""});
    }
    addResult(config.getName(), QaplaTester::runImmediateStopTest(config));
}

void EngineTests::testInfiniteAnalyze(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Infinite analyze test", ""});
    }
    addResult(config.getName(), QaplaTester::runInfiniteAnalyzeTest(config));
}

void EngineTests::testGoLimits(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Go limits test", ""});
    }
    addResult(config.getName(), QaplaTester::runGoLimitsTest(config));
}

void EngineTests::testEpFromFen(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "EP from FEN test", ""});
    }
    addResult(config.getName(), QaplaTester::runEpFromFenTest(config));
}

void EngineTests::testComputeGame(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Compute game test", ""});
    }
    addResult(config.getName(), QaplaTester::runComputeGameTest(config, false));
}

void EngineTests::testPonder(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "UCI ponder test", ""});
    }
    addResult(config.getName(), QaplaTester::runUciPonderTest(config));
    
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Ponder game test", ""});
    }
    addResult(config.getName(), QaplaTester::runPonderGameTest(config, false));
}

void EngineTests::testEpd(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "EPD test", ""});
    }
    addResult(config.getName(), QaplaTester::runEpdTest(config));
}

void EngineTests::testMultipleGames(const EngineConfig& config)
{
    if (state_ == State::Stopping) return;
    {
        std::lock_guard<std::mutex> lock(tableMutex_);
        resultsTable_->push({config.getName(), "Running", "Multiple games test", ""});
    }
    addResult(config.getName(), QaplaTester::runMultipleGamesTest(config, 
        static_cast<uint32_t>(testSelection_.numGames), 
        static_cast<uint32_t>(testSelection_.concurrency)));
}

void EngineTests::runTestsThreaded(std::vector<EngineConfig> engineConfigs)
{
    state_ = State::Running;
    
    // Loop over all engines once for all tests
    for (const auto& config : engineConfigs) {
        if (state_ == State::Stopping) break;
        
        // Run selected tests for this engine
        if (testSelection_.testStartStop) {
            testEngineStartStop(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testHashTableMemory) {
            testHashTableMemory(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testLowerCaseOption) {
            testLowerCaseOption(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testEngineOptions) {
            testEngineOptions(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testAnalyze) {
            testAnalyze(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testImmediateStop) {
            testImmediateStop(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testInfiniteAnalyze) {
            testInfiniteAnalyze(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testGoLimits) {
            testGoLimits(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testEpFromFen) {
            testEpFromFen(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testComputeGame) {
            testComputeGame(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testPonder) {
            testPonder(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testEpd) {
            testEpd(config);
        }
        
        if (state_ == State::Stopping) break;
        if (testSelection_.testMultipleGames) {
            testMultipleGames(config);
        }
    }
    SnackbarManager::instance().showNote("Engine tests completed");
    state_ = State::Stopped;
}

void EngineTests::runTests(const std::vector<EngineConfig>& engineConfigs)
{
    if (!mayRun(true)) {
        return;
    }
    
    // Wait for previous thread to finish if it exists
    if (testThread_ && testThread_->joinable()) {
        testThread_->join();
    }
    
    // Start tests in separate thread to avoid blocking draw loop
    testThread_ = std::make_unique<std::thread>(&EngineTests::runTestsThreaded, this, engineConfigs);
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
    return resultsTable_->draw(size, true);
}

std::unique_ptr<ImGuiTable> EngineTests::createReportTable(const std::string& engineName)
{
    // Get the report data from EngineReport
    auto* checklist = EngineReport::getChecklist(engineName);
    if (!checklist) {
        return nullptr;
    }

    auto reportData = checklist->createReportData();

    // Define the table columns
    std::vector<ImGuiTable::ColumnDef> columns = {
        {"Section", ImGuiTableColumnFlags_None, 120.0f},
        {"Status", ImGuiTableColumnFlags_None, 60.0f},
        {"Topic", ImGuiTableColumnFlags_None, 0.0f},  // Auto-size
        {"Details", ImGuiTableColumnFlags_None, 100.0f}
    };

    // Create the table
    auto table = std::make_unique<ImGuiTable>(
        "EngineReport_" + engineName,
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX,
        columns
    );
    
    table->setClickable(false);
    table->setSortable(false);
    table->setFilterable(false);

    // Helper lambda to add lines from a section
    auto addSectionLines = [&table](const std::string& sectionName, const std::vector<EngineReport::ReportLine>& lines) {
        for (const auto& line : lines) {
            std::string status = line.passed ? "PASS" : "FAIL";
            std::string details = line.passed ? "" : std::to_string(line.failCount) + " failed";
            table->push({sectionName, status, line.text, details});
        }
    };

    // Add all sections
    addSectionLines("Important", reportData.important);
    addSectionLines("Missbehaviour", reportData.missbehaviour);
    addSectionLines("Notes", reportData.notes);
    addSectionLines("Report", reportData.report);

    return table;
}

void EngineTests::init() {
    auto sections = QaplaConfiguration::Configuration::instance().
        getConfigData().getSectionList("enginetest", "enginetest").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    if (sections.size() > 0) {
        auto section = sections[0];
        testSelection_.testStartStop = section.getValue("teststartstop").value_or("true") == "true";
        testSelection_.testHashTableMemory = section.getValue("testhashtablememory").value_or("true") == "true";
        testSelection_.testLowerCaseOption = section.getValue("testlowercaseoption").value_or("true") == "true";
        testSelection_.testEngineOptions = section.getValue("testengineoptions").value_or("true") == "true";
        testSelection_.testAnalyze = section.getValue("testanalyze").value_or("true") == "true";
        testSelection_.testImmediateStop = section.getValue("testimmediatestop").value_or("true") == "true";
        testSelection_.testInfiniteAnalyze = section.getValue("testinfiniteanalyze").value_or("true") == "true";
        testSelection_.testGoLimits = section.getValue("testgolimits").value_or("true") == "true";
        testSelection_.testEpFromFen = section.getValue("testepfromfen").value_or("true") == "true";
        testSelection_.testComputeGame = section.getValue("testcomputegame").value_or("true") == "true";
        testSelection_.testPonder = section.getValue("testponder").value_or("true") == "true";
        testSelection_.testEpd = section.getValue("testepd").value_or("true") == "true";
        testSelection_.testMultipleGames = section.getValue("testmultiplegames").value_or("true") == "true";
        testSelection_.numGames = QaplaHelpers::to_uint32(section.getValue("numgames").value_or("10")).value_or(10);
        testSelection_.concurrency = QaplaHelpers::to_uint32(section.getValue("concurrency").value_or("4")).value_or(4);
    }
}

void EngineTests::updateConfiguration() const {
    QaplaHelpers::IniFile::Section section {
        .name = "enginetest",
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", "enginetest"},
            {"teststartstop", testSelection_.testStartStop ? "true" : "false"},
            {"testhashtablememory", testSelection_.testHashTableMemory ? "true" : "false"},
            {"testlowercaseoption", testSelection_.testLowerCaseOption ? "true" : "false"},
            {"testengineoptions", testSelection_.testEngineOptions ? "true" : "false"},
            {"testanalyze", testSelection_.testAnalyze ? "true" : "false"},
            {"testimmediatestop", testSelection_.testImmediateStop ? "true" : "false"},
            {"testinfiniteanalyze", testSelection_.testInfiniteAnalyze ? "true" : "false"},
            {"testgolimits", testSelection_.testGoLimits ? "true" : "false"},
            {"testepfromfen", testSelection_.testEpFromFen ? "true" : "false"},
            {"testcomputegame", testSelection_.testComputeGame ? "true" : "false"},
            {"testponder", testSelection_.testPonder ? "true" : "false"},
            {"testepd", testSelection_.testEpd ? "true" : "false"},
            {"testmultiplegames", testSelection_.testMultipleGames ? "true" : "false"},
            {"numgames", std::to_string(testSelection_.numGames)},
            {"concurrency", std::to_string(testSelection_.concurrency)}
        }
    };
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("enginetest", "enginetest", { section });
}
