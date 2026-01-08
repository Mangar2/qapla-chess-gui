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
#include "test-system/unit/tournament-test-helpers.h"
#include <tournament/tournament.h>

using namespace QaplaTester;
using namespace QaplaTester::Test;

TEST_CASE("Tournament result", "[engine-tester][tournament]") {
    
    SECTION("Set and retrieve results for half of the games") {
        // Create a gauntlet tournament with 1 gauntlet and 2 opponents
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "GauntletEngine"},
            {.name = "Opponent1"},
            {.name = "Opponent2"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Result Test Tournament",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 4,
            .rounds = 1,
            .repeat = 1,
            .openings = Openings{
                .file = "src/test-system/unit/test-openings.pgn",
                .plies = 1
            }
        };
        
        TournamentBuilder builder(engines, config);
        
        // Verify we have 2 pair tournaments
        REQUIRE(builder.pairTournamentCount() == 2);
        
        // Play 2 games in the first pairing: 1 win, 1 draw
        builder.playGame(0, GameResult::WhiteWins, GameEndCause::Checkmate);
        builder.playGame(0, GameResult::Draw, GameEndCause::Adjudication);
        
        // Get results and verify
        auto tournamentResult = builder.getResult();
        auto engineNames = tournamentResult.engineNames();
        
        REQUIRE(!engineNames.empty());
        
        auto gauntletResult = tournamentResult.forEngine("GauntletEngine");
        REQUIRE(gauntletResult.has_value());
        
        auto aggregated = gauntletResult->aggregate("GauntletEngine");
        
        // Should have 2 completed games
        REQUIRE(aggregated.total() == 2);
        REQUIRE(aggregated.winsEngineA == 1);
        REQUIRE(aggregated.draws == 1);
        REQUIRE(aggregated.winsEngineB == 0);
    }
    
    SECTION("Results from multiple pair tournaments accumulate correctly") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Gauntlet1"},
            {.name = "Gauntlet2"},
            {.name = "Opponent"}
        });
        engines[0].setGauntlet(true);
        engines[1].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Multi-Gauntlet Test",
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
        
        TournamentBuilder builder(engines, config);
        
        REQUIRE(builder.pairTournamentCount() == 2);
        
        // Play 1 win for first gauntlet
        builder.playGame(0, GameResult::WhiteWins, GameEndCause::Checkmate);
        
        // Play 1 draw for second gauntlet
        builder.playGame(1, GameResult::Draw, GameEndCause::DrawByFiftyMoveRule);
        
        auto tournamentResult = builder.getResult();
        
        // Gauntlet1 should have 1 win
        auto gauntlet1Result = tournamentResult.forEngine("Gauntlet1");
        REQUIRE(gauntlet1Result.has_value());
        auto agg1 = gauntlet1Result->aggregate("Gauntlet1");
        REQUIRE(agg1.total() == 1);
        REQUIRE(agg1.winsEngineA == 1);
        
        // Gauntlet2 should have 1 draw
        auto gauntlet2Result = tournamentResult.forEngine("Gauntlet2");
        REQUIRE(gauntlet2Result.has_value());
        auto agg2 = gauntlet2Result->aggregate("Gauntlet2");
        REQUIRE(agg2.total() == 1);
        REQUIRE(agg2.draws == 1);
    }
    
    SECTION("Unterminated game prevents tournament from being finished") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "EngineA"},
            {.name = "EngineB"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Unterminated Game Test",
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
        
        TournamentBuilder builder(engines, config);
        
        // Play game 1 as unterminated (explicitly)
        builder.playGame(0, GameResult::Unterminated, GameEndCause::Ongoing);
        
        // Play game 2 as finished
        builder.playGame(0, GameResult::WhiteWins, GameEndCause::Checkmate);
        
        // Get pair tournament and verify it's not finished
        auto pairTournament = builder.tournament.getPairTournament(0);
        REQUIRE(pairTournament.has_value());
        REQUIRE(!(*pairTournament)->isFinished());
        
        // Verify we have results for both games
        auto result = (*pairTournament)->getResult();
        REQUIRE(result.total() == 1); // Only 1 finished game counts
    }
}
