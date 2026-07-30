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

#include "gui-tool-sprt.h"
#include "gui-tool-tournament.h" // resolveEngines() -- pure logic, reused as-is
#include "../sprt-tournament-data.h"
#include "../snackbar.h"

#include <sprt/sprt-manager.h>

#include <imgui.h>

#include <chrono>
#include <filesystem>
#include <format>

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;
    using QaplaWindows::SprtTournamentData;

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

    // Looks for the most recent "sprt-tournament"-topic snackbar since countBefore entries
    // existed -- the specific reason SprtTournamentData::startTournament() showed via
    // SnackbarManager for why it silently declined to start (reports failures only that way,
    // same as TournamentData -- see mayStartTournament()/createTournament() in
    // sprt-tournament-data.cpp).
    std::string findRecentSprtSnackbar(std::size_t countBefore) {
        const auto& history = QaplaWindows::SnackbarManager::instance().getHistory();
        for (auto i = history.size(); i > countBefore; --i) {
            if (history[i - 1].topic == "sprt-tournament") {
                return history[i - 1].message;
            }
        }
        return "";
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

    // ------------------------------------------------------------------
    // select_sprt_engines
    // ------------------------------------------------------------------

    Json::JsonValue buildSelectSprtEnginesSchema() {
        auto schema = noArgsToolSchema();
        auto& properties = schema["properties"];

        auto stringProp = [](const std::string& description) {
            auto prop = Json::JsonValue::object();
            prop["type"] = "string";
            prop["description"] = description;
            return prop;
        };

        properties["champion"] = stringProp(
            "The comparison/baseline engine -- the one already trusted, that the challenger is "
            "being tested against. Matched case-insensitively against the installed engine "
            "catalog -- call list_installed_engines first if unsure what's available.");
        properties["challenger"] = stringProp(
            "The engine under test. Must be a different engine than champion.");
        schema["required"] = Json::JsonValue::array();
        schema["required"].push_back("champion");
        schema["required"].push_back("challenger");
        return schema;
    }

    GuiToolResult handleSelectSprtEngines(const Json::JsonValue& arguments) {
        if (!arguments.contains("champion") || !arguments.at("champion").is_string() ||
            !arguments.contains("challenger") || !arguments.at("challenger").is_string()) {
            return GuiToolResult{
                .success = false,
                .content = "Both \"champion\" and \"challenger\" engine names are required."
            };
        }

        auto championOutcome = resolveEngines({arguments.at("champion").as_string()});
        auto challengerOutcome = resolveEngines({arguments.at("challenger").as_string()});

        std::vector<std::string> notFound = championOutcome.notFound;
        notFound.insert(notFound.end(), challengerOutcome.notFound.begin(), challengerOutcome.notFound.end());
        if (!notFound.empty()) {
            return GuiToolResult{
                .success = false,
                .content = "Not installed: " + joinStrings(notFound) +
                    ". Call list_installed_engines to see what's available."
            };
        }

        auto champion = championOutcome.resolved.front();
        auto challenger = challengerOutcome.resolved.front();
        if (champion.getName() == challenger.getName()) {
            return GuiToolResult{.success = false, .content = "Champion and challenger must be two different engines."};
        }
        // resolveEngines() marks both selected + non-gauntlet; SPRT needs exactly one gauntlet
        // engine (the "engine under test") -- see ChatbotStepSprtSelectGauntlet's identical
        // convention: gauntlet=true is the challenger, gauntlet=false is the champion.
        challenger.setGauntlet(true);

        // setEngineConfigurations() self-persists (see ImGuiEngineSelect::notifyConfigurationChanged())
        // and its registered callback keeps SprtTournamentData::engineConfigurations_ in sync --
        // no separate updateConfiguration() call needed here, same as tournament's select_engines.
        std::vector<QaplaTester::EngineConfig> configs{champion, challenger};
        SprtTournamentData::instance().getEngineSelect().setEngineConfigurations(configs);

        return GuiToolResult{
            .success = true,
            .content = "Champion (comparison engine): " + champion.getName() +
                        ". Challenger (engine under test): " + challenger.getName() + "."
        };
    }

    // ------------------------------------------------------------------
    // configure_sprt
    // ------------------------------------------------------------------

    Json::JsonValue buildConfigureSprtSchema() {
        auto schema = noArgsToolSchema();
        auto& properties = schema["properties"];

        auto stringProp = [](const std::string& description) {
            auto prop = Json::JsonValue::object();
            prop["type"] = "string";
            prop["description"] = description;
            return prop;
        };
        auto numberProp = [](const std::string& description) {
            auto prop = Json::JsonValue::object();
            prop["type"] = "number";
            prop["description"] = description;
            return prop;
        };
        auto integerProp = [](const std::string& description) {
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

        properties["time_control"] = timeControlSchemaProperty();
        properties["elo0"] = numberProp(
            "H0 (null hypothesis) Elo bound -- the challenger is considered NOT stronger at or "
            "below this Elo difference. Usually 0. Default 0.");
        properties["elo1"] = numberProp(
            "H1 (alternative hypothesis) Elo bound -- the challenger is considered stronger at "
            "or above this Elo difference. Must end up greater than elo0 (checked when the test "
            "starts, not when this field alone is set). Default 3.");
        properties["alpha"] = numberProp(
            "Type I error rate: probability of accepting the challenger as stronger when it "
            "actually isn't. Between 0 and 1, exclusive. Default 0.05.");
        properties["beta"] = numberProp(
            "Type II error rate: probability of rejecting the challenger when it actually is "
            "stronger. Between 0 and 1, exclusive. Default 0.05.");
        properties["max_games"] = integerProp(
            "Maximum games to play before stopping inconclusively if no decision is reached "
            "sooner. Default 100000.");
        properties["model"] = stringProp(
            "Statistical model used for the SPRT calculation: \"normalized\" (default), "
            "\"logistic\", or \"bayesian\".");
        properties["pentanomial"] = boolProp(
            "If true, use pentanomial (paired-game) statistics instead of trinomial -- more "
            "statistical power, needs games played in same-opening pairs. Default false.");
        properties["openings_file"] = stringProp("Path to an existing EPD or PGN opening book file on disk.");
        properties["pgn_file"] = stringProp("Path to save the played games as PGN.");
        properties["concurrency"] = integerProp("Number of games to run in parallel.");
        return schema;
    }

    void applyTimeControl(SprtTournamentData& data, const std::string& value, std::vector<std::string>& applied) {
        // Not validated: QaplaTester::TimeControl::parse() is deliberately lenient (never
        // throws) -- see the identical note on tournament mode's applyTimeControl().
        data.getGlobalSettings().setTimeControlSettings({.timeControl = value});
        applied.push_back("time control " + value);
    }

    void applyEloLower(SprtTournamentData& data, double value, std::vector<std::string>& applied) {
        data.sprtConfig().eloH0 = static_cast<float>(value);
        applied.push_back(std::format("elo0={:.2f}", data.sprtConfig().eloH0));
    }

    void applyEloUpper(SprtTournamentData& data, double value, std::vector<std::string>& applied) {
        data.sprtConfig().eloH1 = static_cast<float>(value);
        applied.push_back(std::format("elo1={:.2f}", data.sprtConfig().eloH1));
    }

    void applyAlpha(SprtTournamentData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value <= 0.0 || value >= 1.0) {
            problems.push_back("alpha must be between 0 and 1 (exclusive)");
            return;
        }
        data.sprtConfig().alpha = value;
        applied.push_back(std::format("alpha={:.3f}", value));
    }

    void applyBeta(SprtTournamentData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value <= 0.0 || value >= 1.0) {
            problems.push_back("beta must be between 0 and 1 (exclusive)");
            return;
        }
        data.sprtConfig().beta = value;
        applied.push_back(std::format("beta={:.3f}", value));
    }

    void applyMaxGames(SprtTournamentData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value < 1) {
            problems.push_back("max_games must be at least 1");
            return;
        }
        data.sprtConfig().maxGames = static_cast<uint32_t>(value);
        applied.push_back("max_games=" + std::to_string(data.sprtConfig().maxGames));
    }

    void applyModel(SprtTournamentData& data, const std::string& value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value != "normalized" && value != "logistic" && value != "bayesian") {
            problems.push_back("model must be \"normalized\", \"logistic\", or \"bayesian\" (got \"" + value + "\")");
            return;
        }
        data.sprtConfig().model = value;
        applied.push_back("model=" + value);
    }

    void applyPentanomial(SprtTournamentData& data, bool value, std::vector<std::string>& applied) {
        data.sprtConfig().pentanomial = value;
        applied.push_back(std::string("pentanomial=") + (value ? "yes" : "no"));
    }

    void applyOpeningsFile(SprtTournamentData& data, const std::string& path, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (!std::filesystem::exists(path)) {
            problems.push_back("openings file not found: " + path);
            return;
        }
        data.tournamentOpening().openings().file = path;
        applied.push_back("openings file");
    }

    void applyPgnFile(SprtTournamentData& data, const std::string& path, std::vector<std::string>& applied) {
        data.tournamentPgn().pgnOptions().file = path;
        applied.push_back("PGN output file");
    }

    void applyConcurrency(SprtTournamentData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value < 1) {
            problems.push_back("concurrency must be at least 1");
            return;
        }
        auto concurrency = static_cast<uint32_t>(value);
        data.setExternalConcurrency(concurrency);
        applied.push_back("concurrency=" + std::to_string(concurrency));
    }

    GuiToolResult handleConfigureSprt(const Json::JsonValue& arguments) {
        auto& sprtData = SprtTournamentData::instance();
        std::vector<std::string> applied;
        std::vector<std::string> problems;

        if (arguments.contains("time_control") && arguments.at("time_control").is_string()) {
            applyTimeControl(sprtData, arguments.at("time_control").as_string(), applied);
        }
        if (arguments.contains("elo0") && arguments.at("elo0").is_number()) {
            applyEloLower(sprtData, arguments.at("elo0").as_number(), applied);
        }
        if (arguments.contains("elo1") && arguments.at("elo1").is_number()) {
            applyEloUpper(sprtData, arguments.at("elo1").as_number(), applied);
        }
        if (arguments.contains("alpha") && arguments.at("alpha").is_number()) {
            applyAlpha(sprtData, arguments.at("alpha").as_number(), applied, problems);
        }
        if (arguments.contains("beta") && arguments.at("beta").is_number()) {
            applyBeta(sprtData, arguments.at("beta").as_number(), applied, problems);
        }
        if (arguments.contains("max_games") && arguments.at("max_games").is_number()) {
            applyMaxGames(sprtData, arguments.at("max_games").as_number(), applied, problems);
        }
        if (arguments.contains("model") && arguments.at("model").is_string()) {
            applyModel(sprtData, arguments.at("model").as_string(), applied, problems);
        }
        if (arguments.contains("pentanomial") && arguments.at("pentanomial").is_boolean()) {
            applyPentanomial(sprtData, arguments.at("pentanomial").as_boolean(), applied);
        }
        if (arguments.contains("openings_file") && arguments.at("openings_file").is_string()) {
            applyOpeningsFile(sprtData, arguments.at("openings_file").as_string(), applied, problems);
        }
        if (arguments.contains("pgn_file") && arguments.at("pgn_file").is_string()) {
            applyPgnFile(sprtData, arguments.at("pgn_file").as_string(), applied);
        }
        if (arguments.contains("concurrency") && arguments.at("concurrency").is_number()) {
            applyConcurrency(sprtData, arguments.at("concurrency").as_number(), applied, problems);
        }

        // All of the above (except time_control/concurrency, which self-persist) mutate raw
        // references that don't persist on their own -- see
        // ImGuiSprtConfiguration::updateConfiguration()'s doc comment. Calling all three
        // unconditionally is cheap and always correct either way.
        sprtData.sprtConfiguration().updateConfiguration();
        sprtData.tournamentOpening().updateConfiguration();
        sprtData.tournamentPgn().updateConfiguration();

        return buildConfigureResult(applied, problems);
    }

    // ------------------------------------------------------------------
    // configure_sprt_draw_adjudication / configure_sprt_resign_adjudication
    // ------------------------------------------------------------------

    Json::JsonValue buildConfigureSprtDrawAdjudicationSchema() {
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

    GuiToolResult handleConfigureSprtDrawAdjudication(const Json::JsonValue& arguments) {
        auto& sprtData = SprtTournamentData::instance();
        auto& config = sprtData.tournamentAdjudication().drawConfig();
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

        sprtData.tournamentAdjudication().updateConfiguration();
        return buildConfigureResult(applied, problems);
    }

    Json::JsonValue buildConfigureSprtResignAdjudicationSchema() {
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

    GuiToolResult handleConfigureSprtResignAdjudication(const Json::JsonValue& arguments) {
        auto& sprtData = SprtTournamentData::instance();
        auto& config = sprtData.tournamentAdjudication().resignConfig();
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

        sprtData.tournamentAdjudication().updateConfiguration();
        return buildConfigureResult(applied, problems);
    }

    // ------------------------------------------------------------------
    // get_sprt_status
    // ------------------------------------------------------------------

    struct SprtEngineNames {
        std::string champion;
        std::string challenger;
    };

    SprtEngineNames getSprtEngineNames(SprtTournamentData& data) {
        SprtEngineNames names;
        for (const auto& engine : data.getEngineSelect().getSelectedEngines()) {
            if (engine.isGauntlet()) {
                names.challenger = engine.getName();
            } else {
                names.champion = engine.getName();
            }
        }
        return names;
    }

    std::string adjudicationSummary(SprtTournamentData& data) {
        const auto& draw = data.tournamentAdjudication().drawConfig();
        const auto& resign = data.tournamentAdjudication().resignConfig();
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

    GuiToolResult handleGetSprtStatus(const Json::JsonValue&) {
        auto& sprtData = SprtTournamentData::instance();
        auto names = getSprtEngineNames(sprtData);
        const auto& config = sprtData.sprtConfig();
        const auto& openingsFile = sprtData.tournamentOpening().openings().file;
        const auto& pgnFile = sprtData.tournamentPgn().pgnOptions().file;

        std::string runState = "No SPRT test is currently running.";
        if (sprtData.isStarting()) {
            runState = "An SPRT test is currently starting.";
        } else if (sprtData.isRunning()) {
            runState = "An SPRT test is currently running.";
        }
        if (sprtData.isFinished()) {
            runState += " A decision has been reached (or the game limit was hit) -- call "
                         "show_sprt_result to see it.";
        }

        std::string message = std::format(
            "Champion (comparison engine): {}. Challenger (engine under test): {}. "
            "Time control: {}. Elo bounds: H0={:.2f}, H1={:.2f}. Alpha={:.3f}, Beta={:.3f}. "
            "Max games: {}. Model: {}{}. Openings file: {}. PGN output file: {}. Concurrency: {}. {} {}",
            names.champion.empty() ? "(not set)" : names.champion,
            names.challenger.empty() ? "(not set)" : names.challenger,
            sprtData.getGlobalSettings().getTimeControlSettings().timeControl,
            config.eloH0, config.eloH1, config.alpha, config.beta, config.maxGames,
            config.model, config.pentanomial ? " (pentanomial)" : "",
            openingsFile.empty() ? "(not set)" : openingsFile,
            pgnFile.empty() ? "(not set)" : pgnFile,
            sprtData.getExternalConcurrency(),
            runState,
            adjudicationSummary(sprtData));

        return GuiToolResult{.success = true, .content = message};
    }

    // ------------------------------------------------------------------
    // start_sprt_tournament
    // ------------------------------------------------------------------

    GuiToolResult handleStartSprtTournament(const Json::JsonValue&) {
        auto& sprtData = SprtTournamentData::instance();
        if (sprtData.isRunning() || sprtData.isStarting()) {
            return GuiToolResult{
                .success = false,
                .content = "An SPRT test is already running. Stop it first if you want to start a different one."
            };
        }

        auto historyCountBefore = QaplaWindows::SnackbarManager::instance().getHistory().size();

        sprtData.startTournament(); // always verbose, same as the classic "Start" button

        if (!sprtData.isRunning() && !sprtData.isStarting()) {
            auto reason = findRecentSprtSnackbar(historyCountBefore);
            return GuiToolResult{
                .success = false,
                .content = reason.empty() ? "Could not start the SPRT test." : reason
            };
        }

        sprtData.setPoolConcurrency(sprtData.getExternalConcurrency(), true, true);
        return GuiToolResult{.success = true, .content = "SPRT test started."};
    }

    // ------------------------------------------------------------------
    // stop_sprt_tournament
    // ------------------------------------------------------------------

    Json::JsonValue buildStopSprtTournamentSchema() {
        auto schema = noArgsToolSchema();
        auto mode = Json::JsonValue::object();
        mode["type"] = "string";
        auto enumValues = Json::JsonValue::array();
        enumValues.push_back("graceful");
        enumValues.push_back("abrupt");
        mode["enum"] = enumValues;
        mode["description"] =
            "\"graceful\" (default if omitted): let the game currently in progress finish, then "
            "stop. \"abrupt\": abort the in-progress game immediately. If the user just says "
            "\"stop\"/\"end the test\" without qualifying it, use \"graceful\".";
        schema["properties"]["mode"] = mode;
        return schema;
    }

    GuiToolResult handleStopSprtTournament(const Json::JsonValue& arguments) {
        auto& sprtData = SprtTournamentData::instance();
        if (!sprtData.isRunning() && !sprtData.isStarting()) {
            return GuiToolResult{.success = false, .content = "No SPRT test is currently running."};
        }

        bool graceful = true;
        if (arguments.contains("mode") && arguments.at("mode").is_string()) {
            graceful = arguments.at("mode").as_string() != "abrupt";
        }

        sprtData.stopPool(graceful);
        return GuiToolResult{
            .success = true,
            .content = graceful
                ? "Stopping the SPRT test gracefully: the game already in progress will be "
                  "finished, no new games will start."
                : "Stopping the SPRT test abruptly: the in-progress game is being aborted "
                  "immediately."
        };
    }

    // ------------------------------------------------------------------
    // clear_sprt_result
    // ------------------------------------------------------------------

    GuiToolResult handleClearSprtResult(const Json::JsonValue&) {
        auto& sprtData = SprtTournamentData::instance();
        if (!sprtData.hasResults()) {
            return GuiToolResult{.success = true, .content = "There are no SPRT results to clear."};
        }

        bool wasRunning = sprtData.isRunning() || sprtData.isStarting();
        sprtData.clear();
        return GuiToolResult{
            .success = true,
            .content = wasRunning
                ? "SPRT test stopped and all results cleared."
                : "All SPRT results have been cleared."
        };
    }

    // ------------------------------------------------------------------
    // show_sprt_result
    // ------------------------------------------------------------------

    GuiToolResult handleShowSprtResult(const Json::JsonValue&) {
        auto& sprtData = SprtTournamentData::instance();
        if (!sprtData.hasResults()) {
            return GuiToolResult{.success = true, .content = "No SPRT results are available yet."};
        }

        // Renders the same two live controls the classic (non-AI) chatbot's SPRT results step
        // draws (see ChatbotStepSprtTournamentResult::draw()): the SPRT test table (LLR/bounds/
        // decision status) and the raw duel win/draw/loss table -- real ImGuiTables, not a text
        // dump. Always reads SprtTournamentData::instance() fresh at draw time, so it reflects
        // the test's current state on every frame, exactly like that classic step does.
        return GuiToolResult{
            .success = true,
            .content = "Showing the current SPRT results as tables in the chat -- they are "
                        "already visible to the user, so do not restate, list, or summarize the "
                        "numbers in your reply; just briefly confirm what you did. This is the "
                        "ONLY way you ever learn the actual SPRT decision or duel score -- never "
                        "state, type, or guess one yourself instead of calling this.",
            .renderWidget = []() {
                auto& data = SprtTournamentData::instance();
                ImGui::Text("SPRT Test Result:");
                data.drawSprtTable(ImVec2(0.0F, 100.0F));
                ImGui::Spacing();
                ImGui::Text("Duel Result:");
                data.drawResultTable(ImVec2(0.0F, 100.0F));
            }
        };
    }
}

void registerSprtTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "select_sprt_engines",
        .description = "Selects the two engines for an SPRT test: \"champion\" (comparison "
                        "baseline) and \"challenger\" (engine under test), replacing any "
                        "previous selection. Names are matched case-insensitively against the "
                        "installed engine catalog -- call list_installed_engines first if unsure "
                        "what's available. This is entirely separate from select_engines/"
                        "configure_tournament (classic tournament mode) -- selecting SPRT "
                        "engines never changes the tournament's engine selection, and vice "
                        "versa.",
        .parametersSchema = buildSelectSprtEnginesSchema(),
        .handler = handleSelectSprtEngines
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_sprt",
        .description = "Sets SPRT test options: time_control, elo0/elo1 (H0/H1 Elo bounds), "
                        "alpha, beta, max_games, model, pentanomial, openings_file, pgn_file, "
                        "concurrency. Every field is independent and optional -- pass ONLY the "
                        "one thing the user asked to change; do not require or ask for any of "
                        "the other fields first. Anything not passed here keeps whatever it was "
                        "already set to (this session or an earlier one) -- call get_sprt_status "
                        "first if you're not sure what that currently is. IMPORTANT: this is a "
                        "completely separate, independent configuration from configure_tournament "
                        "-- the classic tournament and SPRT each have their own time control, "
                        "openings file, concurrency, etc., even though the field names look the "
                        "same. If the user's message doesn't make it clear whether they mean the "
                        "tournament or an SPRT test (e.g. just \"set the time control to 1 "
                        "minute per game\" out of nowhere), infer it from the conversation so far "
                        "(are they discussing a multi-engine tournament or a champion-vs-"
                        "challenger SPRT test?); if it's genuinely still unclear even from "
                        "context, ask which one they mean rather than guessing -- guessing wrong "
                        "silently configures the other feature instead. openings_file must be "
                        "set (here or in an earlier session) before start_sprt_tournament will "
                        "succeed.",
        .parametersSchema = buildConfigureSprtSchema(),
        .handler = handleConfigureSprt
    });

    registry.registerTool(GuiToolDefinition{
        .name = "get_sprt_status",
        .description = "Reports the SPRT test configuration and state as currently set up: "
                        "champion/challenger engines, time control, Elo bounds/alpha/beta/"
                        "max_games/model, openings file, PGN output file, concurrency, draw/"
                        "resign adjudication settings, and whether a test is running or "
                        "finished. This is entirely separate from get_tournament_status (classic "
                        "tournament mode) -- call this one specifically for SPRT questions. Call "
                        "it FIRST whenever a request only changes one SPRT setting and you're "
                        "not certain everything else is already configured, or whenever it's "
                        "unclear whether the user means the tournament or SPRT (comparing "
                        "against get_tournament_status can help you tell which one they're "
                        "already mid-configuring).",
        .handler = handleGetSprtStatus
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_sprt_draw_adjudication",
        .description = "Sets draw adjudication for the SPRT test (separate from the classic "
                        "tournament's configure_draw_adjudication): mode (off/test/active), "
                        "min_full_moves, required_consecutive_moves, centipawn_threshold. Every "
                        "field is independent and optional -- pass only what the user actually "
                        "asked to change. Disabled (mode=\"off\") by default.",
        .parametersSchema = buildConfigureSprtDrawAdjudicationSchema(),
        .handler = handleConfigureSprtDrawAdjudication
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_sprt_resign_adjudication",
        .description = "Sets resign adjudication for the SPRT test (separate from the classic "
                        "tournament's configure_resign_adjudication): mode (off/test/active), "
                        "required_consecutive_moves, centipawn_threshold, two_sided. Every field "
                        "is independent and optional -- pass only what the user actually asked "
                        "to change. Disabled (mode=\"off\") by default.",
        .parametersSchema = buildConfigureSprtResignAdjudicationSchema(),
        .handler = handleConfigureSprtResignAdjudication
    });

    registry.registerTool(GuiToolDefinition{
        .name = "start_sprt_tournament",
        .description = "Starts the SPRT test with the engines and settings configured via "
                        "select_sprt_engines/configure_sprt. Requires exactly a champion and a "
                        "challenger to already be selected and an openings file to already be "
                        "configured; the result tells you exactly which precondition is missing "
                        "if it can't start.",
        .handler = handleStartSprtTournament,
        // Engine processes need to launch and initialize; a handful of
        // engines can legitimately take longer than the default 30s.
        .timeout = std::chrono::seconds(60)
    });

    registry.registerTool(GuiToolDefinition{
        .name = "stop_sprt_tournament",
        .description = "Stops the currently running SPRT test. Optional \"mode\": \"graceful\" "
                        "(default) finishes the game already in progress and starts no new one; "
                        "\"abrupt\" aborts the in-progress game immediately. Fails if no SPRT "
                        "test is running.",
        .parametersSchema = buildStopSprtTournamentSchema(),
        .handler = handleStopSprtTournament
    });

    registry.registerTool(GuiToolDefinition{
        .name = "clear_sprt_result",
        .description = "Discards the current SPRT test's results (and stops it first if it's "
                        "still running). Use when the user wants to throw away progress so far, "
                        "e.g. before reconfiguring and starting a fresh test.",
        .handler = handleClearSprtResult
    });

    registry.registerTool(GuiToolDefinition{
        .name = "show_sprt_result",
        .description = "Displays the current SPRT test results as tables in the chat: the SPRT "
                        "decision table (LLR value, bounds, pass/fail/running status) and the "
                        "raw win/draw/loss duel table. This is an action that renders table "
                        "controls in the chat UI -- it isn't for reading the data yourself to "
                        "describe in your own words, so just call it and briefly say you're "
                        "showing the results, rather than restating the numbers from its "
                        "response. Works both while a test is running (partial results so far) "
                        "and after it has finished; reports that no results are available yet if "
                        "nothing has been played. This is the ONLY way you ever learn any actual "
                        "SPRT decision or duel score -- you have no other source for them. Never "
                        "state, type, or guess a result yourself instead of calling this; that "
                        "would be fabricated information, not a real result.",
        .handler = handleShowSprtResult
    });
}

} // namespace QaplaLlm
