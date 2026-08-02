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
#include "../os-dialogs.h"
#include "../callback-manager.h"

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

    // Tab-only variant of the classic (non-AI) tournament chatbot flow's "Switch to Tournament
    // View" message (see chatbot-step-tournament-start.cpp). ImGuiTabBar flips to the Tournament
    // tab on the next frame just like for "switch_to_tournament_view", but TournamentData does
    // NOT listen for this message, so it does not also activate a running game's board window.
    // Called whenever a tool actually changes tournament settings or its run state, so the user
    // sees what the AI just did without a board popping up and covering the AI chat.
    void switchToTournamentView() {
        QaplaWindows::StaticCallbacks::message().invokeAll("switch_to_tournament_tab");
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
        engines["description"] = "Engine display names, e.g. [\"Stockfish\",\"Qapla\"].";
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
        if (!outcome.ambiguous.empty()) {
            return GuiToolResult{
                .success = false,
                .content = formatAmbiguousEngineNames(outcome.ambiguous) + " Ask the user which one they mean."
            };
        }
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
        switchToTournamentView();
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
            "Games per engine pairing PER ROUND, not tournament total. Total per pairing = "
            "games*rounds, applies to every pairing. User gives one number, no rounds mention "
            "-> set games to that, leave rounds at default 1, so total=what they said. User "
            "gives BOTH total games and rounds (e.g. \"100 games total, 10 rounds\") -> compute "
            "games=total/rounds yourself (100/10 -> games=10, rounds=10), never put total as-is "
            "into games. If unclear whether given count is total or per-round (e.g. \"100 games "
            "and 10 rounds\", not specified which), ask user, never guess.");
        properties["rounds"] = integerProp(
            "Times full pairing set repeats. Default 1. See \"games\" for how this multiplies "
            "into tournament total.");
        properties["event"] = stringProp("Tournament/event name.");
        properties["openings_file"] = stringProp("Path to existing EPD/PGN opening book file on disk.");
        properties["pgn_file"] = stringProp("Path to save played games as PGN.");
        properties["concurrency"] = integerProp("Games run in parallel.");
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
            problems.push_back("openings file not found: " + path +
                " -- call open_tournament_openings_file_dialog to let the user pick a valid one");
            return;
        }
        data.tournamentOpening().openings().file = path;
        applied.push_back("openings file");
    }

    GuiToolResult handleOpenTournamentOpeningsFileDialog(const Json::JsonValue&) {
        auto paths = QaplaWindows::OsDialogs::openFileDialog(false);
        if (paths.empty()) {
            return GuiToolResult{
                .success = true,
                .content = "The user cancelled the dialog; the openings file was not changed."
            };
        }

        auto& tournamentData = TournamentData::instance();
        tournamentData.tournamentOpening().openings().file = paths.front();
        tournamentData.tournamentOpening().updateConfiguration();
        switchToTournamentView();
        return GuiToolResult{.success = true, .content = "Openings file set to: " + paths.front()};
    }

    GuiToolResult handleOpenTournamentPgnFileDialog(const Json::JsonValue&) {
        auto& tournamentData = TournamentData::instance();
        auto path = QaplaWindows::OsDialogs::saveFileDialog({{"PGN files (*.pgn)", "pgn"}}, tournamentData.pgnConfig().file);
        if (path.empty()) {
            return GuiToolResult{
                .success = true,
                .content = "The user cancelled the dialog; the PGN output file was not changed."
            };
        }

        tournamentData.pgnConfig().file = path;
        tournamentData.tournamentPgn().updateConfiguration();
        switchToTournamentView();
        return GuiToolResult{.success = true, .content = "PGN output file set to: " + path};
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

        if (!applied.empty()) {
            switchToTournamentView();
        }
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
            "\"off\": disables draw adjudication. \"test\": evaluates/logs decision, doesn't end "
            "games. \"active\": ends games early as draw once conditions below met.");
        properties["min_full_moves"] = intProp(
            "Min full moves before draw adjudication can trigger. Default 80.");
        properties["required_consecutive_moves"] = intProp(
            "Consecutive moves (engines' own eval) that must stay within centipawn_threshold of "
            "equal before draw adjudication. Default 20.");
        properties["centipawn_threshold"] = intProp(
            "Max abs eval in centipawns still counting as drawn (e.g. 20 = within +/-20cp of "
            "equal). Positive number. Default 20.");
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
        if (!applied.empty()) {
            switchToTournamentView();
        }
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
            "\"off\": disables resign adjudication. \"test\": evaluates/logs decision, doesn't "
            "end games. \"active\": ends games early as loss for losing side once conditions "
            "below met.");
        properties["required_consecutive_moves"] = intProp(
            "Consecutive moves whose own eval must stay at/below -centipawn_threshold (that bad "
            "or worse) before resign adjudication. Default 5.");
        properties["centipawn_threshold"] = intProp(
            "How bad, in centipawns from losing side's own view, before resignation-worthy -- "
            "e.g. 500 = down ~queen's worth. Positive magnitude, not negative. Default 500.");
        properties["two_sided"] = boolProp(
            "If true, both engines must independently agree position lost before adjudicating "
            "-- more conservative, avoids resignation from one engine's eval blunder alone. "
            "Default false.");
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
        if (!applied.empty()) {
            switchToTournamentView();
        }
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
            "\"graceful\" (default): finish in-progress games, then stop, no new games start. "
            "\"abrupt\": abort all in-progress games immediately. If user just says "
            "\"stop\"/\"end tournament\" unqualified, use \"graceful\" -- ask only if they've "
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
        switchToTournamentView();
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
        switchToTournamentView();
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
        switchToTournamentView();
        return GuiToolResult{.success = true, .content = "Tournament started."};
    }
}

void registerTournamentTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "select_engines",
        .description = "Selects configured engines for next tournament, replaces previous "
                        "selection. Names matched case-insensitively vs installed engine "
                        "catalog; informal/short name (e.g. \"spike\") auto-matched to the one "
                        "engine it can mean (e.g. \"Spike 1.4.1\") -- pass name user said, no "
                        "need to call list_installed_engines first for exact name. If name "
                        "matches multiple engines, result lists candidates -- ask user which, "
                        "never guess. Sets up round-robin (every engine plays every other); "
                        "gauntlet mode not supported via chat.",
        .parametersSchema = buildSelectEnginesSchema(),
        .handler = handleSelectEngines
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_tournament",
        .description = "Sets tournament options: time_control, games (per pairing), rounds, "
                        "event (name), openings_file, pgn_file, concurrency. Each field "
                        "independent/optional -- pass ONLY what user asked to change (e.g. just "
                        "\"games\"); don't require/ask other fields first. Unpassed fields keep "
                        "prior value (this session or earlier) -- call get_tournament_status "
                        "first if unsure, don't assume unset. openings_file must be set (here "
                        "or earlier session) before start_tournament succeeds, no safe default. "
                        "If missing/invalid or user wants to browse, call "
                        "open_tournament_openings_file_dialog instead of asking for typed path "
                        "-- same for pgn_file/open_tournament_pgn_file_dialog.",
        .parametersSchema = buildConfigureTournamentSchema(),
        .handler = handleConfigureTournament
    });

    registry.registerTool(GuiToolDefinition{
        .name = "open_tournament_openings_file_dialog",
        .description = "Opens GUI's native file picker for user to choose tournament's openings "
                        "file -- you have no filesystem access, never guess/invent a path. Use "
                        "instead of asking user to type/paste path whenever missing, invalid, "
                        "or user wants to browse. Chosen path applied immediately, same as "
                        "configure_tournament's openings_file.",
        .handler = handleOpenTournamentOpeningsFileDialog,
        // Waits on the user picking a file in a native dialog, which can legitimately take
        // much longer than a normal tool call -- see open_add_engine_dialog's identical timeout.
        .timeout = std::chrono::minutes(10)
    });

    registry.registerTool(GuiToolDefinition{
        .name = "open_tournament_pgn_file_dialog",
        .description = "Opens GUI's native save-file picker for user to choose tournament's PGN "
                        "output path -- you have no filesystem access, never guess/invent a "
                        "path. Use instead of asking user to type/paste path. Chosen path "
                        "applied immediately, same as configure_tournament's pgn_file.",
        .handler = handleOpenTournamentPgnFileDialog,
        .timeout = std::chrono::minutes(10)
    });

    registry.registerTool(GuiToolDefinition{
        .name = "get_tournament_status",
        .description = "Reports current tournament config/state: selected engines, time "
                        "control, games/rounds, event name, openings file, PGN output file, "
                        "concurrency, draw/resign adjudication settings, whether running. Call "
                        "FIRST when request changes only one thing (e.g. \"set it to 10 games\", "
                        "\"turn on resign adjudication\") and rest of config uncertain -- almost "
                        "always already set, this session or earlier. Use to confirm before "
                        "asking user to restate settings or declining requested change, and to "
                        "confirm a prior configure_*/select_engines call took effect.",
        .handler = handleGetTournamentStatus
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_draw_adjudication",
        .description = "Sets draw adjudication: mode (off/test/active), min_full_moves, "
                        "required_consecutive_moves, centipawn_threshold. Ends game early as "
                        "draw once N consecutive moves stay within small eval margin of equal. "
                        "Each field independent/optional -- pass only what user asked to "
                        "change; unpassed keeps prior value (call get_tournament_status first "
                        "if unsure). Disabled (mode=\"off\") by default.",
        .parametersSchema = buildConfigureDrawAdjudicationSchema(),
        .handler = handleConfigureDrawAdjudication
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_resign_adjudication",
        .description = "Sets resign adjudication: mode (off/test/active), "
                        "required_consecutive_moves, centipawn_threshold, two_sided. Ends game "
                        "early as loss for one side once its own eval stays badly negative for "
                        "N consecutive moves. Each field independent/optional -- pass only what "
                        "user asked to change; unpassed keeps prior value (call "
                        "get_tournament_status first if unsure). Disabled (mode=\"off\") by "
                        "default.",
        .parametersSchema = buildConfigureResignAdjudicationSchema(),
        .handler = handleConfigureResignAdjudication
    });

    registry.registerTool(GuiToolDefinition{
        .name = "start_tournament",
        .description = "Starts tournament with engines/settings from "
                        "select_engines/configure_tournament. Requires at least two selected "
                        "engines and openings file already configured; result states exactly "
                        "which precondition missing if it can't start.",
        .handler = handleStartTournament,
        // Engine processes need to launch and initialize; a handful of
        // engines can legitimately take longer than the default 30s.
        .timeout = std::chrono::seconds(60)
    });

    registry.registerTool(GuiToolDefinition{
        .name = "stop_tournament",
        .description = "Stops running tournament. Optional \"mode\": \"graceful\" (default) "
                        "finishes in-progress games, starts no new ones; \"abrupt\" aborts "
                        "every in-progress game immediately. Fails if no tournament running.",
        .parametersSchema = buildStopTournamentSchema(),
        .handler = handleStopTournament
    });

    registry.registerTool(GuiToolDefinition{
        .name = "clear_tournament_result",
        .description = "Discards current tournament results (stops it first if still running). "
                        "Use when user wants to discard what's been played so far, e.g. before "
                        "reconfiguring/starting fresh tournament with same engines.",
        .handler = handleClearTournamentResult
    });

    registry.registerTool(GuiToolDefinition{
        .name = "show_tournament_result",
        .description = "Displays current tournament results as table in chat, ranked by Elo "
                        "(score, win%, Elo w/ error margin, games played per engine). Renders "
                        "table control in chat UI -- not for reading data yourself to describe "
                        "in own words, just call it and briefly confirm you're showing results, "
                        "don't restate numbers. Works while running (partial results) or after "
                        "finish; reports none available if nothing played yet. ONLY way you "
                        "ever learn any actual score/standing/Elo -- no other source. Never "
                        "state/type/guess a result yourself instead of calling this -- that's "
                        "fabrication, not a real result.",
        .handler = handleShowTournamentResult
    });
}

} // namespace QaplaLlm
