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
#include "tournament.h"
#include "tournament-result-incremental.h"

using namespace QaplaTester;
using namespace QaplaTester::Test;
using namespace QaplaWindows;

TEST_CASE("TournamentResultIncremental polling", "[gui][tournament-result]") {
    
    SECTION("Single pair: one game then all remaining") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "EngineA"},
            {.name = "EngineB"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Single Pair Test",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 10,
            .rounds = 1,
            .repeat = 1,
            .openings = Openings{
                .file = "src/test-system/unit/test-openings.pgn",
                .plies = 1
            }
        };
        
        TournamentBuilder builder(engines, config);
        TournamentResultIncremental incremental;
        
        // Initial poll
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getTotalScheduledGames() == 10);
        REQUIRE(incremental.getPlayedGames() == 0);
        REQUIRE(incremental.hasGamesLeft());
        
        // 1. Play first game only
        builder.playGame(0, GameResult::WhiteWins);
        incremental.poll(builder.tournament, 2600.0);
        
        REQUIRE(incremental.getTotalScheduledGames() == 10);
        REQUIRE(incremental.getPlayedGames() == 1);
        REQUIRE(incremental.hasGamesLeft());
        
        auto result = incremental.getResult();
        REQUIRE(result.forEngine("EngineA")->aggregate("EngineA").total() == 1);
        REQUIRE(result.forEngine("EngineA")->aggregate("EngineA").winsEngineA == 1);
        
        // 2. Play all remaining 9 games at once
        builder.playGames(0, std::vector<GameResult>{
            GameResult::Draw, GameResult::BlackWins, GameResult::WhiteWins,
            GameResult::Draw, GameResult::WhiteWins, GameResult::BlackWins,
            GameResult::WhiteWins, GameResult::Draw, GameResult::WhiteWins
        });
        incremental.poll(builder.tournament, 2600.0);
        
        REQUIRE(incremental.getTotalScheduledGames() == 10);
        REQUIRE(incremental.getPlayedGames() == 10);
        REQUIRE(!incremental.hasGamesLeft());
        
        result = incremental.getResult();
        auto engineA = result.forEngine("EngineA");
        REQUIRE(engineA.has_value());
        auto agg = engineA->aggregate("EngineA");
        REQUIRE(agg.total() == 10);
        REQUIRE(agg.winsEngineA == 1);  // Only game 1 is a win for EngineA (white)
        REQUIRE(agg.draws == 3);        // Games 2, 5, 9
        REQUIRE(agg.winsEngineB == 6);  // EngineA loses games 3,4,6,7,8,10
    }
    
    SECTION("Multiple rounds with non-uniform progress") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Alpha"},
            {.name = "Beta"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Multi-Round Mixed Progress",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 5,
            .rounds = 4,
            .repeat = 1,
            .openings = Openings{
                .file = "src/test-system/unit/test-openings.pgn",
                .plies = 1
            }
        };
        
        TournamentBuilder builder(engines, config);
        TournamentResultIncremental incremental;
        
        // Initial poll
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getTotalScheduledGames() == 20); // 1 pair × 4 rounds × 5 games
        
        // Round 1 (pair 0): Play 1 game
        builder.playGame(0, GameResult::WhiteWins);
        
        // Round 2 (pair 1): Play 2 games
        builder.playGame(1, GameResult::Draw);
        builder.playGame(1, GameResult::BlackWins);
        
        // Round 3 (pair 2): Play 0 games (skip)
        
        // Round 4 (pair 3): Play all 5 games at once
        builder.playAllGames(3, std::vector<GameResult>{
            GameResult::WhiteWins, GameResult::Draw, GameResult::WhiteWins,
            GameResult::BlackWins, GameResult::WhiteWins
        });
        
        // Poll and verify all three key outputs
        incremental.poll(builder.tournament, 2600.0);
        
        REQUIRE(incremental.getTotalScheduledGames() == 20);
        REQUIRE(incremental.getPlayedGames() == 8); // 1 + 2 + 0 + 5
        REQUIRE(incremental.hasGamesLeft()); // Rounds 1-3 still incomplete
        
        auto result = incremental.getResult();
        auto alpha = result.forEngine("Alpha");
        REQUIRE(alpha.has_value());
        auto agg = alpha->aggregate("Alpha");
        REQUIRE(agg.total() == 8);
        REQUIRE(agg.winsEngineA == 6);  // 1 + 1 + 4 WhiteWins where Alpha is white
        REQUIRE(agg.draws == 2);        // 1 Draw from pair 1 + 1 Draw from pair 3
        REQUIRE(agg.winsEngineB == 0);  // 0 losses for Alpha
    }
    
    SECTION("Poll updates when games are added") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "EngineA"},
            {.name = "EngineB"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Incremental Test",
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
        TournamentResultIncremental incremental;
        
        // Initial poll should detect tournament setup
        bool updated = incremental.poll(builder.tournament, 2600.0);
        REQUIRE(updated);
        REQUIRE(incremental.getTotalScheduledGames() == 4);
        REQUIRE(incremental.getPlayedGames() == 0);
        REQUIRE(incremental.hasGamesLeft());
        
        // Poll again without changes - should return false
        updated = incremental.poll(builder.tournament, 2600.0);
        REQUIRE(!updated);
        
        // Play 2 games
        builder.playGame(0, GameResult::WhiteWins);
        builder.playGame(0, GameResult::Draw);
        
        // Poll should detect changes
        updated = incremental.poll(builder.tournament, 2600.0);
        REQUIRE(updated);
        REQUIRE(incremental.getPlayedGames() == 2);
        REQUIRE(incremental.hasGamesLeft());
        
        // Verify results
        auto result = incremental.getResult();
        auto engineA = result.forEngine("EngineA");
        REQUIRE(engineA.has_value());
        auto agg = engineA->aggregate("EngineA");
        REQUIRE(agg.total() == 2);
        REQUIRE(agg.winsEngineA == 1);
        REQUIRE(agg.draws == 1);
    }
    
    SECTION("Poll tracks completed and partial pair tournaments") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Main"},
            {.name = "Opponent1"},
            {.name = "Opponent2"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Multi-Pair Test",
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
        TournamentResultIncremental incremental;
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getTotalScheduledGames() == 4); // 2 pairings × 2 games
        
        // Complete first pairing fully
        builder.playGame(0, GameResult::WhiteWins);
        builder.playGame(0, GameResult::WhiteWins);
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 2);
        
        auto result1 = incremental.getResult();
        auto main1 = result1.forEngine("Main");
        REQUIRE(main1.has_value());
        REQUIRE(main1->aggregate("Main").total() == 2);
        
        // Play 1 game in second pairing (partial)
        builder.playGame(1, GameResult::Draw);
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 3);
        
        // Results should include partial pairing
        auto result2 = incremental.getResult();
        auto main2 = result2.forEngine("Main");
        REQUIRE(main2.has_value());
        REQUIRE(main2->aggregate("Main").total() == 3);
    }
    
    SECTION("Poll handles multiple rounds correctly") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Alpha"},
            {.name = "Beta"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Multi-Round Test",
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
        
        TournamentBuilder builder(engines, config);
        TournamentResultIncremental incremental;
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getTotalScheduledGames() == 6); // 1 pairing × 3 rounds × 2 games
        
        // Complete round 1
        builder.playGame(0, GameResult::WhiteWins);
        builder.playGame(0, GameResult::Draw);
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 2);
        
        // Complete round 2
        builder.playGame(1, GameResult::WhiteWins);
        builder.playGame(1, GameResult::WhiteWins);
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 4);
        
        // Partial round 3
        builder.playGame(2, GameResult::Draw);
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 5);
        REQUIRE(incremental.hasGamesLeft());
        
        auto result = incremental.getResult();
        auto alpha = result.forEngine("Alpha");
        REQUIRE(alpha.has_value());
        REQUIRE(alpha->aggregate("Alpha").total() == 5);
    }
    
    SECTION("Partial then complete pairing") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Engine1"},
            {.name = "Engine2"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Partial Complete Test",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 3,
            .rounds = 1,
            .repeat = 1,
            .openings = Openings{
                .file = "src/test-system/unit/test-openings.pgn",
                .plies = 1
            }
        };
        
        TournamentBuilder builder(engines, config);
        TournamentResultIncremental incremental;
        
        // Initial poll
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getTotalScheduledGames() == 3);
        REQUIRE(incremental.getPlayedGames() == 0);
        REQUIRE(incremental.hasGamesLeft());
        
        // Play game but pairing not finished
        builder.playGame(0, GameResult::WhiteWins);
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 1);
        REQUIRE(incremental.hasGamesLeft());
        
        // Complete the pairing
        builder.playGame(0, GameResult::Draw);
        builder.playGame(0, GameResult::WhiteWins);
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getTotalScheduledGames() == 3);
        REQUIRE(incremental.getPlayedGames() == 3);
        REQUIRE(!incremental.hasGamesLeft());
        
        auto result = incremental.getResult();
        
        // Verify TournamentResult contains both engines
        auto engineNames = result.engineNames();
        REQUIRE(engineNames.size() == 2);
        REQUIRE(std::find(engineNames.begin(), engineNames.end(), "Engine1") != engineNames.end());
        REQUIRE(std::find(engineNames.begin(), engineNames.end(), "Engine2") != engineNames.end());
        
        // Verify Engine1 results (gauntlet)
        auto eng1 = result.forEngine("Engine1");
        REQUIRE(eng1.has_value());
        auto agg1 = eng1->aggregate("Engine1");
        REQUIRE(agg1.total() == 3);
        REQUIRE(agg1.winsEngineA == 2);  // 2x WhiteWins
        REQUIRE(agg1.draws == 1);        // 1x Draw
        REQUIRE(agg1.winsEngineB == 0);
        
        // Verify Engine2 results (challenger)
        auto eng2 = result.forEngine("Engine2");
        REQUIRE(eng2.has_value());
        auto agg2 = eng2->aggregate("Engine2");
        REQUIRE(agg2.total() == 3);
        REQUIRE(agg2.winsEngineA == 0);
        REQUIRE(agg2.draws == 1);
        REQUIRE(agg2.winsEngineB == 2);
        
        // Poll again without changes - should remain stable
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 3);
        REQUIRE(!incremental.hasGamesLeft());
    }
        
    SECTION("Adding gauntlet engine preserves existing games") {
        // 1. Create tournament: 1 gauntlet (Champion) + 20 challengers, 3 rounds, 100 games/round
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Champion", .isGauntlet = true}
        });
        for (int i = 0; i < 20; i++) {
            engines.push_back(createEngine(
                TestEngineParams{.name = "Challenger" + std::to_string(i + 1)}));
        }
        
        TournamentConfig config{
            .event = "Large Tournament",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 100,
            .rounds = 3,
            .repeat = 1,
            .openings = Openings{
                .file = "src/test-system/unit/test-openings.pgn",
                .plies = 1
            }
        };
        
        TournamentBuilder builder(engines, config);
        TournamentResultIncremental incremental;
        
        // Play all 6000 games (20 pairings × 3 rounds × 100 games)
        for (size_t round = 0; round < 3; round++) {
            for (size_t pair = 0; pair < 20; pair++) {
                builder.playGames(pair + round * 20, std::vector<GameResult>(100, GameResult::WhiteWins));
            }
        }
        
        // Initial poll sees all 6000 games
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getTotalScheduledGames() == 6000);
        REQUIRE(incremental.getPlayedGames() == 6000);
        
        auto result1 = incremental.getResult();
        auto champion1 = result1.forEngine("Champion");
        REQUIRE(champion1.has_value());
        REQUIRE(champion1->aggregate("Champion").total() == 6000);
        
        // 2. Add second gauntlet engine to expand tournament
        engines.push_back(createEngine(
            TestEngineParams{.name = "ChampionNew", .isGauntlet = true}));
        
        // createTournament is state-preserving: old games remain
        builder.tournament.createTournament(engines, config);
        
        // 3. Poll after tournament expansion (modification detected)
        incremental.poll(builder.tournament, 2600.0);
        
        // New tournament structure: 2 gauntlets × 20 challengers × 3 rounds × 100 games = 12000 total
        REQUIRE(incremental.getTotalScheduledGames() == 12000);
        
        // 4. Existing 6000 games from Champion vs Challengers remain intact
        REQUIRE(incremental.getPlayedGames() == 6000);
    }
}
