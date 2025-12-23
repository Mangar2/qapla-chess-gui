/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include <catch2/catch_test_macros.hpp>
#include "test-system/unit/unit-test-helpers.h"

using namespace QaplaTester;
using namespace QaplaTester::Test;

TEST_CASE("Engine creation", "[engine-tester]") {
    
    SECTION("Engine with custom name and command") {
        auto engine = createEngine({.name = "Stockfish", .cmd = "/usr/bin/stockfish"});
        
        REQUIRE(engine.getName() == "Stockfish");
        REQUIRE(engine.getCmd() == "/usr/bin/stockfish");
    }
    
    SECTION("Create multiple engines with different configurations") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "FastEngine", .cmd = "fast.exe", .ponder = true},
            {.name = "SlowEngine", .cmd = "slow.exe", .ponder = false}
        });
        
        REQUIRE(engines.size() == 2);
        REQUIRE(engines[0].getName() == "FastEngine");
        REQUIRE(engines[0].getCmd() == "fast.exe");
        REQUIRE(engines[0].isPonderEnabled() == true);
        REQUIRE(engines[1].getName() == "SlowEngine");
        REQUIRE(engines[1].isPonderEnabled() == false);
    }
    
    SECTION("Engines have independent configurations") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Engine1"},
            {.name = "Engine2"},
            {.name = "Engine3"}
        });
        
        REQUIRE(engines.size() == 3);
        REQUIRE(engines[0].getName() != engines[1].getName());
        REQUIRE(engines[1].getName() != engines[2].getName());
    }
}
