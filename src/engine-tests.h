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
#include "qapla-tester/engine-config.h"

namespace QaplaWindows
{
    /**
     * @brief Singleton class for executing various engine tests
     */
    class EngineTests
    {
    public:
        /**
         * @brief Get the singleton instance
         */
        static EngineTests& instance();
        
        /**
         * @brief Test that starts and stops engines
         * This test verifies that engines can be properly started and stopped
         * @param engineConfigs Vector of engine configurations to test
         */
        void testEngineStartStop(const std::vector<EngineConfig>& engineConfigs);
        
        /**
         * @brief Set the selected engines for testing
         * @param configs Vector of engine configurations
         */
        void setEngineConfigurations(const std::vector<EngineConfig>& configs);
        
        /**
         * @brief Get the last test results
         * @return Vector of key-value pairs with test results
         */
        const std::vector<std::pair<std::string, std::string>>& getLastResults() const;

    private:
        EngineTests() = default;
        ~EngineTests() = default;
        EngineTests(const EngineTests&) = delete;
        EngineTests& operator=(const EngineTests&) = delete;
        
        std::vector<EngineConfig> engineConfigs_;
        std::vector<std::pair<std::string, std::string>> lastResults_;
    };

} // namespace QaplaWindows

