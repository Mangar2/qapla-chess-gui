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
        auto boolProp = [](const std::string& description) {
            auto prop = Json::JsonValue::object();
            prop["type"] = "boolean";
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
        properties["openings_file"] = stringProp("Path to EPD/PGN opening book file.");
        properties["openings_file_dialog"] = boolProp(
            "Set true to open a native file picker instead of passing openings_file.");
        properties["pgn_file"] = stringProp("Path to save played games as PGN.");
        properties["pgn_file_dialog"] = boolProp(
            "Set true to open a native save-file picker instead of passing pgn_file.");
        properties["concurrency"] = integerProp("Games run in parallel.");
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
                " -- set openings_file_dialog=true instead to let the user pick a valid one");
            return;
        }
        data.tournamentOpening().openings().file = path;
        applied.push_back("openings file: " + path);
    }

    void openOpeningsFileDialog(TournamentData& data, std::vector<std::string>& applied) {
        auto paths = QaplaWindows::OsDialogs::openFileDialog(false);
        if (paths.empty()) {
            applied.push_back("openings file (dialog cancelled, unchanged)");
            return;
        }
        data.tournamentOpening().openings().file = paths.front();
        applied.push_back("openings file selected: " + paths.front());
    }

    void applyPgnFile(TournamentData& data, const std::string& path, std::vector<std::string>& applied) {
        data.pgnConfig().file = path;
        applied.push_back("PGN output file: " + path);
    }

    void openPgnFileDialog(TournamentData& data, std::vector<std::string>& applied) {
        auto chosenPath = QaplaWindows::OsDialogs::saveFileDialog({{"PGN files (*.pgn)", "pgn"}}, data.pgnConfig().file);
        if (chosenPath.empty()) {
            applied.push_back("PGN output file (dialog cancelled, unchanged)");
            return;
        }
        data.pgnConfig().file = chosenPath;
        applied.push_back("PGN output file selected: " + chosenPath);
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

    void applyDrawMode(TournamentData& data, const std::string& mode, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        auto& config = data.drawConfig();
        if (applyAdjudicationMode(mode, config.active, config.testOnly, problems)) {
            applied.push_back("draw adjudication mode=" + mode);
        }
    }

    void applyDrawMinFullMoves(TournamentData& data, double value, std::vector<std::string>& applied) {
        auto& config = data.drawConfig();
        config.minFullMoves = static_cast<uint32_t>(value);
        applied.push_back("draw min full moves=" + std::to_string(config.minFullMoves));
    }

    void applyDrawRequiredConsecutiveMoves(TournamentData& data, double value, std::vector<std::string>& applied) {
        auto& config = data.drawConfig();
        config.requiredConsecutiveMoves = static_cast<uint32_t>(value);
        applied.push_back("draw required consecutive moves=" + std::to_string(config.requiredConsecutiveMoves));
    }

    void applyDrawCentipawnThreshold(TournamentData& data, double value, std::vector<std::string>& applied) {
        auto& config = data.drawConfig();
        config.centipawnThreshold = static_cast<int>(value);
        applied.push_back("draw centipawn threshold=" + std::to_string(config.centipawnThreshold));
    }

    void applyResignMode(TournamentData& data, const std::string& mode, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        auto& config = data.resignConfig();
        if (applyAdjudicationMode(mode, config.active, config.testOnly, problems)) {
            applied.push_back("resign adjudication mode=" + mode);
        }
    }

    void applyResignRequiredConsecutiveMoves(TournamentData& data, double value, std::vector<std::string>& applied) {
        auto& config = data.resignConfig();
        config.requiredConsecutiveMoves = static_cast<uint32_t>(value);
        applied.push_back("resign required consecutive moves=" + std::to_string(config.requiredConsecutiveMoves));
    }

    void applyResignCentipawnThreshold(TournamentData& data, double value, std::vector<std::string>& applied) {
        auto& config = data.resignConfig();
        config.centipawnThreshold = static_cast<int>(value);
        applied.push_back("resign centipawn threshold=" + std::to_string(config.centipawnThreshold));
    }

    void applyResignTwoSided(TournamentData& data, bool value, std::vector<std::string>& applied) {
        auto& config = data.resignConfig();
        config.twoSided = value;
        applied.push_back(std::string("resign two-sided=") + (value ? "yes" : "no"));
    }

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

    // isRunning() is true for every non-Stopped state (Starting/Running/GracefulStopping/
    // Stopping alike -- see TournamentData::isRunning()'s doc comment), so it can't tell those
    // apart on its own; getState() can. Distinguishing GracefulStopping matters here since a
    // tournament in that state is still actively playing its in-progress games, just declining
    // to start new ones -- reporting it as plain "running" would hide that a stop was already
    // requested.
    std::string tournamentRunStateText(TournamentData& tournamentData) {
        switch (tournamentData.getState()) {
            case TournamentData::State::Starting:
                return "A tournament is currently starting.";
            case TournamentData::State::Running:
                return "A tournament is currently running.";
            case TournamentData::State::GracefulStopping:
                return "A tournament is currently running but stopping gracefully -- "
                       "in-progress games will finish, no new ones will start.";
            case TournamentData::State::Stopping:
                return "A tournament is currently stopping abruptly -- in-progress games are "
                       "being aborted.";
            case TournamentData::State::Stopped:
            default:
                return "No tournament is currently running.";
        }
    }

    // Shared by handleGetTournamentStatus (dispatched into by the unified get_status tool) and
    // configure_tournament's result (the latter always ends with the full status too -- see
    // buildConfigureResult -- so the model never needs a separate get_status round-trip just to
    // confirm what it changed).
    std::string buildTournamentStatusText(TournamentData& tournamentData) {
        auto selectedEngines = tournamentData.getEngineSelect().getSelectedEngines();
        std::vector<std::string> engineNames;
        for (const auto& engine : selectedEngines) {
            engineNames.push_back(engine.getName());
        }
        const auto& config = tournamentData.config();
        const auto& openingsFile = tournamentData.tournamentOpening().openings().file;
        const auto& pgnFile = tournamentData.pgnConfig().file;

        std::string runState = tournamentRunStateText(tournamentData);

        return std::format(
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
    }

    GuiToolResult buildConfigureResult(TournamentData& tournamentData, const std::vector<std::string>& problems,
        bool dialogOpened = false) {
        std::string message;
        if (!problems.empty()) {
            message = "Problems: " + joinStrings(problems) + ". ";
        }
        message += buildTournamentStatusText(tournamentData);
        return GuiToolResult{.success = problems.empty(), .content = message, .renderWidget = nullptr,
            .terminal = dialogOpened};
    }

    GuiToolResult handleConfigureTournament(const Json::JsonValue& arguments) {
        auto& tournamentData = TournamentData::instance();
        std::vector<std::string> applied;
        std::vector<std::string> problems;
        bool dialogOpened = false;

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
        if (arguments.contains("openings_file_dialog") && arguments.at("openings_file_dialog").is_boolean() &&
            arguments.at("openings_file_dialog").as_boolean()) {
            openOpeningsFileDialog(tournamentData, applied);
            dialogOpened = true;
        } else if (arguments.contains("openings_file") && arguments.at("openings_file").is_string()) {
            applyOpeningsFile(tournamentData, arguments.at("openings_file").as_string(), applied, problems);
        }
        if (arguments.contains("pgn_file_dialog") && arguments.at("pgn_file_dialog").is_boolean() &&
            arguments.at("pgn_file_dialog").as_boolean()) {
            openPgnFileDialog(tournamentData, applied);
            dialogOpened = true;
        } else if (arguments.contains("pgn_file") && arguments.at("pgn_file").is_string()) {
            applyPgnFile(tournamentData, arguments.at("pgn_file").as_string(), applied);
        }
        if (arguments.contains("concurrency") && arguments.at("concurrency").is_number()) {
            applyConcurrency(tournamentData, arguments.at("concurrency").as_number(), applied, problems);
        }
        if (arguments.contains("draw_mode") && arguments.at("draw_mode").is_string()) {
            applyDrawMode(tournamentData, arguments.at("draw_mode").as_string(), applied, problems);
        }
        if (arguments.contains("draw_min_full_moves") && arguments.at("draw_min_full_moves").is_number()) {
            applyDrawMinFullMoves(tournamentData, arguments.at("draw_min_full_moves").as_number(), applied);
        }
        if (arguments.contains("draw_required_consecutive_moves") &&
            arguments.at("draw_required_consecutive_moves").is_number()) {
            applyDrawRequiredConsecutiveMoves(tournamentData, arguments.at("draw_required_consecutive_moves").as_number(), applied);
        }
        if (arguments.contains("draw_centipawn_threshold") && arguments.at("draw_centipawn_threshold").is_number()) {
            applyDrawCentipawnThreshold(tournamentData, arguments.at("draw_centipawn_threshold").as_number(), applied);
        }
        if (arguments.contains("resign_mode") && arguments.at("resign_mode").is_string()) {
            applyResignMode(tournamentData, arguments.at("resign_mode").as_string(), applied, problems);
        }
        if (arguments.contains("resign_required_consecutive_moves") &&
            arguments.at("resign_required_consecutive_moves").is_number()) {
            applyResignRequiredConsecutiveMoves(tournamentData, arguments.at("resign_required_consecutive_moves").as_number(), applied);
        }
        if (arguments.contains("resign_centipawn_threshold") && arguments.at("resign_centipawn_threshold").is_number()) {
            applyResignCentipawnThreshold(tournamentData, arguments.at("resign_centipawn_threshold").as_number(), applied);
        }
        if (arguments.contains("resign_two_sided") && arguments.at("resign_two_sided").is_boolean()) {
            applyResignTwoSided(tournamentData, arguments.at("resign_two_sided").as_boolean(), applied);
        }

        // applyGames/applyRounds/applyEvent, applyOpeningsFile and
        // applyPgnFile all mutate raw references (config()/openings()/
        // pgnOptions()) that don't persist on their own -- see
        // ImGuiTournamentConfiguration::updateConfiguration()'s doc comment.
        // Calling all unconditionally is cheap and always correct, whether or
        // not this particular call touched their fields.
        tournamentData.tournamentConfiguration().updateConfiguration();
        tournamentData.tournamentOpening().updateConfiguration();
        tournamentData.tournamentPgn().updateConfiguration();
        tournamentData.tournamentAdjudication().updateConfiguration();

        if (!applied.empty()) {
            switchToTournamentView();
        }
        return buildConfigureResult(tournamentData, problems, dialogOpened);
    }

} // namespace

// Exported (see gui-tool-tournament.h) so the unified get_status/clear_result/show_result tools
// (gui-tool-status-register.cpp) can dispatch into them directly by type --
// get_tournament_status/clear_tournament_result/show_tournament_result no longer exist as
// separate model-visible tools, but the underlying logic is unchanged.
GuiToolResult handleGetTournamentStatus(const QaplaTester::Json::JsonValue&) {
    return GuiToolResult{.success = true, .content = buildTournamentStatusText(TournamentData::instance())};
}

GuiToolResult handleClearTournamentResult(const QaplaTester::Json::JsonValue&) {
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

GuiToolResult handleShowTournamentResult(const QaplaTester::Json::JsonValue&) {
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

// Exported (see gui-tool-tournament.h) so the unified start/stop tool (gui-tool-status-register.cpp)
// can dispatch into it directly by type -- start_tournament/stop_tournament no longer exist as
// separate model-visible tools, but the underlying logic is unchanged.
GuiToolResult handleStartTournament(const QaplaTester::Json::JsonValue&) {
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

GuiToolResult handleStopTournament(const QaplaTester::Json::JsonValue& arguments) {
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
                        "event (name), openings_file, pgn_file, concurrency, draw_mode/"
                        "draw_min_full_moves/draw_required_consecutive_moves/"
                        "draw_centipawn_threshold, resign_mode/resign_required_consecutive_moves/"
                        "resign_centipawn_threshold/resign_two_sided. Each field independent/"
                        "optional -- pass ONLY what user asked to change (e.g. just \"games\"); "
                        "don't require/ask other fields first. Unpassed fields keep prior value "
                        "(this session or earlier), don't assume unset. Response always reports "
                        "the full current tournament config, so no separate get_status "
                        "(type=\"tournament\") call is normally needed to confirm what changed. "
                        "openings_file must be set (here or earlier session) before start "
                        "(type=\"tournament\") succeeds, no safe default. For openings_file/"
                        "pgn_file, set openings_file_dialog/pgn_file_dialog to true instead of a "
                        "typed path to open a native file picker -- never type/guess a path "
                        "yourself. draw_mode/resign_mode "
                        "\"off\"/\"test\"/\"active\"; both disabled (\"off\") by default.",
        .parametersSchema = buildConfigureTournamentSchema(),
        .handler = handleConfigureTournament,
        // openings_file_dialog/pgn_file_dialog wait on the user picking a file in a native
        // dialog, which can legitimately take much longer than a normal tool call -- see
        // open_add_engine_dialog's identical timeout.
        .timeout = std::chrono::minutes(10)
    });

}

} // namespace QaplaLlm
