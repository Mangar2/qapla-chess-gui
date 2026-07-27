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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

// Split out of gui-tool-tournament.cpp so the unit-tests target can link the
// pure logic (resolveEngines) without dragging in the GUI stack:
// tournament-data.h transitively pulls in ImGui/GLFW headers the unit-tests
// target has no include paths for.

#include "gui-tool-tournament.h"
#include "../tournament-data.h"
#include "../snackbar.h"

#include <tournament/tournament.h>

#include <chrono>
#include <filesystem>
#include <format>

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;
    using QaplaWindows::TournamentData;

    std::string joinStrings(const std::vector<std::string>& values) {
        std::string joined;
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                joined += ", ";
            }
            joined += values[i];
        }
        return joined;
    }

    // Looks for the most recent "tournament"-topic snackbar since
    // countBefore entries existed, i.e. the specific reason
    // TournamentData::startTournament() showed via SnackbarManager for why
    // it silently declined to start (it reports failures only that way --
    // no exception, no return value -- see mayStartTournament()/
    // createTournament() in tournament-data.cpp).
    std::string findRecentTournamentSnackbar(std::size_t countBefore) {
        const auto& history = QaplaWindows::SnackbarManager::instance().getHistory();
        for (auto i = history.size(); i > countBefore; --i) {
            if (history[i - 1].topic == "tournament") {
                return history[i - 1].message;
            }
        }
        return "";
    }

    // ------------------------------------------------------------------
    // select_engines
    // ------------------------------------------------------------------

    Json::JsonValue buildSelectEnginesSchema() {
        auto schema = noArgsToolSchema();
        auto engines = Json::JsonValue::object();
        engines["type"] = "array";
        auto items = Json::JsonValue::object();
        items["type"] = "string";
        engines["items"] = items;
        engines["description"] = "Engine display names, e.g. [\"Stockfish\", \"Qapla\"].";
        schema["properties"]["engines"] = engines;
        schema["required"] = Json::JsonValue::array();
        schema["required"].push_back("engines");
        return schema;
    }

    GuiToolResult handleSelectEngines(const Json::JsonValue& arguments) {
        std::vector<std::string> names;
        if (arguments.contains("engines") && arguments.at("engines").is_array()) {
            for (const auto& item : arguments.at("engines").as_array()) {
                if (item.is_string()) {
                    names.push_back(item.as_string());
                }
            }
        }
        if (names.empty()) {
            return GuiToolResult{.success = false, .content = "No engine names were given."};
        }

        auto outcome = resolveEngines(names);
        if (outcome.resolved.empty()) {
            return GuiToolResult{
                .success = false,
                .content = "None of these engines are installed: " + joinStrings(outcome.notFound) +
                    ". Call list_installed_engines to see what's available."
            };
        }

        auto& tournamentData = TournamentData::instance();
        tournamentData.getEngineSelect().setEngineConfigurations(outcome.resolved);
        tournamentData.config().type = "round-robin"; // gauntlet mode is not exposed via chat
        // config() is a raw reference -- unlike setEngineConfigurations() above,
        // mutating it directly doesn't persist on its own (see
        // ImGuiTournamentConfiguration::updateConfiguration()'s doc comment).
        tournamentData.tournamentConfiguration().updateConfiguration();

        std::vector<std::string> selectedNames;
        for (const auto& engine : outcome.resolved) {
            selectedNames.push_back(engine.getName());
        }

        std::string message = "Selected: " + joinStrings(selectedNames) + ".";
        if (!outcome.notFound.empty()) {
            message += " Not installed (skipped): " + joinStrings(outcome.notFound) + ".";
        }
        return GuiToolResult{.success = true, .content = message};
    }

    // ------------------------------------------------------------------
    // configure_tournament
    // ------------------------------------------------------------------

    Json::JsonValue buildConfigureTournamentSchema() {
        auto schema = noArgsToolSchema();
        auto& properties = schema["properties"];

        auto stringProp = [](const std::string& description) {
            auto prop = Json::JsonValue::object();
            prop["type"] = "string";
            prop["description"] = description;
            return prop;
        };
        auto integerProp = [](const std::string& description) {
            auto prop = Json::JsonValue::object();
            prop["type"] = "integer";
            prop["description"] = description;
            return prop;
        };

        properties["time_control"] = timeControlSchemaProperty();
        properties["games"] = integerProp(
            "Games played per engine pairing PER ROUND -- not the tournament total. "
            "Total games for one pairing = games * rounds (and the tournament plays this for "
            "every pairing of selected engines). If the user just gives one number of games "
            "with no mention of rounds, set games to that number and leave rounds at its "
            "default of 1, so total equals what they said. If the user gives BOTH a total game "
            "count and a number of rounds (e.g. \"100 games total, 10 rounds\"), compute "
            "games = total / rounds yourself (100/10 -> games=10, rounds=10) -- do not set the "
            "total as-is into games. If a game count and a round count are both given but it's "
            "unclear whether the game count is the total or the per-round count (e.g. \"set it "
            "to 100 games and 10 rounds\" with nothing clarifying which), ask the user which "
            "they mean instead of guessing.");
        properties["rounds"] = integerProp(
            "How many times the full set of pairings is repeated. Defaults to 1. See the "
            "\"games\" description for how this multiplies into the tournament total.");
        properties["event"] = stringProp("Tournament/event name.");
        properties["openings_file"] = stringProp("Path to an existing EPD or PGN opening book file on disk.");
        properties["pgn_file"] = stringProp("Path to save the played games as PGN.");
        properties["concurrency"] = integerProp("Number of games to run in parallel.");
        return schema;
    }

    void applyTimeControl(TournamentData& data, const std::string& value, std::vector<std::string>& applied) {
        // Not validated: QaplaTester::TimeControl::parse() is deliberately
        // lenient (never throws, silently ignores whatever it can't make
        // sense of) -- the classic UI's time control text field relies on
        // the same behavior, so a stricter check here would just be
        // inconsistent, not safer.
        data.getGlobalSettings().setTimeControlSettings({.timeControl = value});
        applied.push_back("time control " + value);
    }

    // Reports the effective per-pairing total (games * rounds) alongside
    // whichever field just changed -- games and rounds multiply into the
    // actual game count, which is easy to misjudge (both for the model and
    // the user reading the chat), so every change restates it using
    // whatever the *other* field currently is, not just the one just set.
    std::string gamesAndRoundsSummary(TournamentData& data) {
        return std::format("games={} per round, rounds={} ({} games per pairing in total)",
            data.config().games, data.config().rounds, data.config().games * data.config().rounds);
    }

    void applyGames(TournamentData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value < 1) {
            problems.push_back("games must be at least 1");
            return;
        }
        data.config().games = static_cast<uint32_t>(value);
        applied.push_back(gamesAndRoundsSummary(data));
    }

    void applyRounds(TournamentData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value < 1) {
            problems.push_back("rounds must be at least 1");
            return;
        }
        data.config().rounds = static_cast<uint32_t>(value);
        applied.push_back(gamesAndRoundsSummary(data));
    }

    void applyEvent(TournamentData& data, const std::string& value, std::vector<std::string>& applied) {
        data.config().event = value;
        applied.push_back("event name");
    }

    void applyOpeningsFile(TournamentData& data, const std::string& path, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (!std::filesystem::exists(path)) {
            problems.push_back("openings file not found: " + path);
            return;
        }
        data.tournamentOpening().openings().file = path;
        applied.push_back("openings file");
    }

    void applyPgnFile(TournamentData& data, const std::string& path, std::vector<std::string>& applied) {
        data.pgnConfig().file = path;
        applied.push_back("PGN output file");
    }

    void applyConcurrency(TournamentData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value < 1) {
            problems.push_back("concurrency must be at least 1");
            return;
        }
        auto concurrency = static_cast<uint32_t>(value);
        data.setExternalConcurrency(concurrency);
        applied.push_back("concurrency=" + std::to_string(concurrency));
    }

    GuiToolResult buildConfigureResult(const std::vector<std::string>& applied, const std::vector<std::string>& problems) {
        std::string message;
        if (!applied.empty()) {
            message = "Configured: " + joinStrings(applied) + ".";
        }
        if (!problems.empty()) {
            if (!message.empty()) {
                message += " ";
            }
            message += "Problems: " + joinStrings(problems) + ".";
        }
        if (message.empty()) {
            message = "No configuration changes were provided.";
        }
        return GuiToolResult{.success = problems.empty(), .content = message};
    }

    GuiToolResult handleConfigureTournament(const Json::JsonValue& arguments) {
        auto& tournamentData = TournamentData::instance();
        std::vector<std::string> applied;
        std::vector<std::string> problems;

        if (arguments.contains("time_control") && arguments.at("time_control").is_string()) {
            applyTimeControl(tournamentData, arguments.at("time_control").as_string(), applied);
        }
        if (arguments.contains("games") && arguments.at("games").is_number()) {
            applyGames(tournamentData, arguments.at("games").as_number(), applied, problems);
        }
        if (arguments.contains("rounds") && arguments.at("rounds").is_number()) {
            applyRounds(tournamentData, arguments.at("rounds").as_number(), applied, problems);
        }
        if (arguments.contains("event") && arguments.at("event").is_string()) {
            applyEvent(tournamentData, arguments.at("event").as_string(), applied);
        }
        if (arguments.contains("openings_file") && arguments.at("openings_file").is_string()) {
            applyOpeningsFile(tournamentData, arguments.at("openings_file").as_string(), applied, problems);
        }
        if (arguments.contains("pgn_file") && arguments.at("pgn_file").is_string()) {
            applyPgnFile(tournamentData, arguments.at("pgn_file").as_string(), applied);
        }
        if (arguments.contains("concurrency") && arguments.at("concurrency").is_number()) {
            applyConcurrency(tournamentData, arguments.at("concurrency").as_number(), applied, problems);
        }

        // applyGames/applyRounds/applyEvent, applyOpeningsFile and
        // applyPgnFile all mutate raw references (config()/openings()/
        // pgnOptions()) that don't persist on their own -- see
        // ImGuiTournamentConfiguration::updateConfiguration()'s doc comment.
        // Calling all three unconditionally is cheap and always correct,
        // whether or not this particular call touched their fields.
        tournamentData.tournamentConfiguration().updateConfiguration();
        tournamentData.tournamentOpening().updateConfiguration();
        tournamentData.tournamentPgn().updateConfiguration();

        return buildConfigureResult(applied, problems);
    }

    // ------------------------------------------------------------------
    // get_tournament_status
    // ------------------------------------------------------------------

    GuiToolResult handleGetTournamentStatus(const Json::JsonValue&) {
        auto& tournamentData = TournamentData::instance();
        auto selectedEngines = tournamentData.getEngineSelect().getSelectedEngines();
        std::vector<std::string> engineNames;
        for (const auto& engine : selectedEngines) {
            engineNames.push_back(engine.getName());
        }
        const auto& config = tournamentData.config();
        const auto& openingsFile = tournamentData.tournamentOpening().openings().file;
        const auto& pgnFile = tournamentData.pgnConfig().file;

        std::string runState = "No tournament is currently running.";
        if (tournamentData.isRunning()) {
            runState = "A tournament is currently running.";
        } else if (tournamentData.isStarting()) {
            runState = "A tournament is currently starting.";
        }

        std::string message = std::format(
            "Engines: {}. Time control: {}. Games per pairing: {}. Rounds: {}. "
            "Event name: {}. Openings file: {}. PGN output file: {}. Concurrency: {}. {}",
            engineNames.empty() ? "none selected" : joinStrings(engineNames),
            tournamentData.getGlobalSettings().getTimeControlSettings().timeControl,
            config.games, config.rounds,
            config.event.empty() ? "(not set)" : config.event,
            openingsFile.empty() ? "(not set)" : openingsFile,
            pgnFile.empty() ? "(not set)" : pgnFile,
            tournamentData.getExternalConcurrency(),
            runState);

        return GuiToolResult{.success = true, .content = message};
    }

    // ------------------------------------------------------------------
    // start_tournament
    // ------------------------------------------------------------------

    GuiToolResult handleStartTournament(const Json::JsonValue&) {
        auto& tournamentData = TournamentData::instance();
        if (tournamentData.isRunning() || tournamentData.isStarting()) {
            return GuiToolResult{
                .success = false,
                .content = "A tournament is already running. Stop it first if you want to start a different one."
            };
        }

        auto historyCountBefore = QaplaWindows::SnackbarManager::instance().getHistory().size();

        tournamentData.startTournament(); // verbose=true, same as the classic "Start Tournament" button

        if (!tournamentData.isRunning() && !tournamentData.isStarting()) {
            auto reason = findRecentTournamentSnackbar(historyCountBefore);
            return GuiToolResult{
                .success = false,
                .content = reason.empty() ? "Could not start the tournament." : reason
            };
        }

        tournamentData.setPoolConcurrency(tournamentData.getExternalConcurrency(), true, true);
        return GuiToolResult{.success = true, .content = "Tournament started."};
    }
}

void registerTournamentTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "select_engines",
        .description = "Selects which configured chess engines play in the next tournament, "
                        "replacing any previous selection. Names are matched case-insensitively "
                        "against the installed engine catalog -- call list_installed_engines "
                        "first if unsure what's available. Sets up a round-robin tournament "
                        "(every selected engine plays every other one); gauntlet mode is not "
                        "supported via chat.",
        .parametersSchema = buildSelectEnginesSchema(),
        .handler = handleSelectEngines
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_tournament",
        .description = "Sets tournament options: time_control, games (per pairing), rounds, "
                        "event (name), openings_file, pgn_file, concurrency. Every field is "
                        "independent and optional -- pass ONLY the one thing the user asked to "
                        "change (e.g. just \"games\" to change the game count); do not require "
                        "or ask for any of the other fields first. Anything not passed here "
                        "keeps whatever it was already set to (this session or an earlier one) "
                        "-- call get_tournament_status first if you're not sure what that "
                        "currently is, rather than assuming it's unset. openings_file must be "
                        "set (here or in an earlier session) before start_tournament will "
                        "succeed; there is no safe default, so ask the user for a path only if "
                        "get_tournament_status confirms none is configured yet.",
        .parametersSchema = buildConfigureTournamentSchema(),
        .handler = handleConfigureTournament
    });

    registry.registerTool(GuiToolDefinition{
        .name = "get_tournament_status",
        .description = "Reports the tournament configuration and state as currently set up: "
                        "selected engines, time control, games/rounds, event name, openings "
                        "file, PGN output file, concurrency, and whether a tournament is "
                        "running. Call this FIRST whenever a request only changes one thing "
                        "(e.g. \"set it to 10 games\") and you're not certain everything else is "
                        "already configured -- it almost always is, from this session or an "
                        "earlier one. Use it to confirm that before asking the user to restate "
                        "settings or declining to make the one change they actually asked for, "
                        "and to confirm a previous select_engines/configure_tournament call "
                        "actually took effect.",
        .handler = handleGetTournamentStatus
    });

    registry.registerTool(GuiToolDefinition{
        .name = "start_tournament",
        .description = "Starts the tournament with the engines and settings configured via "
                        "select_engines/configure_tournament. Requires at least two selected "
                        "engines and an openings file to already be configured; the result tells "
                        "you exactly which precondition is missing if it can't start.",
        .handler = handleStartTournament,
        // Engine processes need to launch and initialize; a handful of
        // engines can legitimately take longer than the default 30s.
        .timeout = std::chrono::seconds(60)
    });
}

} // namespace QaplaLlm
