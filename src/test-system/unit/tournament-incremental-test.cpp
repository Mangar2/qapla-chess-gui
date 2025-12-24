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
    
    SECTION("Update count increments only on actual changes") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Engine1"},
            {.name = "Engine2"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Update Count Test",
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
        uint64_t updateCount1 = incremental.getUpdateCount();
        REQUIRE(updateCount1 == 0); // No completed pairs yet
        
        // Play game but pairing not finished
        builder.playGame(0, GameResult::WhiteWins);
        incremental.poll(builder.tournament, 2600.0);
        uint64_t updateCount2 = incremental.getUpdateCount();
        REQUIRE(updateCount2 == 0); // Still no completed pairs
        
        // Complete the pairing
        builder.playGame(0, GameResult::Draw);
        builder.playGame(0, GameResult::WhiteWins);
        incremental.poll(builder.tournament, 2600.0);
        uint64_t updateCount3 = incremental.getUpdateCount();
        REQUIRE(updateCount3 == 1); // First pair completed
        
        // Poll again without changes
        incremental.poll(builder.tournament, 2600.0);
        uint64_t updateCount4 = incremental.getUpdateCount();
        REQUIRE(updateCount4 == 1); // No change
    }
    
    SECTION("Clear resets all state") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "EngineX"},
            {.name = "EngineY"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Clear Test",
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
        
        // Build up some state
        incremental.poll(builder.tournament, 2600.0);
        builder.playGame(0, GameResult::WhiteWins);
        builder.playGame(0, GameResult::Draw);
        incremental.poll(builder.tournament, 2600.0);
        
        REQUIRE(incremental.getPlayedGames() > 0);
        REQUIRE(incremental.getTotalScheduledGames() > 0);
        
        // Clear and verify
        incremental.clear();
        REQUIRE(incremental.getUpdateCount() == 0);
        REQUIRE(incremental.getPlayedGames() == 0);
        REQUIRE(incremental.getTotalScheduledGames() == 0);
        
        // Poll should rebuild state from tournament (which still has the games)
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getTotalScheduledGames() == 2);
        REQUIRE(incremental.getPlayedGames() == 2); // Games are still in the tournament
    }
    
    SECTION("Clear followed by tournament change (adding engine) - exposes bug") {
        // Build initial tournament with 2 engines, 3 rounds, 100 games per round
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
        
        // Generate all 300 games (1 pairing × 3 rounds × 100 games)
        for (size_t round = 0; round < 3; round++) {
            for (size_t pair = 0; pair < 20; pair++) {
                builder.playGames(pair + round * 20, std::vector<GameResult>(100, GameResult::WhiteWins));
            }
        }
        
        // Poll should see all games
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getTotalScheduledGames() == 6000);
        REQUIRE(incremental.getPlayedGames() == 6000);
        
        auto result1 = incremental.getResult();
        auto champion1 = result1.forEngine("Champion");
        REQUIRE(champion1.has_value());
        REQUIRE(champion1->aggregate("Champion").total() == 6000);
        
        // Clear incremental state
        incremental.clear();
        REQUIRE(incremental.getPlayedGames() == 0);
        
        // Add a third engine to the same tournament (recreate with more engines)
        engines.push_back(createEngine(
            TestEngineParams{.name = "ChampionNew", .isGauntlet = true}));
        
        // Recreate tournament with same config but more engines
        builder.tournament.createTournament(engines, config);
        
        // Poll with the modified tournament
        // BUG: TournamentResultIncremental may not correctly restore state after clear()
        // when tournament structure has changed
        incremental.poll(builder.tournament, 2600.0);
        
        // New tournament structure: 2 opponents × 3 rounds × 100 games = 600 total
        REQUIRE(incremental.getTotalScheduledGames() == 12000);
        
        // BUG EXPECTED HERE: The 300 games from Champion vs Challenger1 still exist
        // in the tournament, but incremental might not see them correctly after
        // clear() and structure change
        REQUIRE(incremental.getPlayedGames() == 6000);
    }
}
