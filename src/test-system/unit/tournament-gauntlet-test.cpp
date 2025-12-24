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
#include "tournament.h"

using namespace QaplaTester;
using namespace QaplaTester::Test;

// Note: The 'repeat' parameter controls opening repetition during gameplay,
// not the total number of games. Total games = gauntlet_engines * opponent_engines * games

TEST_CASE("Tournament gauntlet game count", "[engine-tester][tournament]") {
    
    SECTION("Gauntlet with 1 gauntlet engine and 2 opponents") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "GauntletEngine"},
            {.name = "Opponent1"},
            {.name = "Opponent2"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Test Gauntlet",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 2,
            .rounds = 1,
            .repeat = 2,
            .openings = Openings{}
        };
        
        uint32_t totalGames = Tournament::calculateTotalGames(engines, config);
        
        // Gauntlet: 1 gauntlet engine plays against 2 opponents
        // games=2 means 2 games per pairing (with color swap)
        // repeat=2 affects opening repetition, not game count
        // Expected: 1 gauntlet * 2 opponents * 2 games = 4 games
        REQUIRE(totalGames == 4);
    }
    
    SECTION("Gauntlet with 1 gauntlet engine and 3 opponents, no repeat") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "MainEngine"},
            {.name = "Challenger1"},
            {.name = "Challenger2"},
            {.name = "Challenger3"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Simple Gauntlet",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 2,
            .rounds = 1,
            .repeat = 1,
            .openings = Openings{
                .file = "src/test-system/unit/test-openings.pgn",
                .plies = 1
            }
        };
        
        Tournament tournament;
        tournament.createTournament(engines, config);
        
        // Should create 3 pair tournaments (gauntlet vs each opponent)
        REQUIRE(tournament.pairTournamentCount() == 3);
        
        // 1 gauntlet * 3 opponents * 2 games = 6 games (repeat=1 has no effect)
        uint32_t totalGames = Tournament::calculateTotalGames(engines, config);
        REQUIRE(totalGames == 6);
    }
    
    SECTION("Gauntlet with multiple rounds") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Champion"},
            {.name = "Contender1"},
            {.name = "Contender2"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Multi-Round Gauntlet",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 2,
            .rounds = 3,
            .repeat = 1,
            .openings = Openings{
                .file = "src/test-system/unit/test-openings.pgn",
                .plies = 1
            }
        };
        
        Tournament tournament;
        tournament.createTournament(engines, config);
        
        // Should create 6 pair tournaments (2 pairings × 3 rounds)
        // 1 gauntlet vs 2 opponents = 2 pairings, each played 3 times (rounds)
        REQUIRE(tournament.pairTournamentCount() == 6);
        
        // 1 gauntlet * 2 opponents * 2 games * 3 rounds = 12 games (repeat doesn't multiply)
        uint32_t totalGames = Tournament::calculateTotalGames(engines, config);
        REQUIRE(totalGames == 12);
    }
}
