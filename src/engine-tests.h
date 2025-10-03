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

namespace QaplaWindows
{
    /**
     * @brief Class for executing various engine tests
     */
    class EngineTests
    {
    public:
        EngineTests() = default;
        ~EngineTests() = default;

        /**
         * @brief Test that starts and stops engines
         * This test verifies that engines can be properly started and stopped
         */
        void testEngineStartStop();

    private:
        // Future test state variables will be added here
    };

} // namespace QaplaWindows
