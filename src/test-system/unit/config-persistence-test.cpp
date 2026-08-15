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
#include <tournament/tournament-file.h>

#include <filesystem>
#include <fstream>

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

TEST_CASE("A section that no longer matches the schema is reported, not silently defaulted",
          "[config-persistence]") {
    // A stored file written by an older version can carry keys the current schema no longer
    // knows (e.g. a .qtour with "seed" where "openings" now expects "srand"). The group then
    // falls back to defaults; without a reporter that fallback is invisible to the user and
    // the empty openings.file goes on to block tournament creation -- and with it the restore
    // of stored game results.
    QaplaHelpers::IniFile::Section stale;
    stale.name = "openings";
    stale.addEntry("id", "test-id");
    stale.addEntry("file", __FILE__);
    stale.addEntry("seed", "815"); // schema key is "srand"

    std::vector<std::pair<std::string, std::string>> reported;
    setConfigLoadErrorReporter([&](const std::string& sectionName, const std::string& message) {
        reported.emplace_back(sectionName, message);
    });

    auto& manager = loadGroupIntoManager("openings", {stale});
    auto roundtripped = QaplaTester::OpeningConfig::fromManager(manager, "openings");

    setConfigLoadErrorReporter(nullptr);

    REQUIRE(reported.size() == 1);
    REQUIRE(reported[0].first == "openings");
    REQUIRE(reported[0].second.find("seed") != std::string::npos);
    // The fallback itself must stay intact: defaults rather than a propagated exception.
    REQUIRE(roundtripped.file.empty());
}

TEST_CASE("Loading a tournament state file keeps unrelated settings", "[config-persistence]") {
    // A .qtour holds only the section types TournamentFile::save() writes. Reading it must
    // replace exactly those and leave every other setting alone -- reading it with
    // ConfigData::load() used to clear the whole tree, and the truncated tree was then
    // written back to qapla-chess-gui.ini, permanently losing unrelated settings.
    namespace fs = std::filesystem;

    auto stateFile = fs::temp_directory_path() / "qapla-state-file-test.qtour";
    {
        std::ofstream out(stateFile, std::ios::trunc);
        REQUIRE(out.is_open());
        out << "[tournament]\nid=tournament\nevent=Loaded Cup\n\n"
            << "[round]\nid=tournament\nengineA=A\nengineB=B\ngames=1\n\n";
    }

    auto sectionWith = [](const std::string& name, const std::string& id,
        const std::string& key, const std::string& value) {
        QaplaHelpers::IniFile::Section section{ .name = name, .entries = {} };
        section.addEntry("id", id);
        section.addEntry(key, value);
        return section;
    };

    QaplaHelpers::ConfigData config;
    config.addSection(sectionWith("performance", "general", "remotedesktopmode", "true"));
    config.addSection(sectionWith("llmchat", "general", "host", "localhost"));
    config.addSection(sectionWith("engine", "board1", "name", "BoardEngine"));
    config.addSection(sectionWith("timecontroloptions", "tournament", "timecontrol", "40/5+0"));
    config.addSection(sectionWith("tournament", "tournament", "event", "Old Cup"));
    config.addSection(sectionWith("pgnoutput", "tournament", "file", "old.pgn"));

    loadStateFileSections(stateFile.string(), config,
        QaplaTester::TournamentFile::sectionNames, QaplaTester::TournamentFile::id);
    fs::remove(stateFile);

    SECTION("Settings that do not belong to the tournament survive") {
        REQUIRE(config.getSectionList("performance", "general").value().at(0)
            .getValue("remotedesktopmode").value() == "true");
        REQUIRE(config.getSectionList("llmchat", "general").value().at(0)
            .getValue("host").value() == "localhost");
        // "engine" is a tournament section type, but only under the tournament's own id
        REQUIRE(config.getSectionList("engine", "board1").value().at(0)
            .getValue("name").value() == "BoardEngine");
        // Not written by TournamentFile::save(), so loading must not drop it either
        REQUIRE(config.getSectionList("timecontroloptions", "tournament").value().at(0)
            .getValue("timecontrol").value() == "40/5+0");
    }

    SECTION("The tournament's own sections are replaced by the file") {
        REQUIRE(config.getSectionList("tournament", "tournament").value().at(0)
            .getValue("event").value() == "Loaded Cup");
        REQUIRE(config.getSectionList("round", "tournament").value().size() == 1);
    }

    SECTION("Tournament sections missing from the file are cleared") {
        // Otherwise the previous tournament's PGN output would leak into the loaded one
        REQUIRE(config.getSectionList("pgnoutput", "tournament").value_or(
            QaplaHelpers::IniFile::SectionList{}).empty());
    }
}

TEST_CASE("A state file lists the participants only", "[config-persistence]") {
    // Everything outside the GUI reads every "engine" section of a state file as an entrant
    // -- "selected" is GUI-only state and is not evaluated there.
    auto engine = [](const std::string& name, bool selected) {
        QaplaHelpers::IniFile::Section section;
        section.name = "engine";
        section.addEntry("id", "board1");
        section.addEntry("name", name);
        section.addEntry("cmd", "/engines/" + name);
        auto config = QaplaTester::EngineConfig::createFromSection(section);
        config.setSelected(selected);
        return config;
    };

    const auto sections = toParticipantSections(
        {engine("Playing", true), engine("Deselected", false)}, "tournament");

    REQUIRE(sections.size() == 1);
    CHECK(sections.at(0).getValue("name").value() == "Playing");
    CHECK(sections.at(0).getValue("id").value() == "tournament");
    CHECK_FALSE(sections.at(0).getValue("selected").has_value());
}

TEST_CASE("A state file's \"each\" section holds the settings in force only",
          "[config-persistence]") {
    // The GUI keeps a switched-off setting's value so the user gets it back when switching the
    // setting on again. A state file cannot say that: "each" is the CLI's defaults layer, where
    // everything listed applies. So only what is switched on is written -- and the CLI's own key
    // names are used, or the file does not parse at all.
    QaplaTester::EngineGlobalConfig config;
    config.useGlobalHash = true;
    config.hashSizeMB = 128;
    config.useGlobalPonder = false;
    config.ponder = true;              // configured, but switched off
    config.useGlobalTrace = true;
    config.traceLevel = "none";
    config.useGlobalRestart = false;
    config.restart = "on";             // configured, but switched off
    config.timeControl = "20.0+0.02";

    const auto section = toEachSection(config, "tournament");

    REQUIRE(section.name == "each");
    CHECK(section.getValue("id").value() == "tournament");
    CHECK(section.getValue("option.Hash").value() == "128");
    CHECK(section.getValue("trace").value() == "none");
    CHECK(section.getValue("tc").value() == "20.0+0.02");
    CHECK_FALSE(section.getValue("ponder").has_value());
    CHECK_FALSE(section.getValue("restart").has_value());
    // The flags themselves are GUI state and have no meaning in a state file
    for (const auto& key : {"usehash", "useponder", "usetrace", "userestart", "hash"}) {
        CHECK_FALSE(section.getValue(key).has_value());
    }

    SECTION("The section parses against the CLI's \"each\" schema") {
        std::vector<std::pair<std::string, std::string>> reported;
        setConfigLoadErrorReporter([&](const std::string& sectionName, const std::string& message) {
            reported.emplace_back(sectionName, message);
        });
        loadGroupIntoManager("each", {section});
        setConfigLoadErrorReporter(nullptr);

        CHECK(reported.empty());
    }

    SECTION("Being listed is what switches a setting on when reading back") {
        const auto roundtripped = fromEachSection(section);

        CHECK(roundtripped.useGlobalHash == true);
        CHECK(roundtripped.hashSizeMB == 128);
        CHECK(roundtripped.useGlobalTrace == true);
        CHECK(roundtripped.traceLevel == "none");
        CHECK(roundtripped.timeControl == "20.0+0.02");
        CHECK(roundtripped.useGlobalPonder == false);
        CHECK(roundtripped.useGlobalRestart == false);
    }
}

namespace {

    QaplaHelpers::IniFile::Section sectionOf(const std::string& name, const std::string& id,
        const std::vector<std::pair<std::string, std::string>>& entries) {
        QaplaHelpers::IniFile::Section section{ .name = name, .entries = {} };
        section.addEntry("id", id);
        for (const auto& [key, value] : entries) {
            section.addEntry(key, value);
        }
        return section;
    }

} // namespace

TEST_CASE("A global setting is written where the CLI's rule lets it win", "[config-persistence]") {
    // Read the CLI's way, an engine's own entry beats "each". EngineConfig::toSection() writes
    // "trace" and "restart" for every engine, so a global trace level would never take effect
    // outside the GUI unless those keys are taken out of the engine sections.
    const auto each = sectionOf("each", "tournament", {{"trace", "none"}, {"option.Hash", "32"}});
    const QaplaHelpers::IniFile::SectionList engines = {
        sectionOf("engine", "tournament", {{"name", "A"}, {"trace", "all"}, {"option.Hash", "64"}}),
        sectionOf("engine", "tournament", {{"name", "B"}, {"trace", "all"}, {"tc", "1+0"}}),
    };

    const auto cleaned = withoutEachDefaults(engines, each);

    REQUIRE(cleaned.size() == 2);
    for (const auto& engine : cleaned) {
        CHECK_FALSE(engine.getValue("trace").has_value());
        CHECK_FALSE(engine.getValue("option.Hash").has_value());
        CHECK(engine.getValue("id").value() == "tournament");
    }
    // Untouched: not a global setting
    CHECK(cleaned.at(1).getValue("tc").value() == "1+0");
}

TEST_CASE("Reading a state file turns its defaults into the GUI's globals",
          "[config-persistence]") {
    const auto each = sectionOf("each", "tournament",
        {{"ponder", "false"}, {"trace", "none"}, {"option.Threads", "4"}});
    const QaplaHelpers::IniFile::SectionList engines = {
        sectionOf("engine", "tournament", {{"name", "A"}, {"ponder", "true"}}),
        sectionOf("engine", "tournament", {{"name", "B"}}),
    };

    const auto resolved = resolveEachDefaults(each, engines);

    SECTION("A setting the engines disagree on is not global, and every engine states it") {
        // Engine B did not mention ponder, so it inherits the file's default -- exactly what the
        // CLI would have run it with. Without that, dropping "ponder" from "each" would flip B.
        CHECK(resolved.engines.at(0).getValue("ponder").value() == "true");
        CHECK(resolved.engines.at(1).getValue("ponder").value() == "false");
        CHECK(resolved.each.getValue("useponder").value() == "false");
        // The value stays visible so the user sees what the file proposed
        CHECK(resolved.each.getValue("ponder").value() == "false");
        CHECK(fromEachSection(resolved.each).useGlobalPonder == false);
    }

    SECTION("A setting they all end up agreeing on becomes the global one") {
        CHECK(resolved.each.getValue("trace").value() == "none");
        for (const auto& engine : resolved.engines) {
            CHECK_FALSE(engine.getValue("trace").has_value());
        }
        CHECK(fromEachSection(resolved.each).useGlobalTrace == true);
    }

    SECTION("A setting the GUI has no control for moves to the engines") {
        // The GUI's global panel cannot hold it, so leaving it in "each" would mean losing it on
        // the next save. In the engine sections it survives untouched.
        CHECK_FALSE(resolved.each.getValue("option.Threads").has_value());
        for (const auto& engine : resolved.engines) {
            CHECK(engine.getValue("option.Threads").value() == "4");
        }
    }

    SECTION("Saving what was read leaves the file's meaning unchanged") {
        // The full round trip: what the GUI took from the file goes back through its own global
        // settings and out again.
        const auto each2 = toEachSection(fromEachSection(resolved.each), "tournament");
        const auto engines2 = withoutEachDefaults(resolved.engines, each2);
        const auto again = resolveEachDefaults(each2, engines2);

        CHECK(again.engines.at(0).entries == resolved.engines.at(0).entries);
        CHECK(again.engines.at(1).entries == resolved.engines.at(1).entries);
        CHECK(again.each.getValue("trace").value() == "none");
        CHECK_FALSE(again.each.getValue("ponder").has_value());
    }
}

TEST_CASE("An \"each\" section with \"use...\" flags applies only what is switched on",
          "[config-persistence]") {
    // State files written before the flags were dropped still carry them, and so does
    // qapla-chess-gui.ini. Neither the flags nor a value they switch off may reach the engines:
    // "usehash" is no engine key, and the CLI refuses to read a section containing one.
    const auto each = sectionOf("each", "tournament",
        {{"usehash", "false"}, {"hash", "512"}, {"usetrace", "true"}, {"trace", "none"}});
    const QaplaHelpers::IniFile::SectionList engines = {
        sectionOf("engine", "tournament", {{"name", "A"}}),
    };

    SECTION("Reading") {
        const auto resolved = resolveEachDefaults(each, engines);

        CHECK_FALSE(resolved.engines.at(0).getValue("usehash").has_value());
        CHECK_FALSE(resolved.engines.at(0).getValue("hash").has_value());
        CHECK(resolved.each.getValue("trace").value() == "none");
    }

    SECTION("Writing") {
        const auto cleaned = withoutEachDefaults(
            {sectionOf("engine", "tournament", {{"name", "A"}, {"hash", "64"}, {"trace", "all"}})},
            each);

        CHECK(cleaned.at(0).getValue("hash").value() == "64");  // global hash is switched off
        CHECK_FALSE(cleaned.at(0).getValue("trace").has_value());
    }
}

TEST_CASE("The GUI's own \"each\" section keeps its switched-off values", "[config-persistence]") {
    // qapla-chess-gui.ini states the flags explicitly, so that a value configured but not in use
    // survives a restart. Those flags win over the presence rule a state file is read by.
    QaplaHelpers::IniFile::Section section;
    section.name = "each";
    section.addEntry("id", "tournament");
    section.addEntry("usehash", "false");
    section.addEntry("hash", "512");
    section.addEntry("useponder", "true");
    section.addEntry("ponder", "true");

    const auto config = fromEachSection(section);

    CHECK(config.useGlobalHash == false);
    CHECK(config.hashSizeMB == 512);       // remembered although switched off
    CHECK(config.useGlobalPonder == true);
    CHECK(config.ponder == true);
    // Nothing said about trace/restart at all: not in force, and no time control named
    CHECK(config.useGlobalTrace == false);
    CHECK(config.useGlobalRestart == false);
    CHECK(config.timeControl.empty());
}
