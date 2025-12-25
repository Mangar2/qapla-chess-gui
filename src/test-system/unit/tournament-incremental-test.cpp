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
        // Only works, because we only have one pair tournament here
        REQUIRE(incremental.getResult().results() == builder.tournament.getResult().results());
        
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
        // Only works, because we only have one pair tournament here
        REQUIRE(incremental.getResult().results() == builder.tournament.getResult().results());
    }
    
    SECTION("Poll returns false when no changes occur") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "EngineX"},
            {.name = "EngineY"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Idempotent Polling Test",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 5,
            .rounds = 1,
            .repeat = 1,
            .openings = Openings{
                .file = "src/test-system/unit/test-openings.pgn",
                .plies = 1
            }
        };
        
        TournamentBuilder builder(engines, config);
        TournamentResultIncremental incremental;
        
        // First poll detects new tournament
        bool updated = incremental.poll(builder.tournament, 2600.0);
        REQUIRE(updated);
        REQUIRE(incremental.getTotalScheduledGames() == 5);
        REQUIRE(incremental.getPlayedGames() == 0);
        
        // Second poll without changes should return false
        updated = incremental.poll(builder.tournament, 2600.0);
        REQUIRE(!updated);
        
        // Third poll without changes should still return false
        updated = incremental.poll(builder.tournament, 2600.0);
        REQUIRE(!updated);
        
        // Play games to trigger change
        builder.playGame(0, GameResult::WhiteWins);
        builder.playGame(0, GameResult::Draw);
        
        // Poll detects new games
        updated = incremental.poll(builder.tournament, 2600.0);
        REQUIRE(updated);
        REQUIRE(incremental.getPlayedGames() == 2);
        
        // Poll again without changes should return false
        updated = incremental.poll(builder.tournament, 2600.0);
        REQUIRE(!updated);
        
        // Play remaining games
        builder.playGame(0, GameResult::WhiteWins);
        builder.playGame(0, GameResult::BlackWins);
        builder.playGame(0, GameResult::Draw);
        
        // Poll detects completion
        updated = incremental.poll(builder.tournament, 2600.0);
        REQUIRE(updated);
        REQUIRE(incremental.getPlayedGames() == 5);
        REQUIRE(!incremental.hasGamesLeft());
        
        // Final poll without changes should return false
        updated = incremental.poll(builder.tournament, 2600.0);
        REQUIRE(!updated);
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
        
        auto incResult = incremental.getResult();
        auto tourResult = builder.tournament.getResult();
        REQUIRE(incResult.forEngine("Alpha") == tourResult.forEngine("Alpha"));
        REQUIRE(incResult.forEngine("Beta") == tourResult.forEngine("Beta"));
    }
    
    SECTION("Partial pair transitions to finished correctly") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Main"},
            {.name = "Opponent1"},
            {.name = "Opponent2"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Partial to Finished Test",
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
        REQUIRE(incremental.getTotalScheduledGames() == 20); // 2 pairings × 10 games
        REQUIRE(incremental.getPlayedGames() == 0);
        
        // Pair 0: Play 3 games (partial)
        builder.playGame(0, GameResult::WhiteWins);
        builder.playGame(0, GameResult::Draw);
        builder.playGame(0, GameResult::WhiteWins);
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 3);
        REQUIRE(incremental.hasGamesLeft());
        
        auto result1 = incremental.getResult();
        auto main1 = result1.forEngine("Main");
        REQUIRE(main1.has_value());
        REQUIRE(main1->aggregate("Main").total() == 3);
        
        // Pair 0: Complete remaining 7 games (partial → finished)
        builder.playGames(0, std::vector<GameResult>{
            GameResult::BlackWins, GameResult::WhiteWins, GameResult::Draw,
            GameResult::WhiteWins, GameResult::Draw, GameResult::WhiteWins,
            GameResult::BlackWins
        });
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 10);
        REQUIRE(incremental.hasGamesLeft()); // Pair 1 still pending
        
        auto result2 = incremental.getResult();
        auto main2 = result2.forEngine("Main");
        REQUIRE(main2.has_value());
        REQUIRE(main2->aggregate("Main").total() == 10);
        
        // Pair 1: Start playing (verify completed pair stays stable)
        builder.playGame(1, GameResult::WhiteWins);
        builder.playGame(1, GameResult::Draw);
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 12); // 10 from pair 0 + 2 from pair 1
        REQUIRE(incremental.hasGamesLeft());

        auto incResult = incremental.getResult();
        auto tourResult = builder.tournament.getResult();
        REQUIRE(incResult.forEngine("Main") == tourResult.forEngine("Main"));
        REQUIRE(incResult.forEngine("Opponent1") == tourResult.forEngine("Opponent1"));
        REQUIRE(incResult.forEngine("Opponent2") == tourResult.forEngine("Opponent2"));
    }
    
    SECTION("ExtraChecks mechanism limits polling of empty rounds") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Alpha"},
            {.name = "Beta"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "ExtraChecks Test",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 2,
            .rounds = 20,
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
        REQUIRE(incremental.getTotalScheduledGames() == 40); // 20 rounds × 2 games
        REQUIRE(incremental.getPlayedGames() == 0);
        
        // Step 1: Play round 15 (pairTournamentIndex 14) - should NOT be visible due to 10+ empty rounds before it
        builder.playGame(14, GameResult::WhiteWins);
        builder.playGame(14, GameResult::Draw);
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 0); // Round 15 not counted (10 empty rounds block it)
        
        // Step 2: Play rounds 2-5 (pairTournamentIndex 1-4) - should be visible, but round 15 still blocked
        for (size_t pairIdx = 1; pairIdx <= 4; ++pairIdx) {
            builder.playGame(pairIdx, GameResult::WhiteWins);
            builder.playGame(pairIdx, GameResult::Draw);
        }
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 8); // Only rounds 2-5 (4 × 2 = 8 games)
        
        // Step 3: Play round 6 (pairTournamentIndex 5) - now round 15 should become visible (only 9 empty rounds between)
        builder.playGame(5, GameResult::WhiteWins);
        builder.playGame(5, GameResult::Draw);
        
        incremental.poll(builder.tournament, 2600.0);
        REQUIRE(incremental.getPlayedGames() == 12); // Rounds 2-6 + round 15 (6 × 2 = 12 games)

        auto incResult = incremental.getResult();
        auto tourResult = builder.tournament.getResult();
        REQUIRE(incResult.forEngine("Alpha") == tourResult.forEngine("Alpha"));
        REQUIRE(incResult.forEngine("Beta") == tourResult.forEngine("Beta"));
        
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

        auto incResult = incremental.getResult();
        auto tourResult = builder.tournament.getResult();
        REQUIRE(incResult.forEngine("Champion") == tourResult.forEngine("Champion"));
        
        // 5. Play first game for ChampionNew in round 1 (pair 20: first pair of new gauntlet)
        // Note: all games of round 1 are scheduled before round 2 starts
        builder.playGame(20, GameResult::WhiteWins);
        
        // Poll after adding new game
        incremental.poll(builder.tournament, 2600.0);
        
        // Total played games should now be 6001
        REQUIRE(incremental.getPlayedGames() == 6001);
        
        // Verify Champion's games are still intact after adding ChampionNew game
        auto incResult2 = incremental.getResult();
        auto tourResult2 = builder.tournament.getResult();
        REQUIRE(incResult2.forEngine("Champion") == tourResult2.forEngine("Champion"));

    }
}
