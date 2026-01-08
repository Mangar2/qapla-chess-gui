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

TEST_CASE("Tournament result stability", "[engine-tester][tournament]") {
    
    SECTION("Results remain when reducing games per round") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "EngineA"},
            {.name = "EngineB"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Stability Test",
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
        
        // Play all 4 games with specific results
        // Game 1: EngineA(white) wins, Game 2: EngineB(white) draws
        // Game 3: EngineA(white) wins, Game 4: EngineB(white) loses
        builder.playGames(0, {
            GameResult::WhiteWins,  // EngineA wins (white)
            GameResult::Draw,       // Draw
            GameResult::WhiteWins,  // EngineA wins (white again)
            GameResult::BlackWins   // EngineA wins (black)
        });
        
        // Verify we have 4 games: 3 wins for EngineA, 0 wins for EngineB, 1 draw
        auto result1 = builder.getResult();
        auto engineA1 = result1.forEngine("EngineA");
        REQUIRE(engineA1.has_value());
        auto agg1 = engineA1->aggregate("EngineA");
        REQUIRE(agg1.total() == 4);
        REQUIRE(agg1.winsEngineA == 3);
        REQUIRE(agg1.winsEngineB == 0);
        REQUIRE(agg1.draws == 1);
        
        // Verify pair tournament is finished
        auto pair1 = builder.tournament.getPairTournament(0);
        REQUIRE(pair1.has_value());
        REQUIRE((*pair1)->isFinished());
        
        // Now reduce to 2 games per round
        config.games = 2;
        builder.tournament.createTournament(engines, config);
        
        // Results should still show all 4 games (they don't disappear)
        auto result2 = builder.getResult();
        auto engineA2 = result2.forEngine("EngineA");
        REQUIRE(engineA2.has_value());
        auto agg2 = engineA2->aggregate("EngineA");
        REQUIRE(agg2.total() == 4);
        REQUIRE(agg2.winsEngineA == 3);
        REQUIRE(agg2.winsEngineB == 0);
        REQUIRE(agg2.draws == 1);
        
        // Pair tournament should now be marked as finished (4 games played, 2 required)
        auto pair2 = builder.tournament.getPairTournament(0);
        REQUIRE(pair2.has_value());
        REQUIRE((*pair2)->isFinished());
    }
    
    SECTION("Results remain stable when adding new engine") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "MainEngine"},
            {.name = "Opponent1"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Add Engine Test",
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
        
        // Play 2 games: MainEngine vs Opponent1
        builder.playGames(0, {GameResult::WhiteWins, GameResult::Draw});
        
        auto result1 = builder.getResult();
        auto main1 = result1.forEngine("MainEngine");
        REQUIRE(main1.has_value());
        auto agg1 = main1->aggregate("MainEngine");
        REQUIRE(agg1.total() == 2);
        REQUIRE(agg1.winsEngineA == 1);
        REQUIRE(agg1.draws == 1);
        
        // Pair tournament should be finished
        auto pair1 = builder.tournament.getPairTournament(0);
        REQUIRE(pair1.has_value());
        REQUIRE((*pair1)->isFinished());
        
        // Add a new opponent
        auto newEngines = createEngines(std::vector<TestEngineParams>{
            {.name = "MainEngine"},
            {.name = "Opponent1"},
            {.name = "Opponent2"}
        });
        newEngines[0].setGauntlet(true);
        
        builder.tournament.createTournament(newEngines, config);
        
        // Original results should still be there
        auto result2 = builder.getResult();
        auto main2 = result2.forEngine("MainEngine");
        REQUIRE(main2.has_value());
        auto agg2 = main2->aggregate("MainEngine");
        REQUIRE(agg2.total() == 2); // Still 2 games from original pairing
        REQUIRE(agg2.winsEngineA == 1);
        REQUIRE(agg2.draws == 1);
        
        // New pairing should have 0 games
        REQUIRE(builder.pairTournamentCount() == 2);
        
        // First pair tournament should still be finished
        auto pair2a = builder.tournament.getPairTournament(0);
        REQUIRE(pair2a.has_value());
        REQUIRE((*pair2a)->isFinished());
        
        // Second pair tournament (new opponent) should not be finished
        auto pair2b = builder.tournament.getPairTournament(1);
        REQUIRE(pair2b.has_value());
        REQUIRE(!(*pair2b)->isFinished());
    }
    
    SECTION("Results disappear when engine is removed") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "GauntletEngine"},
            {.name = "Opponent1"},
            {.name = "Opponent2"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Remove Engine Test",
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
        
        // Play games with both opponents
        builder.playGames(0, {GameResult::WhiteWins, GameResult::Draw}); // vs Opponent1
        builder.playGames(1, {GameResult::BlackWins, GameResult::WhiteWins}); // vs Opponent2
        
        auto result1 = builder.getResult();
        auto gauntlet1 = result1.forEngine("GauntletEngine");
        REQUIRE(gauntlet1.has_value());
        auto agg1 = gauntlet1->aggregate("GauntletEngine");
        REQUIRE(agg1.total() == 4); // 2 games with each opponent
        
        // Both pair tournaments should be finished
        auto pair1a = builder.tournament.getPairTournament(0);
        REQUIRE(pair1a.has_value());
        REQUIRE((*pair1a)->isFinished());
        auto pair1b = builder.tournament.getPairTournament(1);
        REQUIRE(pair1b.has_value());
        REQUIRE((*pair1b)->isFinished());
        
        // Remove Opponent2
        auto reducedEngines = createEngines(std::vector<TestEngineParams>{
            {.name = "GauntletEngine"},
            {.name = "Opponent1"}
        });
        reducedEngines[0].setGauntlet(true);
        
        builder.tournament.createTournament(reducedEngines, config);
        
        // Only results vs Opponent1 should remain
        auto result2 = builder.getResult();
        auto gauntlet2 = result2.forEngine("GauntletEngine");
        REQUIRE(gauntlet2.has_value());
        auto agg2 = gauntlet2->aggregate("GauntletEngine");
        REQUIRE(agg2.total() == 2); // Only games vs Opponent1
        REQUIRE(agg2.winsEngineA == 1);
        REQUIRE(agg2.draws == 1);
        
        // Opponent2 should not be in results anymore
        auto opponent2Result = result2.forEngine("Opponent2");
        REQUIRE(!opponent2Result.has_value());
        
        // The remaining pair tournament should still be finished
        auto pair2 = builder.tournament.getPairTournament(0);
        REQUIRE(pair2.has_value());
        REQUIRE((*pair2)->isFinished());
    }
    
    SECTION("Results of removed rounds disappear") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "ChampionEngine"},
            {.name = "ChallengerEngine"}
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
        
        // Should have 3 pair tournaments (1 pairing × 3 rounds)
        REQUIRE(builder.pairTournamentCount() == 3);
        
        // Play 2 games in each round
        builder.playGames(0, {GameResult::WhiteWins, GameResult::Draw});    // Round 1
        builder.playGames(1, {GameResult::WhiteWins, GameResult::WhiteWins}); // Round 2
        builder.playGames(2, {GameResult::Draw, GameResult::BlackWins});    // Round 3
        
        auto result1 = builder.getResult();
        auto champ1 = result1.forEngine("ChampionEngine");
        REQUIRE(champ1.has_value());
        auto agg1 = champ1->aggregate("ChampionEngine");
        REQUIRE(agg1.total() == 6); // 2 games × 3 rounds
        REQUIRE(agg1.winsEngineA == 3);
        REQUIRE(agg1.winsEngineB == 1);
        REQUIRE(agg1.draws == 2);
        
        // All three pair tournaments should be finished
        for (size_t i = 0; i < 3; ++i) {
            auto pair = builder.tournament.getPairTournament(i);
            REQUIRE(pair.has_value());
            REQUIRE((*pair)->isFinished());
        }
        
        // Reduce to 1 round
        config.rounds = 1;
        builder.tournament.createTournament(engines, config);
        
        // Should now have only 1 pair tournament
        REQUIRE(builder.pairTournamentCount() == 1);
        
        // Only results from round 1 should remain
        auto result2 = builder.getResult();
        auto champ2 = result2.forEngine("ChampionEngine");
        REQUIRE(champ2.has_value());
        auto agg2 = champ2->aggregate("ChampionEngine");
        REQUIRE(agg2.total() == 2); // Only round 1 remains
        REQUIRE(agg2.winsEngineA == 1);
        REQUIRE(agg2.draws == 1);
        REQUIRE(agg2.winsEngineB == 0);
        
        // The remaining pair tournament should still be finished
        auto pair2 = builder.tournament.getPairTournament(0);
        REQUIRE(pair2.has_value());
        REQUIRE((*pair2)->isFinished());
    }
    
    SECTION("Complex scenario: modify multiple parameters") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Alpha"},
            {.name = "Beta"},
            {.name = "Gamma"}
        });
        engines[0].setGauntlet(true);
        engines[1].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Complex Test",
            .type = "gauntlet",
            .tournamentFilename = "",
            .games = 3,
            .rounds = 2,
            .repeat = 1,
            .openings = Openings{
                .file = "src/test-system/unit/test-openings.pgn",
                .plies = 1
            }
        };
        
        TournamentBuilder builder(engines, config);
        
        // Should have 4 pairings: (Alpha vs Gamma) × 2 rounds + (Beta vs Gamma) × 2 rounds
        REQUIRE(builder.pairTournamentCount() == 4);
        
        // Play realistic sequences for each pairing
        builder.playRealisticSequence(0, 3); // Alpha vs Gamma, Round 1
        builder.playRealisticSequence(1, 3); // Alpha vs Gamma, Round 2
        builder.playRealisticSequence(2, 3); // Beta vs Gamma, Round 1
        builder.playRealisticSequence(3, 3); // Beta vs Gamma, Round 2
        
        auto result1 = builder.getResult();
        auto alpha1 = result1.forEngine("Alpha");
        REQUIRE(alpha1.has_value());
        REQUIRE(alpha1->aggregate("Alpha").total() == 6); // 3 games × 2 rounds
        
        auto beta1 = result1.forEngine("Beta");
        REQUIRE(beta1.has_value());
        REQUIRE(beta1->aggregate("Beta").total() == 6);
        
        // All four pair tournaments should be finished
        for (size_t i = 0; i < 4; ++i) {
            auto pair = builder.tournament.getPairTournament(i);
            REQUIRE(pair.has_value());
            REQUIRE((*pair)->isFinished());
        }
        
        // Now: Remove Beta, reduce rounds to 1, reduce games to 2
        auto newEngines = createEngines(std::vector<TestEngineParams>{
            {.name = "Alpha"},
            {.name = "Gamma"}
        });
        newEngines[0].setGauntlet(true);
        
        config.games = 2;
        config.rounds = 1;
        
        builder.tournament.createTournament(newEngines, config);
        
        // Should have 1 pairing now
        REQUIRE(builder.pairTournamentCount() == 1);
        
        // Only Alpha vs Gamma Round 1 results should remain (first 3 games)
        auto result2 = builder.getResult();
        auto alpha2 = result2.forEngine("Alpha");
        REQUIRE(alpha2.has_value());
        REQUIRE(alpha2->aggregate("Alpha").total() == 3); // Only Round 1
        
        // Beta should be gone
        auto beta2 = result2.forEngine("Beta");
        REQUIRE(!beta2.has_value());
        
        // The remaining pair tournament should be marked as finished (3 games played, 2 required)
        auto pair2 = builder.tournament.getPairTournament(0);
        REQUIRE(pair2.has_value());
        REQUIRE((*pair2)->isFinished());
    }
    
    SECTION("Unterminated game keeps isFinished false after adding engine") {
        auto engines = createEngines(std::vector<TestEngineParams>{
            {.name = "Gauntlet"},
            {.name = "Opponent1"}
        });
        engines[0].setGauntlet(true);
        
        TournamentConfig config{
            .event = "Unterminated Stability Test",
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
        
        // Play game 1 as unterminated
        builder.playGame(0, GameResult::Unterminated, GameEndCause::Ongoing);
        
        // Play game 2 as finished
        builder.playGame(0, GameResult::WhiteWins, GameEndCause::Checkmate);
        
        // Verify first pair tournament is not finished (1 unterminated game)
        auto pair1 = builder.tournament.getPairTournament(0);
        REQUIRE(pair1.has_value());
        REQUIRE(!(*pair1)->isFinished());
        
        // Add a third engine
        auto newEngines = createEngines(std::vector<TestEngineParams>{
            {.name = "Gauntlet"},
            {.name = "Opponent1"},
            {.name = "Opponent2"}
        });
        newEngines[0].setGauntlet(true);
        
        builder.tournament.createTournament(newEngines, config);
        
        // Should now have 2 pairings
        REQUIRE(builder.pairTournamentCount() == 2);
        
        // First pair tournament should still not be finished (unterminated game preserved)
        auto pair1AfterAdd = builder.tournament.getPairTournament(0);
        REQUIRE(pair1AfterAdd.has_value());
        REQUIRE(!(*pair1AfterAdd)->isFinished());
        
        // Second pair tournament (new) should also not be finished (no games played)
        auto pair2 = builder.tournament.getPairTournament(1);
        REQUIRE(pair2.has_value());
        REQUIRE(!(*pair2)->isFinished());
    }
}
