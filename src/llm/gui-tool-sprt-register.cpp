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
#include "../os-dialogs.h"
#include "../callback-manager.h"

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

    // Tab-only variant of the classic (non-AI) SPRT chatbot flow's "Switch to SPRT View" message
    // (see chatbot-step-tournament-start.cpp, shared with the tournament flow). ImGuiTabBar flips
    // to the SPRT tab on the next frame just like for "switch_to_sprt_view", but
    // SprtTournamentData does NOT listen for this message, so it does not also activate the
    // running SPRT board window. Called whenever a tool actually changes SPRT settings or its
    // run state, so the user sees what the AI just did without a board popping up and covering
    // the AI chat.
    void switchToSprtView() {
        QaplaWindows::StaticCallbacks::message().invokeAll("switch_to_sprt_tab");
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

    // adjudicationModeText()/adjudicationModeSchemaProperty()/applyAdjudicationMode() live in
    // gui-tool-registry.h -- shared with the tournament adjudication fields
    // (gui-tool-tournament-register.cpp) so the off/test/active vocabulary and its bool-pair
    // mapping is written exactly once.
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

    // sprtData.isRunning() is true for every non-Stopped state (Starting/Running/
    // GracefulStopping alike), so it can't tell those apart on its own; state() can.
    // Distinguishing GracefulStopping matters since a test in that state is still actively
    // finishing its in-progress game, just declining to start a new one -- reporting it as
    // plain "running" would hide that a stop was already requested. Unlike tournament mode,
    // there's no separate abrupt "Stopping" state -- an abrupt stop finishes synchronously,
    // straight to Stopped.
    std::string sprtRunStateText(SprtTournamentData& sprtData) {
        switch (sprtData.state()) {
            case SprtTournamentData::State::Starting:
                return "An SPRT test is currently starting.";
            case SprtTournamentData::State::Running:
                return "An SPRT test is currently running.";
            case SprtTournamentData::State::GracefulStopping:
                return "An SPRT test is currently running but stopping gracefully -- the "
                       "in-progress game will finish, no new one will start.";
            case SprtTournamentData::State::Stopped:
            default:
                return "No SPRT test is currently running.";
        }
    }

    // Shared by handleGetSprtStatus (dispatched into by the unified get_status tool) and
    // configure_sprt's result (the latter always ends with the full status too -- see
    // buildConfigureResult -- so the model never needs a separate get_status round-trip just to
    // confirm what it changed).
    std::string buildSprtStatusText(SprtTournamentData& sprtData) {
        auto names = getSprtEngineNames(sprtData);
        const auto& config = sprtData.sprtConfig();
        const auto& openingsFile = sprtData.tournamentOpening().openings().file;
        const auto& pgnFile = sprtData.tournamentPgn().pgnOptions().file;

        std::string runState = sprtRunStateText(sprtData);
        if (sprtData.isFinished()) {
            runState += " A decision has been reached (or the game limit was hit) -- call "
                         "show_result (type=\"sprt\") to see it.";
        }

        return std::format(
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
    }

    GuiToolResult buildConfigureResult(SprtTournamentData& sprtData, const std::vector<std::string>& problems,
        bool dialogOpened = false) {
        std::string message;
        if (!problems.empty()) {
            message = "Problems: " + joinStrings(problems) + ". ";
        }
        message += buildSprtStatusText(sprtData);
        return GuiToolResult{.success = problems.empty(), .content = message, .renderWidget = nullptr,
            .terminal = dialogOpened};
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
            "Comparison/baseline engine -- trusted one, challenger tested against it. Matched "
            "case-insensitive vs installed engine catalog -- call list_installed_engines first "
            "if unsure what's available.");
        properties["challenger"] = stringProp(
            "Engine under test. Must differ from champion.");
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

        std::vector<AmbiguousEngineName> ambiguous = championOutcome.ambiguous;
        ambiguous.insert(ambiguous.end(), challengerOutcome.ambiguous.begin(), challengerOutcome.ambiguous.end());
        if (!ambiguous.empty()) {
            return GuiToolResult{
                .success = false,
                .content = formatAmbiguousEngineNames(ambiguous) + " Ask the user which one they mean."
            };
        }

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

        switchToSprtView();
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
            "H0 (null hypothesis) Elo bound -- challenger NOT stronger at/below this Elo diff. "
            "Usually 0. Default 0.");
        properties["elo1"] = numberProp(
            "H1 (alt hypothesis) Elo bound -- challenger stronger at/above this Elo diff. Must "
            "end up > elo0 (checked at test start, not when this field alone set). Default 3.");
        properties["alpha"] = numberProp(
            "Type I error rate: prob. of accepting challenger as stronger when it isn't. 0-1 "
            "exclusive. Default 0.05.");
        properties["beta"] = numberProp(
            "Type II error rate: prob. of rejecting challenger when it is stronger. 0-1 "
            "exclusive. Default 0.05.");
        properties["max_games"] = integerProp(
            "Max games before stopping inconclusive if no decision sooner. Default 100000.");
        properties["model"] = stringProp(
            "Statistical model for SPRT calc: \"normalized\" (default), \"logistic\", "
            "\"bayesian\".");
        properties["pentanomial"] = boolProp(
            "True: pentanomial (paired-game) stats instead of trinomial -- more power, needs "
            "games in same-opening pairs. Default false.");
        properties["openings_file"] = stringProp("Path to existing EPD/PGN opening book file on disk.");
        properties["openings_file_dialog"] = boolProp(
            "Set true to open a native file picker instead of passing openings_file.");
        properties["pgn_file"] = stringProp("Path to save played games as PGN.");
        properties["pgn_file_dialog"] = boolProp(
            "Set true to open a native save-file picker instead of passing pgn_file.");
        properties["concurrency"] = integerProp("Games to run in parallel.");
        properties["draw_mode"] = adjudicationModeSchemaProperty(
            "\"off\": disables draw adjudication. \"test\": evaluates/logs decision, doesn't end "
            "games. \"active\": ends games early as draw once conditions below met.");
        properties["draw_min_full_moves"] = integerProp(
            "Min full moves before draw adjudication can trigger. Default 80.");
        properties["draw_required_consecutive_moves"] = integerProp(
            "Consecutive moves (engines' own eval) that must stay within draw_centipawn_threshold "
            "of equal before draw adjudication. Default 20.");
        properties["draw_centipawn_threshold"] = integerProp(
            "Max abs eval in centipawns still counting as drawn (e.g. 20 = within +/-20cp of "
            "equal). Positive number. Default 20.");
        properties["resign_mode"] = adjudicationModeSchemaProperty(
            "\"off\": disables resign adjudication. \"test\": evaluates/logs decision, doesn't "
            "end games. \"active\": ends games early as loss for losing side once conditions "
            "below met.");
        properties["resign_required_consecutive_moves"] = integerProp(
            "Consecutive moves whose own eval must stay at/below -resign_centipawn_threshold "
            "(that bad or worse) before resign adjudication. Default 5.");
        properties["resign_centipawn_threshold"] = integerProp(
            "How bad, in centipawns from losing side's own view, before resignation-worthy -- "
            "e.g. 500 = down ~queen's worth. Positive magnitude, not negative. Default 500.");
        properties["resign_two_sided"] = boolProp(
            "If true, both engines must independently agree position lost before adjudicating "
            "-- more conservative, avoids resignation from one engine's eval blunder alone. "
            "Default false.");
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
            problems.push_back("openings file not found: " + path +
                " -- set openings_file_dialog=true instead to let the user pick a valid one");
            return;
        }
        data.tournamentOpening().openings().file = path;
        applied.push_back("openings file: " + path);
    }

    void openOpeningsFileDialog(SprtTournamentData& data, std::vector<std::string>& applied) {
        auto paths = QaplaWindows::OsDialogs::openFileDialog(false);
        if (paths.empty()) {
            applied.push_back("openings file (dialog cancelled, unchanged)");
            return;
        }
        data.tournamentOpening().openings().file = paths.front();
        applied.push_back("openings file selected: " + paths.front());
    }

    void applyPgnFile(SprtTournamentData& data, const std::string& path, std::vector<std::string>& applied) {
        data.tournamentPgn().pgnOptions().file = path;
        applied.push_back("PGN output file: " + path);
    }

    void openPgnFileDialog(SprtTournamentData& data, std::vector<std::string>& applied) {
        auto path = QaplaWindows::OsDialogs::saveFileDialog(
            {{"PGN files (*.pgn)", "pgn"}}, data.tournamentPgn().pgnOptions().file);
        if (path.empty()) {
            applied.push_back("PGN output file (dialog cancelled, unchanged)");
            return;
        }
        data.tournamentPgn().pgnOptions().file = path;
        applied.push_back("PGN output file selected: " + path);
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

    void applyDrawMode(SprtTournamentData& data, const std::string& mode, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        auto& config = data.tournamentAdjudication().drawConfig();
        if (applyAdjudicationMode(mode, config.active, config.testOnly, problems)) {
            applied.push_back("draw adjudication mode=" + mode);
        }
    }

    void applyDrawMinFullMoves(SprtTournamentData& data, double value, std::vector<std::string>& applied) {
        auto& config = data.tournamentAdjudication().drawConfig();
        config.minFullMoves = static_cast<uint32_t>(value);
        applied.push_back("draw min full moves=" + std::to_string(config.minFullMoves));
    }

    void applyDrawRequiredConsecutiveMoves(SprtTournamentData& data, double value, std::vector<std::string>& applied) {
        auto& config = data.tournamentAdjudication().drawConfig();
        config.requiredConsecutiveMoves = static_cast<uint32_t>(value);
        applied.push_back("draw required consecutive moves=" + std::to_string(config.requiredConsecutiveMoves));
    }

    void applyDrawCentipawnThreshold(SprtTournamentData& data, double value, std::vector<std::string>& applied) {
        auto& config = data.tournamentAdjudication().drawConfig();
        config.centipawnThreshold = static_cast<int>(value);
        applied.push_back("draw centipawn threshold=" + std::to_string(config.centipawnThreshold));
    }

    void applyResignMode(SprtTournamentData& data, const std::string& mode, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        auto& config = data.tournamentAdjudication().resignConfig();
        if (applyAdjudicationMode(mode, config.active, config.testOnly, problems)) {
            applied.push_back("resign adjudication mode=" + mode);
        }
    }

    void applyResignRequiredConsecutiveMoves(SprtTournamentData& data, double value, std::vector<std::string>& applied) {
        auto& config = data.tournamentAdjudication().resignConfig();
        config.requiredConsecutiveMoves = static_cast<uint32_t>(value);
        applied.push_back("resign required consecutive moves=" + std::to_string(config.requiredConsecutiveMoves));
    }

    void applyResignCentipawnThreshold(SprtTournamentData& data, double value, std::vector<std::string>& applied) {
        auto& config = data.tournamentAdjudication().resignConfig();
        config.centipawnThreshold = static_cast<int>(value);
        applied.push_back("resign centipawn threshold=" + std::to_string(config.centipawnThreshold));
    }

    void applyResignTwoSided(SprtTournamentData& data, bool value, std::vector<std::string>& applied) {
        auto& config = data.tournamentAdjudication().resignConfig();
        config.twoSided = value;
        applied.push_back(std::string("resign two-sided=") + (value ? "yes" : "no"));
    }

    GuiToolResult handleConfigureSprt(const Json::JsonValue& arguments) {
        auto& sprtData = SprtTournamentData::instance();
        std::vector<std::string> applied;
        std::vector<std::string> problems;
        bool dialogOpened = false;

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
        if (arguments.contains("openings_file_dialog") && arguments.at("openings_file_dialog").is_boolean() &&
            arguments.at("openings_file_dialog").as_boolean()) {
            openOpeningsFileDialog(sprtData, applied);
            dialogOpened = true;
        } else if (arguments.contains("openings_file") && arguments.at("openings_file").is_string()) {
            applyOpeningsFile(sprtData, arguments.at("openings_file").as_string(), applied, problems);
        }
        if (arguments.contains("pgn_file_dialog") && arguments.at("pgn_file_dialog").is_boolean() &&
            arguments.at("pgn_file_dialog").as_boolean()) {
            openPgnFileDialog(sprtData, applied);
            dialogOpened = true;
        } else if (arguments.contains("pgn_file") && arguments.at("pgn_file").is_string()) {
            applyPgnFile(sprtData, arguments.at("pgn_file").as_string(), applied);
        }
        if (arguments.contains("concurrency") && arguments.at("concurrency").is_number()) {
            applyConcurrency(sprtData, arguments.at("concurrency").as_number(), applied, problems);
        }
        if (arguments.contains("draw_mode") && arguments.at("draw_mode").is_string()) {
            applyDrawMode(sprtData, arguments.at("draw_mode").as_string(), applied, problems);
        }
        if (arguments.contains("draw_min_full_moves") && arguments.at("draw_min_full_moves").is_number()) {
            applyDrawMinFullMoves(sprtData, arguments.at("draw_min_full_moves").as_number(), applied);
        }
        if (arguments.contains("draw_required_consecutive_moves") &&
            arguments.at("draw_required_consecutive_moves").is_number()) {
            applyDrawRequiredConsecutiveMoves(sprtData, arguments.at("draw_required_consecutive_moves").as_number(), applied);
        }
        if (arguments.contains("draw_centipawn_threshold") && arguments.at("draw_centipawn_threshold").is_number()) {
            applyDrawCentipawnThreshold(sprtData, arguments.at("draw_centipawn_threshold").as_number(), applied);
        }
        if (arguments.contains("resign_mode") && arguments.at("resign_mode").is_string()) {
            applyResignMode(sprtData, arguments.at("resign_mode").as_string(), applied, problems);
        }
        if (arguments.contains("resign_required_consecutive_moves") &&
            arguments.at("resign_required_consecutive_moves").is_number()) {
            applyResignRequiredConsecutiveMoves(sprtData, arguments.at("resign_required_consecutive_moves").as_number(), applied);
        }
        if (arguments.contains("resign_centipawn_threshold") && arguments.at("resign_centipawn_threshold").is_number()) {
            applyResignCentipawnThreshold(sprtData, arguments.at("resign_centipawn_threshold").as_number(), applied);
        }
        if (arguments.contains("resign_two_sided") && arguments.at("resign_two_sided").is_boolean()) {
            applyResignTwoSided(sprtData, arguments.at("resign_two_sided").as_boolean(), applied);
        }

        // All of the above (except time_control/concurrency, which self-persist) mutate raw
        // references that don't persist on their own -- see
        // ImGuiSprtConfiguration::updateConfiguration()'s doc comment. Calling all
        // unconditionally is cheap and always correct either way.
        sprtData.sprtConfiguration().updateConfiguration();
        sprtData.tournamentOpening().updateConfiguration();
        sprtData.tournamentPgn().updateConfiguration();
        sprtData.tournamentAdjudication().updateConfiguration();

        if (!applied.empty()) {
            switchToSprtView();
        }
        return buildConfigureResult(sprtData, problems, dialogOpened);
    }

} // namespace

// Exported (see gui-tool-sprt.h) so the unified get_status/clear_result/show_result tools
// (gui-tool-status-register.cpp) can dispatch into them directly by type --
// get_sprt_status/clear_sprt_result/show_sprt_result no longer exist as separate model-visible
// tools, but the underlying logic is unchanged.
GuiToolResult handleGetSprtStatus(const QaplaTester::Json::JsonValue&) {
    return GuiToolResult{.success = true, .content = buildSprtStatusText(SprtTournamentData::instance())};
}

GuiToolResult handleClearSprtResult(const QaplaTester::Json::JsonValue&) {
    auto& sprtData = SprtTournamentData::instance();
    if (!sprtData.hasResults()) {
        return GuiToolResult{.success = true, .content = "There are no SPRT results to clear."};
    }

    bool wasRunning = sprtData.isRunning() || sprtData.isStarting();
    sprtData.clear();
    switchToSprtView();
    return GuiToolResult{
        .success = true,
        .content = wasRunning
            ? "SPRT test stopped and all results cleared."
            : "All SPRT results have been cleared."
    };
}

GuiToolResult handleShowSprtResult(const QaplaTester::Json::JsonValue&) {
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

// Exported (see gui-tool-sprt.h) so the unified start/stop tool (gui-tool-status-register.cpp)
// can dispatch into it directly by type -- start_sprt/stop_sprt no longer exist as separate
// model-visible tools, but the underlying logic is unchanged.
GuiToolResult handleStartSprtTournament(const QaplaTester::Json::JsonValue&) {
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
    switchToSprtView();
    return GuiToolResult{.success = true, .content = "SPRT test started."};
}

GuiToolResult handleStopSprtTournament(const QaplaTester::Json::JsonValue& arguments) {
    auto& sprtData = SprtTournamentData::instance();
    if (!sprtData.isRunning() && !sprtData.isStarting()) {
        return GuiToolResult{.success = false, .content = "No SPRT test is currently running."};
    }

    bool graceful = true;
    if (arguments.contains("mode") && arguments.at("mode").is_string()) {
        graceful = arguments.at("mode").as_string() != "abrupt";
    }

    sprtData.stopPool(graceful);
    switchToSprtView();
    return GuiToolResult{
        .success = true,
        .content = graceful
            ? "Stopping the SPRT test gracefully: the game already in progress will be "
              "finished, no new games will start."
            : "Stopping the SPRT test abruptly: the in-progress game is being aborted "
              "immediately."
    };
}

void registerSprtTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "select_sprt_engines",
        .description = "Selects two engines for SPRT test: \"champion\" (comparison baseline), "
                        "\"challenger\" (engine under test), replacing prior selection. Names "
                        "matched case-insensitive vs installed engine catalog; informal/"
                        "shortened name (e.g. \"spike\") auto-matched to the one installed "
                        "engine it can only mean (e.g. \"Spike 1.4.1\") -- pass name user "
                        "actually said, no need to call list_installed_engines first just to "
                        "find exact full name. If name could mean >1 installed engine, result "
                        "lists candidates -- ask user which they meant, never guess. Entirely "
                        "separate from select_engines/configure_tournament (classic tournament "
                        "mode) -- selecting SPRT engines never changes tournament's engine "
                        "selection, and vice versa.",
        .parametersSchema = buildSelectSprtEnginesSchema(),
        .handler = handleSelectSprtEngines
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_sprt",
        .description = "Sets SPRT test options: time_control, elo0/elo1 (H0/H1 Elo bounds), "
                        "alpha, beta, max_games, model, pentanomial, openings_file, pgn_file, "
                        "concurrency, draw_mode/draw_min_full_moves/"
                        "draw_required_consecutive_moves/draw_centipawn_threshold, resign_mode/"
                        "resign_required_consecutive_moves/resign_centipawn_threshold/"
                        "resign_two_sided. Every field independent/optional -- pass ONLY what "
                        "user asked to change, don't require/ask for others first. Anything not "
                        "passed keeps prior value (this session or earlier). Response always "
                        "reports the full current SPRT config, so no separate get_status "
                        "(type=\"sprt\") call is normally needed to confirm what changed. "
                        "IMPORTANT: fully separate config from configure_tournament -- classic "
                        "tournament and SPRT each have own time control, openings file, "
                        "concurrency, adjudication etc, despite same field names. If user's "
                        "message doesn't make clear tournament vs SPRT (e.g. bare \"set time "
                        "control to 1 min/game\"), infer from conversation so far (multi-engine "
                        "tournament or champion-vs-challenger SPRT?); if still unclear from "
                        "context, ask which they mean, never guess -- wrong guess silently "
                        "configures other feature. openings_file must be set (here or earlier "
                        "session) before start (type=\"sprt\") succeeds. If missing/invalid or "
                        "user wants to browse, set openings_file_dialog/pgn_file_dialog to true "
                        "instead of a typed path. "
                        "draw_mode/resign_mode \"off\"/\"test\"/\"active\"; both disabled "
                        "(\"off\") by default.",
        .parametersSchema = buildConfigureSprtSchema(),
        .handler = handleConfigureSprt,
        // openings_file_dialog/pgn_file_dialog wait on the user picking a file in a native
        // dialog, which can legitimately take much longer than a normal tool call -- see
        // open_add_engine_dialog's identical timeout.
        .timeout = std::chrono::minutes(10)
    });

}

} // namespace QaplaLlm
