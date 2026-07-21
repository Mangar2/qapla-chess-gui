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

#include <catch2/catch_test_macros.hpp>

#include "config-group-loader.h"
#include "tournament-config-sections.h"

#include <tournament/tournament-config.h>
#include <sprt/sprt-config.h>
#include <config/opening-config.h>
#include <config/pgn-config.h>
#include <config/adjudication-config.h>

using namespace QaplaConfiguration;

TEST_CASE("Tournament config round-trips through Settings::Manager", "[config-persistence]") {
    QaplaTester::TournamentConfig config;
    config.event = "Test Cup";
    config.type = "round-robin";
    config.tournamentFilename = "state.ini";
    config.saveIntervalMs = 15000;
    config.games = 4;
    config.rounds = 3;
    config.repeat = 2;
    config.ratingInterval = 50;
    config.outcomeInterval = 10;
    config.averageElo = 2400;
    config.noSwap = true;

    auto section = toTournamentSection(config, "test-id");
    REQUIRE(section.name == "tournament");

    auto& manager = loadGroupIntoManager("tournament", {section});
    auto roundtripped = QaplaTester::TournamentConfigFile::fromManager(manager, "tournament");

    REQUIRE(roundtripped.event == config.event);
    REQUIRE(roundtripped.type == config.type);
    REQUIRE(roundtripped.tournamentFilename == config.tournamentFilename);
    REQUIRE(roundtripped.saveIntervalMs == config.saveIntervalMs);
    REQUIRE(roundtripped.games == config.games);
    REQUIRE(roundtripped.rounds == config.rounds);
    REQUIRE(roundtripped.repeat == config.repeat);
    REQUIRE(roundtripped.ratingInterval == config.ratingInterval);
    REQUIRE(roundtripped.outcomeInterval == config.outcomeInterval);
    REQUIRE(roundtripped.averageElo == config.averageElo);
    REQUIRE(roundtripped.noSwap == config.noSwap);
}

TEST_CASE("Openings config round-trips through Settings::Manager", "[config-persistence]") {
    SECTION("With an explicit ply count") {
        QaplaTester::Openings openings;
        openings.file = __FILE__; // any existing path; schema validates "file" with PathExists
        openings.order = "random";
        openings.plies = 6;
        openings.start = 3;
        openings.seed = 12345;
        openings.policy = "round";

        auto section = toOpeningsSection(openings, "test-id");
        auto& manager = loadGroupIntoManager("openings", {section});
        auto roundtripped = QaplaTester::OpeningConfig::fromManager(manager, "openings");

        REQUIRE(roundtripped.file == openings.file);
        REQUIRE(roundtripped.order == openings.order);
        REQUIRE(roundtripped.plies.has_value());
        REQUIRE(*roundtripped.plies == *openings.plies);
        REQUIRE(roundtripped.start == openings.start);
        REQUIRE(roundtripped.seed == openings.seed);
        REQUIRE(roundtripped.policy == openings.policy);
    }

    SECTION("With plies unset (\"all\")") {
        QaplaTester::Openings openings;
        openings.file = __FILE__; // any existing path; schema validates "file" with PathExists
        openings.order = "sequential";
        openings.plies = std::nullopt;
        openings.start = 0;
        openings.seed = 815;
        openings.policy = "default";

        auto section = toOpeningsSection(openings, "test-id");
        auto& manager = loadGroupIntoManager("openings", {section});
        auto roundtripped = QaplaTester::OpeningConfig::fromManager(manager, "openings");

        REQUIRE_FALSE(roundtripped.plies.has_value());
        REQUIRE(roundtripped.start == openings.start);
    }
}

TEST_CASE("PGN output config round-trips through Settings::Manager", "[config-persistence]") {
    QaplaTester::PgnSave::Options options;
    options.file = "games.pgn";
    options.append = false;
    options.onlyFinishedGames = false;
    options.minimalTags = true;
    options.includeClock = false;
    options.includeEval = false;
    options.includePv = true;
    options.includeDepth = false;

    auto section = toPgnOutputSection(options, "test-id");
    auto& manager = loadGroupIntoManager("pgnoutput", {section});
    auto roundtripped = QaplaTester::PgnConfig::fromManager(manager, "pgnoutput");

    REQUIRE(roundtripped.file == options.file);
    REQUIRE(roundtripped.append == options.append);
    REQUIRE(roundtripped.onlyFinishedGames == options.onlyFinishedGames);
    REQUIRE(roundtripped.minimalTags == options.minimalTags);
    REQUIRE(roundtripped.includeClock == options.includeClock);
    REQUIRE(roundtripped.includeEval == options.includeEval);
    REQUIRE(roundtripped.includePv == options.includePv);
    REQUIRE(roundtripped.includeDepth == options.includeDepth);
}

TEST_CASE("Draw adjudication config round-trips through Settings::Manager", "[config-persistence]") {
    QaplaTester::AdjudicationManager::DrawAdjudicationConfig config;
    config.minFullMoves = 40;
    config.requiredConsecutiveMoves = 12;
    config.centipawnThreshold = 15;
    config.testOnly = true;
    config.active = true;

    auto section = toDrawAdjudicationSection(config, "test-id");
    auto& manager = loadGroupIntoManager("draw", {section});
    auto roundtripped = QaplaTester::AdjudicationConfig::fromDrawManager(manager, "draw");

    REQUIRE(roundtripped.minFullMoves == config.minFullMoves);
    REQUIRE(roundtripped.requiredConsecutiveMoves == config.requiredConsecutiveMoves);
    REQUIRE(roundtripped.centipawnThreshold == config.centipawnThreshold);
    REQUIRE(roundtripped.testOnly == config.testOnly);
    // fromDrawManager reports active=true whenever a group instance is present at all.
    REQUIRE(roundtripped.active == true);
}

TEST_CASE("Inactive draw adjudication is not persisted, round-trips to defaults", "[config-persistence]") {
    // Mirrors ImGuiTournamentAdjudication::updateConfiguration(): an inactive config
    // is not written out at all, so loading an empty section list must produce the
    // default-constructed (inactive) config rather than a stale prior value.
    auto& manager = loadGroupIntoManager("draw", QaplaHelpers::IniFile::SectionList{});
    auto roundtripped = QaplaTester::AdjudicationConfig::fromDrawManager(manager, "draw");

    REQUIRE(roundtripped.active == false);
}

TEST_CASE("Resign adjudication config round-trips through Settings::Manager", "[config-persistence]") {
    QaplaTester::AdjudicationManager::ResignAdjudicationConfig config;
    config.requiredConsecutiveMoves = 8;
    config.centipawnThreshold = 700;
    config.twoSided = true;
    config.testOnly = false;
    config.active = true;

    auto section = toResignAdjudicationSection(config, "test-id");
    auto& manager = loadGroupIntoManager("resign", {section});
    auto roundtripped = QaplaTester::AdjudicationConfig::fromResignManager(manager, "resign");

    REQUIRE(roundtripped.requiredConsecutiveMoves == config.requiredConsecutiveMoves);
    REQUIRE(roundtripped.centipawnThreshold == config.centipawnThreshold);
    REQUIRE(roundtripped.twoSided == config.twoSided);
    REQUIRE(roundtripped.testOnly == config.testOnly);
    REQUIRE(roundtripped.active == true);
}

TEST_CASE("SPRT config round-trips through Settings::Manager", "[config-persistence]") {
    QaplaTester::SprtConfig config;
    config.eloH0 = 0.0F;
    config.eloH1 = 5.0F;
    config.alpha = 0.05;
    config.beta = 0.05;
    config.maxGames = 20000;
    config.model = "logistic";
    config.pentanomial = true;

    auto section = toSprtSection(config, "test-id");
    auto& manager = loadGroupIntoManager("sprt", {section});
    auto roundtripped = QaplaTester::SprtConfigFile::fromManager(manager, "sprt");

    REQUIRE(roundtripped.eloH0 == config.eloH0);
    REQUIRE(roundtripped.eloH1 == config.eloH1);
    REQUIRE(roundtripped.alpha == config.alpha);
    REQUIRE(roundtripped.beta == config.beta);
    REQUIRE(roundtripped.maxGames == config.maxGames);
    REQUIRE(roundtripped.model == config.model);
    REQUIRE(roundtripped.pentanomial == config.pentanomial);
}
