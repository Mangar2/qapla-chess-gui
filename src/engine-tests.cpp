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

using namespace QaplaWindows;

EngineTests& EngineTests::instance()
{
    static EngineTests instance;
    return instance;
}

void EngineTests::testEngineStartStop(const std::vector<EngineConfig>& engineConfigs)
{
    lastResults_.clear();
    
    // Run the test for each engine configuration
    for (const auto& config : engineConfigs) {
        QaplaTester::TestResult result = QaplaTester::runEngineStartStopTest(config);
        
        // Store results
        for (const auto& [key, value] : result) {
            lastResults_.push_back({config.getName() + " - " + key, value});
        }
    }
}

void EngineTests::setEngineConfigurations(const std::vector<EngineConfig>& configs)
{
    engineConfigs_ = configs;
}

const std::vector<std::pair<std::string, std::string>>& EngineTests::getLastResults() const
{
    return lastResults_;
}

