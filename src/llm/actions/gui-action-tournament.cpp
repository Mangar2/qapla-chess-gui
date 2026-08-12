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

// The GUI half of the tournament actions. Only the qapla executable links this: tournament-data.h
// transitively pulls in ImGui/GLFW headers the unit-tests target has no include paths for. The
// pure, unit-testable part (resolveEngines) lives in gui-tool-tournament.cpp.

#include "gui-action-tournament.h"
#include "../gui-tool-tournament.h"
#include "../../tournament-data.h"
#include "../../snackbar.h"
#include "../../os-dialogs.h"
#include "../../callback-manager.h"

#include <tournament/tournament.h>

#include <imgui.h>

#include <filesystem>
#include <format>
#include <optional>

namespace QaplaLlm::Actions {

namespace {
    using QaplaWindows::TournamentData;

    // Tab-only variant of the classic (non-AI) tournament chatbot flow's "Switch to Tournament
    // View" message (see chatbot-step-tournament-start.cpp). ImGuiTabBar flips to the Tournament
    // tab on the next frame just like for "switch_to_tournament_view", but TournamentData does
    // NOT listen for this message, so it does not also activate a running game's board window.
    // Called whenever an action actually changes tournament settings or its run state, so the
    // user sees what the AI just did without a board popping up and covering the AI chat.
    void switchToTournamentView() {
        QaplaWindows::StaticCallbacks::message().invokeAll("switch_to_tournament_tab");
    }

    // Looks for the most recent "tournament"-topic snackbar since countBefore entries existed,
    // i.e. the specific reason TournamentData::startTournament() showed via SnackbarManager for
    // why it silently declined to start (it reports failures only that way -- no exception, no
    // return value -- see mayStartTournament()/createTournament() in tournament-data.cpp).
    std::string findRecentTournamentSnackbar(std::size_t countBefore) {
        const auto& history = QaplaWindows::SnackbarManager::instance().getHistory();
        for (auto i = history.size(); i > countBefore; --i) {
            if (history[i - 1].topic == "tournament") {
                return history[i - 1].message;
            }
        }
        return "";
    }

    // isRunning() is true for every non-Stopped state (Starting/Running/GracefulStopping/
    // Stopping alike -- see TournamentData::isRunning()'s doc comment), so it can't tell those
    // apart on its own; getState() can. Distinguishing GracefulStopping matters here since a
    // tournament in that state is still actively playing its in-progress games, just declining
    // to start new ones -- reporting it as plain "running" would hide that a stop was already
    // requested.
    std::string runStateText(TournamentData& tournamentData) {
        switch (tournamentData.getState()) {
            case TournamentData::State::Starting:
                return "A tournament is currently starting.";
            case TournamentData::State::Running:
                return "A tournament is currently running.";
            case TournamentData::State::GracefulStopping:
                // Never phrased as "running": in this state the tournament accepts no new games
                // and cannot be reconfigured or restarted, so calling it running misleads.
                return "A tournament is finishing its games after a graceful stop. No new games "
                       "start.";
            case TournamentData::State::Stopping:
                return "A tournament is being aborted.";
            case TournamentData::State::Stopped:
            default:
                return "No tournament is currently running.";
        }
    }

    std::string adjudicationSummary(TournamentData& data) {
        const auto& draw = data.drawConfig();
        const auto& resign = data.resignConfig();
        return std::format(
            "Draw adjudication: {} (min full moves={}, required consecutive moves={}, "
            "centipawn threshold={}). Resign adjudication: {} (required consecutive moves={}, "
            "centipawn threshold={}, two-sided={}).",
            adjudicationModeName(adjudicationModeOf(draw.active, draw.testOnly)),
            draw.minFullMoves, draw.requiredConsecutiveMoves, draw.centipawnThreshold,
            adjudicationModeName(adjudicationModeOf(resign.active, resign.testOnly)),
            resign.requiredConsecutiveMoves, resign.centipawnThreshold,
            resign.twoSided ? "yes" : "no");
    }

    // Shared by tournamentStatus() and configureTournament()'s own result -- the latter always
    // ends with the full status too, so a caller never needs a second round-trip just to confirm
    // what a change did.
    std::string statusText(TournamentData& tournamentData) {
        std::vector<std::string> engineNames;
        for (const auto& engine : tournamentData.getEngineSelect().getSelectedEngines()) {
            engineNames.push_back(engine.getName());
        }
        const auto& config = tournamentData.config();
        const auto& openingsFile = tournamentData.tournamentOpening().openings().file;
        const auto& pgnFile = tournamentData.pgnConfig().file;

        return std::format(
            "Engines: {}. Time control: {}. Games per pairing: {}. Rounds: {}. "
            "Event name: {}. Openings file: {}. PGN output file: {}. Concurrency: {}. {} {}",
            engineNames.empty() ? "none selected" : joinList(engineNames),
            tournamentData.getGlobalSettings().getTimeControlSettings().timeControl,
            config.games, config.rounds,
            config.event.empty() ? "(not set)" : config.event,
            openingsFile.empty() ? "(not set)" : openingsFile,
            pgnFile.empty() ? "(not set)" : pgnFile,
            tournamentData.getExternalConcurrency(),
            runStateText(tournamentData),
            adjudicationSummary(tournamentData));
    }

    // Everything except concurrency is baked into the tournament at start: createTournament()
    // snapshots time control, openings, PGN and adjudication, and scheduleAll() lays out every
    // game up front. A mid-run change reaches neither the games in progress nor later rounds --
    // and if it did, a tournament played half under one setting and half under another would give
    // a meaningless Elo result. So such a patch is refused whole rather than stored, which would
    // leave the status reporting a value that is not the one being played.
    bool changesMoreThanConcurrency(const TournamentSettings& settings) {
        return settings.timeControl || settings.gamesPerPairing || settings.rounds
            || settings.event || settings.openingsFile || settings.pickOpeningsFile
            || settings.pgnFile || settings.pickPgnFile
            || settings.drawMode || settings.drawMinFullMoves
            || settings.drawRequiredConsecutiveMoves || settings.drawCentipawnThreshold
            || settings.resignMode || settings.resignRequiredConsecutiveMoves
            || settings.resignCentipawnThreshold || settings.resignTwoSided;
    }

    // A running tournament owns its configuration. Split into two phases because the advice
    // differs: one can still be stopped, the other is already stopping and only needs waiting out.
    enum class RunLock { None, Running, Stopping };

    RunLock runLockOf(TournamentData& data) {
        switch (data.getState()) {
            case TournamentData::State::Running:
            case TournamentData::State::Starting:
                return RunLock::Running;
            case TournamentData::State::GracefulStopping:
            case TournamentData::State::Stopping:
                return RunLock::Stopping;
            case TournamentData::State::Stopped:
            default:
                return RunLock::None;
        }
    }

    // Why the patch is refused, or nullopt if it may go through. The rationale (all games are
    // scheduled at start, so a mid-run change reaches none of them and would mix conditions into
    // one result) belongs here, not in the message: the model needs the rule and the next step,
    // and every extra sentence costs context it needs for the conversation.
    std::optional<std::string> settingsLockedReason(TournamentData& data,
                                                    const TournamentSettings& settings) {
        if (!changesMoreThanConcurrency(settings)) {
            return std::nullopt;
        }
        switch (runLockOf(data)) {
            case RunLock::Running:
                return "Settings are locked while a tournament runs. Nothing changed. Ask the "
                       "user: stop gracefully or abruptly? Then set the values and start. Only "
                       "concurrency can be changed while running.";
            case RunLock::Stopping:
                return "Settings are locked until the tournament has stopped. Nothing changed. "
                       "Wait, then set the values and start.";
            case RunLock::None:
            default:
                return std::nullopt;
        }
    }

    // Same rule for the engines: they are what the result table compares, so they are fixed for
    // as long as the tournament that uses them. Adding engines to the catalog is unrelated.
    std::optional<std::string> engineSelectionLockedReason(TournamentData& data) {
        switch (runLockOf(data)) {
            case RunLock::Running:
                return "Engine selection is locked while a tournament runs. Nothing changed. Ask "
                       "the user: stop gracefully or abruptly? Then select and start. Installing "
                       "engines still works.";
            case RunLock::Stopping:
                return "Engine selection is locked until the tournament has stopped. Nothing "
                       "changed. Wait, then select and start.";
            case RunLock::None:
            default:
                return std::nullopt;
        }
    }

    // ------------------------------------------------------------------
    // configureTournament() field appliers
    //
    // Each reports what it changed into `applied` (which only decides whether the tournament view
    // is brought to the front) or why it couldn't into `problems`. A rejected field never stops
    // the others in the same patch from being applied.
    // ------------------------------------------------------------------

    struct ConfigureOutcome {
        std::vector<std::string> applied;
        std::vector<std::string> problems;
        bool dialogShown = false;
        Remedy remedy = Remedy::None;
        // Set when the patch changed concurrency; lets configureTournament() answer a
        // concurrency-only patch with just this sentence instead of the full status.
        std::optional<std::string> concurrencyNote;
    };

    // Reports the effective per-pairing total (games * rounds) alongside whichever field just
    // changed -- games and rounds multiply into the actual game count, which is easy to misjudge,
    // so every change restates it using whatever the *other* field currently is.
    std::string gamesAndRoundsSummary(TournamentData& data) {
        return std::format("games={} per round, rounds={} ({} games per pairing in total)",
            data.config().games, data.config().rounds, data.config().games * data.config().rounds);
    }

    // Concurrency is the one setting that takes effect on a tournament that is actually running,
    // so it gets its own sentence saying whether it just did. Reporting it the usual way -- one
    // "concurrency=5" item inside the full status -- reads as if the change was ignored, and has
    // provoked stop/start cycles to force it through that were never needed.
    //
    // Switches on the exact state rather than isRunning(): that predicate is true for Starting,
    // GracefulStopping and Stopping as well, and claiming "the running tournament now plays 5 in
    // parallel" is wrong in every one of those -- a gracefully stopping tournament starts no new
    // games at all, so nothing there changes. applyConcurrency() below applies the value live
    // under exactly the same condition, so text and behaviour cannot drift apart.
    std::string concurrencySummary(TournamentData& data, uint32_t count) {
        switch (data.getState()) {
            case TournamentData::State::Running:
                return std::format("Concurrency is now {}. The running tournament uses it "
                                   "already; do not restart it.", count);
            case TournamentData::State::Starting:
                return std::format("Concurrency is now {}. The starting tournament will use it.",
                    count);
            default:
                return std::format("Concurrency is now {}. Applies to the next tournament.",
                    count);
        }
    }

    void applyOpeningsFile(TournamentData& data, const std::string& path, ConfigureOutcome& outcome) {
        if (!std::filesystem::exists(path)) {
            outcome.problems.push_back("openings file not found: " + path);
            outcome.remedy = Remedy::AskUserForOpeningsFile;
            return;
        }
        data.tournamentOpening().openings().file = path;
        outcome.applied.push_back("openings file: " + path);
    }

    void pickOpeningsFile(TournamentData& data, ConfigureOutcome& outcome) {
        outcome.dialogShown = true;
        auto paths = QaplaWindows::OsDialogs::openFileDialog(false);
        if (paths.empty()) {
            outcome.applied.push_back("openings file (dialog cancelled, unchanged)");
            return;
        }
        data.tournamentOpening().openings().file = paths.front();
        outcome.applied.push_back("openings file selected: " + paths.front());
    }

    void pickPgnFile(TournamentData& data, ConfigureOutcome& outcome) {
        outcome.dialogShown = true;
        auto chosenPath = QaplaWindows::OsDialogs::saveFileDialog(
            {{"PGN files (*.pgn)", "pgn"}}, data.pgnConfig().file);
        if (chosenPath.empty()) {
            outcome.applied.push_back("PGN output file (dialog cancelled, unchanged)");
            return;
        }
        data.pgnConfig().file = chosenPath;
        outcome.applied.push_back("PGN output file selected: " + chosenPath);
    }

    void applySettings(TournamentData& data, const TournamentSettings& settings,
        ConfigureOutcome& outcome) {
        if (settings.timeControl) {
            // Not validated: QaplaTester::TimeControl::parse() is deliberately lenient (never
            // throws, silently ignores whatever it can't make sense of) -- the classic UI's time
            // control text field relies on the same behaviour, so a stricter check here would be
            // inconsistent, not safer.
            data.getGlobalSettings().setTimeControlSettings({.timeControl = *settings.timeControl});
            outcome.applied.push_back("time control " + *settings.timeControl);
        }
        if (settings.gamesPerPairing) {
            if (*settings.gamesPerPairing < 1) {
                outcome.problems.push_back("games must be at least 1");
            } else {
                data.config().games = *settings.gamesPerPairing;
                outcome.applied.push_back(gamesAndRoundsSummary(data));
            }
        }
        if (settings.rounds) {
            if (*settings.rounds < 1) {
                outcome.problems.push_back("rounds must be at least 1");
            } else {
                data.config().rounds = *settings.rounds;
                outcome.applied.push_back(gamesAndRoundsSummary(data));
            }
        }
        if (settings.event) {
            data.config().event = *settings.event;
            outcome.applied.push_back("event name");
        }
        if (settings.pickOpeningsFile) {
            pickOpeningsFile(data, outcome);
        } else if (settings.openingsFile) {
            applyOpeningsFile(data, *settings.openingsFile, outcome);
        }
        if (settings.pickPgnFile) {
            pickPgnFile(data, outcome);
        } else if (settings.pgnFile) {
            data.pgnConfig().file = *settings.pgnFile;
            outcome.applied.push_back("PGN output file: " + *settings.pgnFile);
        }
        if (settings.concurrency) {
            if (*settings.concurrency < 1) {
                outcome.problems.push_back("concurrency must be at least 1");
            } else {
                data.setExternalConcurrency(*settings.concurrency);
                // Push it into the live pool only while the tournament is really running -- this
                // is what makes the change take effect mid-run instead of only at the next start.
                // Not during GracefulStopping: raising concurrency there would hand the pool room
                // for more games while the tournament is supposed to start none, contradicting
                // both the graceful stop and what concurrencySummary() reports for that state.
                if (data.getState() == TournamentData::State::Running) {
                    data.setPoolConcurrency(*settings.concurrency, true, true);
                }
                outcome.concurrencyNote = concurrencySummary(data, *settings.concurrency);
                outcome.applied.push_back(*outcome.concurrencyNote);
            }
        }

        auto& draw = data.drawConfig();
        if (settings.drawMode) {
            applyAdjudicationMode(*settings.drawMode, draw.active, draw.testOnly);
            outcome.applied.push_back(
                "draw adjudication mode=" + adjudicationModeName(*settings.drawMode));
        }
        if (settings.drawMinFullMoves) {
            draw.minFullMoves = *settings.drawMinFullMoves;
            outcome.applied.push_back("draw min full moves=" + std::to_string(draw.minFullMoves));
        }
        if (settings.drawRequiredConsecutiveMoves) {
            draw.requiredConsecutiveMoves = *settings.drawRequiredConsecutiveMoves;
            outcome.applied.push_back(
                "draw required consecutive moves=" + std::to_string(draw.requiredConsecutiveMoves));
        }
        if (settings.drawCentipawnThreshold) {
            draw.centipawnThreshold = *settings.drawCentipawnThreshold;
            outcome.applied.push_back(
                "draw centipawn threshold=" + std::to_string(draw.centipawnThreshold));
        }

        auto& resign = data.resignConfig();
        if (settings.resignMode) {
            applyAdjudicationMode(*settings.resignMode, resign.active, resign.testOnly);
            outcome.applied.push_back(
                "resign adjudication mode=" + adjudicationModeName(*settings.resignMode));
        }
        if (settings.resignRequiredConsecutiveMoves) {
            resign.requiredConsecutiveMoves = *settings.resignRequiredConsecutiveMoves;
            outcome.applied.push_back("resign required consecutive moves=" +
                std::to_string(resign.requiredConsecutiveMoves));
        }
        if (settings.resignCentipawnThreshold) {
            resign.centipawnThreshold = *settings.resignCentipawnThreshold;
            outcome.applied.push_back(
                "resign centipawn threshold=" + std::to_string(resign.centipawnThreshold));
        }
        if (settings.resignTwoSided) {
            resign.twoSided = *settings.resignTwoSided;
            outcome.applied.push_back(
                std::string("resign two-sided=") + (resign.twoSided ? "yes" : "no"));
        }
    }
} // namespace

ActionResult selectTournamentEngines(const std::vector<std::string>& engineNames) {
    if (engineNames.empty()) {
        return failed("No engine names were given.");
    }
    if (auto locked = engineSelectionLockedReason(TournamentData::instance())) {
        return failed(*locked);
    }

    auto outcome = resolveEngines(engineNames);
    if (!outcome.ambiguous.empty()) {
        return failed(formatAmbiguousEngineNames(outcome.ambiguous) +
            " Ask the user which one they mean.");
    }
    if (outcome.resolved.empty()) {
        return failed("None of these engines are installed: " + joinList(outcome.notFound) +
            ". Look up which engines are available first.");
    }

    auto& tournamentData = TournamentData::instance();
    tournamentData.getEngineSelect().setEngineConfigurations(outcome.resolved);
    tournamentData.config().type = "round-robin"; // gauntlet mode is not exposed via chat
    // config() is a raw reference -- unlike setEngineConfigurations() above, mutating it directly
    // doesn't persist on its own (see ImGuiTournamentConfiguration::updateConfiguration()).
    tournamentData.tournamentConfiguration().updateConfiguration();

    std::vector<std::string> selectedNames;
    for (const auto& engine : outcome.resolved) {
        selectedNames.push_back(engine.getName());
    }

    std::string message = "Selected: " + joinList(selectedNames) + ".";
    if (!outcome.notFound.empty()) {
        message += " Not installed (skipped): " + joinList(outcome.notFound) + ".";
    }
    switchToTournamentView();
    return succeeded(message);
}

ActionResult configureTournament(const TournamentSettings& settings) {
    auto& tournamentData = TournamentData::instance();

    // Refused before anything is applied, so a mixed patch cannot leave half its fields in place:
    // partial success is the one outcome a caller has no way to recover from sensibly.
    if (auto locked = settingsLockedReason(tournamentData, settings)) {
        return failed(*locked);
    }

    ConfigureOutcome outcome;
    applySettings(tournamentData, settings, outcome);

    // Most appliers above mutate raw references (config()/openings()/pgnConfig()) that don't
    // persist on their own -- see ImGuiTournamentConfiguration::updateConfiguration()'s doc
    // comment. Calling all four unconditionally is cheap and always correct, whether or not this
    // particular patch touched their fields.
    tournamentData.tournamentConfiguration().updateConfiguration();
    tournamentData.tournamentOpening().updateConfiguration();
    tournamentData.tournamentPgn().updateConfiguration();
    tournamentData.tournamentAdjudication().updateConfiguration();

    if (!outcome.applied.empty()) {
        switchToTournamentView();
    }

    // A patch that changed nothing but concurrency answers with just that one sentence. The full
    // status would bury it between a dozen unrelated fields and end on the run state, which is
    // exactly the reading that makes a caller think the change didn't land.
    if (outcome.problems.empty() && outcome.concurrencyNote && outcome.applied.size() == 1) {
        return ActionResult{.ok = true, .text = *outcome.concurrencyNote, .widget = nullptr,
            .endsTurn = outcome.dialogShown, .remedy = outcome.remedy};
    }

    std::string message;
    if (!outcome.problems.empty()) {
        message = "Problems: " + joinList(outcome.problems) + ". ";
    }
    message += statusText(tournamentData);
    return ActionResult{.ok = outcome.problems.empty(), .text = message, .widget = nullptr,
        .endsTurn = outcome.dialogShown, .remedy = outcome.remedy};
}

ActionResult startTournament() {
    auto& tournamentData = TournamentData::instance();
    // "Stop it first" is only the right advice for a tournament that is actually running. A
    // tournament that is already on its way out needs the opposite -- waiting -- and telling the
    // caller to stop it again has sent them round a stop/start loop that could never succeed.
    switch (tournamentData.getState()) {
        case TournamentData::State::Running:
            return failed("A tournament is already running. Stop it first if you want to start a "
                          "different one.");
        case TournamentData::State::Starting:
            return failed("A tournament is already starting.");
        case TournamentData::State::GracefulStopping:
            return failed("The previous tournament is still finishing its games. Wait, or stop "
                          "abruptly to end them now.");
        case TournamentData::State::Stopping:
            return failed("The previous tournament is still stopping. Wait.");
        case TournamentData::State::Stopped:
        default:
            break;
    }

    auto historyCountBefore = QaplaWindows::SnackbarManager::instance().getHistory().size();

    tournamentData.startTournament(); // verbose=true, same as the classic "Start Tournament" button

    if (!tournamentData.isRunning() && !tournamentData.isStarting()) {
        auto reason = findRecentTournamentSnackbar(historyCountBefore);
        return failed(reason.empty() ? "Could not start the tournament." : reason);
    }

    tournamentData.setPoolConcurrency(tournamentData.getExternalConcurrency(), true, true);
    switchToTournamentView();
    return succeeded("Tournament started.");
}

ActionResult stopTournament(StopMode mode) {
    auto& tournamentData = TournamentData::instance();
    if (!tournamentData.isRunning() && !tournamentData.isStarting()) {
        return failed("No tournament is currently running.");
    }

    // Reporting "stopping the tournament gracefully" for a tournament that has been stopping
    // gracefully all along reads like the request just took effect, which invites waiting for a
    // change that will never come. Say plainly that nothing changed, and name the one thing that
    // still would.
    if (tournamentData.getState() == TournamentData::State::GracefulStopping
        && mode == StopMode::Graceful) {
        return succeeded("Already stopping gracefully. Nothing changed. Wait, or stop abruptly.");
    }
    if (tournamentData.getState() == TournamentData::State::Stopping) {
        return succeeded("Already stopping. Nothing changed.");
    }

    if (mode == StopMode::Abrupt) {
        // Returns a result, not an intention: the call blocks until the games are really gone, so
        // the caller can configure and start straight away. Reporting "is being aborted" and
        // returning early is what produced the retry loops -- the next call arrived milliseconds
        // later and found a tournament that was still stopping.
        tournamentData.stopPoolAbruptlyAndWait();
        switchToTournamentView();
        return succeeded("Tournament stopped.");
    }

    tournamentData.stopPool(true);
    switchToTournamentView();
    return succeeded("Stopping gracefully. Running games finish first, no new ones start. Not "
                     "done yet.");
}

ActionResult tournamentStatus() {
    return succeeded(statusText(TournamentData::instance()));
}

ActionResult clearTournamentResult() {
    auto& tournamentData = TournamentData::instance();
    if (!tournamentData.hasTasksScheduled()) {
        return succeeded("There are no tournament results to clear.");
    }

    bool wasRunning = tournamentData.isRunning() || tournamentData.isStarting();
    tournamentData.clear(false); // verbose=false: this action's own text already tells the user
    switchToTournamentView();
    return succeeded(wasRunning ? "Tournament stopped and all results cleared."
                                : "All tournament results have been cleared.");
}

ActionResult showTournamentResult() {
    auto& tournamentData = TournamentData::instance();
    if (tournamentData.getTournamentResult().scoredEngines().empty()) {
        return succeeded("No tournament results are available yet.");
    }

    // Renders the same live control the classic (non-AI) chatbot's results step draws (see
    // ChatbotStepStandardTournamentResult::draw()) -- a real ImGuiTable, not a text dump of the
    // data. Always reads TournamentData::instance() fresh at draw time (every frame the entry
    // stays visible), so it reflects the tournament's current state rather than a one-time
    // snapshot of the results as they were when this action ran.
    return ActionResult{
        .ok = true,
        .text = "Showing the current tournament results as a table in the chat -- it is "
                "already visible to the user, so do not restate, list, or summarize the "
                "numbers in your reply; just briefly confirm what you did.",
        .widget = []() {
            auto& data = TournamentData::instance();
            ImGui::Text("Tournament Progress: %u / %u games completed",
                data.getPlayedGames(), data.getTotalGames());
            ImGui::Spacing();
            data.drawEloTable(ImVec2(0.0F, 3000.0F));
        }};
}

std::string tournamentActivityText() {
    using State = TournamentData::State;
    switch (TournamentData::instance().getState()) {
        case State::Starting: return "a tournament is starting";
        case State::Running: return "a tournament is running";
        case State::GracefulStopping:
            // Same reasoning as runStateText()'s GracefulStopping case: never call this "running".
            return "a tournament is finishing its games after a graceful stop";
        case State::Stopping: return "a tournament is stopping abruptly";
        case State::Stopped:
        default: return "";
    }
}

} // namespace QaplaLlm::Actions
