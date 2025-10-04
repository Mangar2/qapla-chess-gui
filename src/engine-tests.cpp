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
        {"Result", ImGuiTableColumnFlags_None, 200.0f}
    };
    
    resultsTable_ = std::make_unique<ImGuiTable>(
        "EngineTestResults",
        ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
        columns
    );
    resultsTable_->setClickable(false);
    resultsTable_->setSortable(false);
    resultsTable_->setFilterable(false);
}

EngineTests& EngineTests::instance()
{
    static EngineTests instance;
    return instance;
}

void EngineTests::testEngineStartStop(const std::vector<EngineConfig>& engineConfigs)
{
    if (!mayRun(true)) {
        return;
    }
    
    state_ = State::Running;
    
    // Run the test for each engine configuration
    for (const auto& config : engineConfigs) {
        if (state_ == State::Stopping) {
            break;
        }
        
        QaplaTester::TestResult result = QaplaTester::runEngineStartStopTest(config);
        
        // Add results to table
        for (const auto& [testName, testResult] : result) {
            resultsTable_->push({config.getName(), testName, testResult});
        }
    }
    
    state_ = State::Stopped;
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
    return resultsTable_->draw(size);
}
