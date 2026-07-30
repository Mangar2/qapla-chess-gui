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

#include <imgui.h>

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

    // adjudicationModeText()/adjudicationModeSchemaProperty()/applyAdjudicationMode() live in
    // gui-tool-registry.h -- shared with the SPRT adjudication tools (gui-tool-sprt-register.cpp)
    // so the off/test/active vocabulary and its bool-pair mapping is written exactly once.
    std::string adjudicationSummary(TournamentData& data) {
        const auto& draw = data.drawConfig();
        const auto& resign = data.resignConfig();
        return std::format(
            "Draw adjudication: {} (min full moves={}, required consecutive moves={}, "
            "centipawn threshold={}). Resign adjudication: {} (required consecutive moves={}, "
            "centipawn threshold={}, two-sided={}).",
            adjudicationModeText(draw.active, draw.testOnly),
            draw.minFullMoves, draw.requiredConsecutiveMoves, draw.centipawnThreshold,
            adjudicationModeText(resign.active, resign.testOnly),
            resign.requiredConsecutiveMoves, resign.centipawnThreshold,
            resign.twoSided ? "yes" : "no");
    }

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
            "Event name: {}. Openings file: {}. PGN output file: {}. Concurrency: {}. {} {}",
            engineNames.empty() ? "none selected" : joinStrings(engineNames),
            tournamentData.getGlobalSettings().getTimeControlSettings().timeControl,
            config.games, config.rounds,
            config.event.empty() ? "(not set)" : config.event,
            openingsFile.empty() ? "(not set)" : openingsFile,
            pgnFile.empty() ? "(not set)" : pgnFile,
            tournamentData.getExternalConcurrency(),
            runState,
            adjudicationSummary(tournamentData));

        return GuiToolResult{.success = true, .content = message};
    }

    // ------------------------------------------------------------------
    // configure_draw_adjudication / configure_resign_adjudication
    // ------------------------------------------------------------------

    Json::JsonValue buildConfigureDrawAdjudicationSchema() {
        auto schema = noArgsToolSchema();
        auto& properties = schema["properties"];

        auto intProp = [](const std::string& description) {
            auto prop = Json::JsonValue::object();
            prop["type"] = "integer";
            prop["description"] = description;
            return prop;
        };

        properties["mode"] = adjudicationModeSchemaProperty(
            "\"off\" disables draw adjudication. \"test\" evaluates and logs what it would have "
            "decided without ending games. \"active\" actually ends games early as a draw once "
            "the conditions below are met.");
        properties["min_full_moves"] = intProp(
            "Minimum number of full moves that must be played before draw adjudication can "
            "trigger at all. Default 80.");
        properties["required_consecutive_moves"] = intProp(
            "Number of consecutive moves (evaluated by the engines themselves) that must all "
            "stay within centipawn_threshold of equal before the game is adjudicated a draw. "
            "Default 20.");
        properties["centipawn_threshold"] = intProp(
            "Maximum absolute evaluation, in centipawns, for a position to still count as drawn "
            "(e.g. 20 means within +/-20cp of dead equal). Positive number. Default 20.");
        return schema;
    }

    GuiToolResult handleConfigureDrawAdjudication(const Json::JsonValue& arguments) {
        auto& tournamentData = TournamentData::instance();
        auto& config = tournamentData.drawConfig();
        std::vector<std::string> applied;
        std::vector<std::string> problems;

        if (arguments.contains("mode") && arguments.at("mode").is_string()) {
            const auto& mode = arguments.at("mode").as_string();
            if (applyAdjudicationMode(mode, config.active, config.testOnly, problems)) {
                applied.push_back("draw adjudication mode=" + mode);
            }
        }
        if (arguments.contains("min_full_moves") && arguments.at("min_full_moves").is_number()) {
            config.minFullMoves = static_cast<uint32_t>(arguments.at("min_full_moves").as_number());
            applied.push_back("min full moves=" + std::to_string(config.minFullMoves));
        }
        if (arguments.contains("required_consecutive_moves") &&
            arguments.at("required_consecutive_moves").is_number()) {
            config.requiredConsecutiveMoves =
                static_cast<uint32_t>(arguments.at("required_consecutive_moves").as_number());
            applied.push_back("required consecutive moves=" + std::to_string(config.requiredConsecutiveMoves));
        }
        if (arguments.contains("centipawn_threshold") && arguments.at("centipawn_threshold").is_number()) {
            config.centipawnThreshold = static_cast<int>(arguments.at("centipawn_threshold").as_number());
            applied.push_back("centipawn threshold=" + std::to_string(config.centipawnThreshold));
        }

        // config is a raw reference into ImGuiTournamentAdjudication -- mutating it directly
        // doesn't persist on its own (see ImGuiTournamentAdjudication::updateConfiguration()'s
        // doc comment), so this must be called explicitly after every change made this way.
        tournamentData.tournamentAdjudication().updateConfiguration();
        return buildConfigureResult(applied, problems);
    }

    Json::JsonValue buildConfigureResignAdjudicationSchema() {
        auto schema = noArgsToolSchema();
        auto& properties = schema["properties"];

        auto intProp = [](const std::string& description) {
            auto prop = Json::JsonValue::object();
            prop["type"] = "integer";
            prop["description"] = description;
            return prop;
        };
        auto boolProp = [](const std::string& description) {
            auto prop = Json::JsonValue::object();
            prop["type"] = "boolean";
            prop["description"] = description;
            return prop;
        };

        properties["mode"] = adjudicationModeSchemaProperty(
            "\"off\" disables resign adjudication. \"test\" evaluates and logs what it would "
            "have decided without ending games. \"active\" actually ends games early as a loss "
            "for the losing side once the conditions below are met.");
        properties["required_consecutive_moves"] = intProp(
            "Number of consecutive moves whose own evaluation must stay at or below "
            "-centipawn_threshold (i.e. that bad or worse) before the game is adjudicated a "
            "resignation. Default 5.");
        properties["centipawn_threshold"] = intProp(
            "How bad (in centipawns) a position must be, from the losing side's own point of "
            "view, before it counts as resignation-worthy -- e.g. 500 means resign once down "
            "roughly a queen's worth of evaluation. Give a positive magnitude, not a negative "
            "number. Default 500.");
        properties["two_sided"] = boolProp(
            "If true, both engines must independently agree the position is lost before "
            "adjudicating -- more conservative, avoids a resignation from one engine's blunder "
            "in evaluation alone. Default false.");
        return schema;
    }

    GuiToolResult handleConfigureResignAdjudication(const Json::JsonValue& arguments) {
        auto& tournamentData = TournamentData::instance();
        auto& config = tournamentData.resignConfig();
        std::vector<std::string> applied;
        std::vector<std::string> problems;

        if (arguments.contains("mode") && arguments.at("mode").is_string()) {
            const auto& mode = arguments.at("mode").as_string();
            if (applyAdjudicationMode(mode, config.active, config.testOnly, problems)) {
                applied.push_back("resign adjudication mode=" + mode);
            }
        }
        if (arguments.contains("required_consecutive_moves") &&
            arguments.at("required_consecutive_moves").is_number()) {
            config.requiredConsecutiveMoves =
                static_cast<uint32_t>(arguments.at("required_consecutive_moves").as_number());
            applied.push_back("required consecutive moves=" + std::to_string(config.requiredConsecutiveMoves));
        }
        if (arguments.contains("centipawn_threshold") && arguments.at("centipawn_threshold").is_number()) {
            config.centipawnThreshold = static_cast<int>(arguments.at("centipawn_threshold").as_number());
            applied.push_back("centipawn threshold=" + std::to_string(config.centipawnThreshold));
        }
        if (arguments.contains("two_sided") && arguments.at("two_sided").is_boolean()) {
            config.twoSided = arguments.at("two_sided").as_boolean();
            applied.push_back(std::string("two-sided=") + (config.twoSided ? "yes" : "no"));
        }

        tournamentData.tournamentAdjudication().updateConfiguration();
        return buildConfigureResult(applied, problems);
    }

    // ------------------------------------------------------------------
    // stop_tournament
    // ------------------------------------------------------------------

    Json::JsonValue buildStopTournamentSchema() {
        auto schema = noArgsToolSchema();
        auto mode = Json::JsonValue::object();
        mode["type"] = "string";
        auto enumValues = Json::JsonValue::array();
        enumValues.push_back("graceful");
        enumValues.push_back("abrupt");
        mode["enum"] = enumValues;
        mode["description"] =
            "\"graceful\" (default if omitted): let games currently in progress finish, then "
            "stop -- no new games are started. \"abrupt\": abort all in-progress games "
            "immediately without letting them finish. If the user just says \"stop\"/\"end the "
            "tournament\" without qualifying it, use \"graceful\" -- ask only if they've "
            "previously shown they care about the distinction.";
        schema["properties"]["mode"] = mode;
        return schema;
    }

    GuiToolResult handleStopTournament(const Json::JsonValue& arguments) {
        auto& tournamentData = TournamentData::instance();
        if (!tournamentData.isRunning() && !tournamentData.isStarting()) {
            return GuiToolResult{.success = false, .content = "No tournament is currently running."};
        }

        bool graceful = true;
        if (arguments.contains("mode") && arguments.at("mode").is_string()) {
            graceful = arguments.at("mode").as_string() != "abrupt";
        }

        tournamentData.stopPool(graceful);
        return GuiToolResult{
            .success = true,
            .content = graceful
                ? "Stopping the tournament gracefully: games already in progress will be "
                  "finished, no new games will start."
                : "Stopping the tournament abruptly: all in-progress games are being aborted "
                  "immediately."
        };
    }

    // ------------------------------------------------------------------
    // clear_tournament_result
    // ------------------------------------------------------------------

    GuiToolResult handleClearTournamentResult(const Json::JsonValue&) {
        auto& tournamentData = TournamentData::instance();
        if (!tournamentData.hasTasksScheduled()) {
            return GuiToolResult{.success = true, .content = "There are no tournament results to clear."};
        }

        bool wasRunning = tournamentData.isRunning() || tournamentData.isStarting();
        tournamentData.clear(false); // verbose=false: this tool's own content already tells the user
        return GuiToolResult{
            .success = true,
            .content = wasRunning
                ? "Tournament stopped and all results cleared."
                : "All tournament results have been cleared."
        };
    }

    // ------------------------------------------------------------------
    // show_tournament_result
    // ------------------------------------------------------------------

    GuiToolResult handleShowTournamentResult(const Json::JsonValue&) {
        auto& tournamentData = TournamentData::instance();
        if (tournamentData.getTournamentResult().scoredEngines().empty()) {
            return GuiToolResult{.success = true, .content = "No tournament results are available yet."};
        }

        // Renders the same live control the classic (non-AI) chatbot's results step draws (see
        // ChatbotStepStandardTournamentResult::draw()) -- a real ImGuiTable, not a text dump of
        // the data. Always reads TournamentData::instance() fresh at draw time (every frame the
        // ChatEntry stays visible), so it reflects the tournament's current state, exactly like
        // that classic step does, rather than a one-time snapshot of the results as they were
        // when this tool was called.
        return GuiToolResult{
            .success = true,
            .content = "Showing the current tournament results as a table in the chat -- it is "
                        "already visible to the user, so do not restate, list, or summarize the "
                        "numbers in your reply; just briefly confirm what you did.",
            .renderWidget = []() {
                auto& data = TournamentData::instance();
                ImGui::Text("Tournament Progress: %u / %u games completed",
                    data.getPlayedGames(), data.getTotalGames());
                ImGui::Spacing();
                data.drawEloTable(ImVec2(0.0F, 3000.0F));
            }
        };
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
                        "file, PGN output file, concurrency, draw/resign adjudication settings, "
                        "and whether a tournament is running. Call this FIRST whenever a request "
                        "only changes one thing (e.g. \"set it to 10 games\", \"turn on resign "
                        "adjudication\") and you're not certain everything else is already "
                        "configured -- it almost always is, from this session or an earlier one. "
                        "Use it to confirm that before asking the user to restate settings or "
                        "declining to make the one change they actually asked for, and to confirm "
                        "a previous configure_*/select_engines call actually took effect.",
        .handler = handleGetTournamentStatus
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_draw_adjudication",
        .description = "Sets draw adjudication for the tournament: mode (off/test/active), "
                        "min_full_moves, required_consecutive_moves, centipawn_threshold. Ends "
                        "games early as a draw once N consecutive moves all stay within a small "
                        "evaluation margin of equal. Every field is independent and optional -- "
                        "pass only what the user actually asked to change; anything not passed "
                        "keeps its previous value (call get_tournament_status first if unsure "
                        "what that is). Disabled (mode=\"off\") by default.",
        .parametersSchema = buildConfigureDrawAdjudicationSchema(),
        .handler = handleConfigureDrawAdjudication
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_resign_adjudication",
        .description = "Sets resign adjudication for the tournament: mode (off/test/active), "
                        "required_consecutive_moves, centipawn_threshold, two_sided. Ends games "
                        "early as a loss for one side once its own evaluation stays badly "
                        "negative for N consecutive moves. Every field is independent and "
                        "optional -- pass only what the user actually asked to change; anything "
                        "not passed keeps its previous value (call get_tournament_status first "
                        "if unsure what that is). Disabled (mode=\"off\") by default.",
        .parametersSchema = buildConfigureResignAdjudicationSchema(),
        .handler = handleConfigureResignAdjudication
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

    registry.registerTool(GuiToolDefinition{
        .name = "stop_tournament",
        .description = "Stops the currently running tournament. Optional \"mode\": \"graceful\" "
                        "(default) finishes games already in progress and starts no new ones; "
                        "\"abrupt\" aborts every in-progress game immediately. Fails if no "
                        "tournament is running.",
        .parametersSchema = buildStopTournamentSchema(),
        .handler = handleStopTournament
    });

    registry.registerTool(GuiToolDefinition{
        .name = "clear_tournament_result",
        .description = "Discards the current tournament's results (and stops it first if it's "
                        "still running). Use when the user wants to throw away what's been "
                        "played so far, e.g. before reconfiguring and starting a fresh "
                        "tournament with the same engines.",
        .handler = handleClearTournamentResult
    });

    registry.registerTool(GuiToolDefinition{
        .name = "show_tournament_result",
        .description = "Displays the current tournament results as a table in the chat, ranked "
                        "by Elo (each engine's score, win percentage, Elo with error margin, and "
                        "games played). This is an action that renders a table control in the "
                        "chat UI -- it isn't for reading the data yourself to describe in your "
                        "own words, so just call it and briefly say you're showing the results, "
                        "rather than restating the numbers from its response. Works both while a "
                        "tournament is running (partial results so far) and after it has "
                        "finished; reports that no results are available yet if nothing has been "
                        "played. This is the ONLY way you ever learn any actual score, standing, "
                        "or Elo number -- you have no other source for them. Never state, type, "
                        "or guess a result yourself instead of calling this; that would be "
                        "fabricated information, not a real result.",
        .handler = handleShowTournamentResult
    });
}

} // namespace QaplaLlm
