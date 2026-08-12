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

// Only the qapla executable links this -- sprt-tournament-data.h pulls in the ImGui/GLFW stack.
// Engine name resolution reuses resolveEngines() from gui-tool-tournament.h, which is pure and
// unit-tested there.

#include "gui-action-sprt.h"
#include "../gui-tool-tournament.h"
#include "../../sprt-tournament-data.h"
#include "../../snackbar.h"
#include "../../os-dialogs.h"
#include "../../callback-manager.h"

#include <sprt/sprt-manager.h>

#include <imgui.h>

#include <filesystem>
#include <format>
#include <optional>
#include <vector>

namespace QaplaLlm::Actions {

namespace {
    using QaplaWindows::SprtTournamentData;

    // Tab-only variant of the classic (non-AI) SPRT chatbot flow's "Switch to SPRT View" message
    // (see chatbot-step-tournament-start.cpp, shared with the tournament flow). ImGuiTabBar flips
    // to the SPRT tab on the next frame just like for "switch_to_sprt_view", but
    // SprtTournamentData does NOT listen for this message, so it does not also activate the
    // running SPRT board window and cover the AI chat.
    void switchToSprtView() {
        QaplaWindows::StaticCallbacks::message().invokeAll("switch_to_sprt_tab");
    }

    // Looks for the most recent "sprt-tournament"-topic snackbar since countBefore entries
    // existed -- the specific reason SprtTournamentData::startTournament() showed via
    // SnackbarManager for why it silently declined to start (reports failures only that way,
    // same as TournamentData -- see mayStartTournament()/createTournament()).
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

    // gauntlet=true is the challenger, gauntlet=false is the champion -- the same convention
    // ChatbotStepSprtSelectGauntlet uses.
    SprtEngineNames engineNamesOf(SprtTournamentData& data) {
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
            adjudicationModeName(adjudicationModeOf(draw.active, draw.testOnly)),
            draw.minFullMoves, draw.requiredConsecutiveMoves, draw.centipawnThreshold,
            adjudicationModeName(adjudicationModeOf(resign.active, resign.testOnly)),
            resign.requiredConsecutiveMoves, resign.centipawnThreshold,
            resign.twoSided ? "yes" : "no");
    }

    // isRunning() is true for every non-Stopped state (Starting/Running/GracefulStopping/Stopping
    // alike), so it can't tell those apart on its own; state() can. Distinguishing the two
    // pending-stop states matters: a test in either is still busy with games that were already
    // under way, it just declines to start new ones -- and only GracefulStopping lets those games
    // play out, while Stopping aborts them.
    std::string runStateText(SprtTournamentData& sprtData) {
        switch (sprtData.state()) {
            case SprtTournamentData::State::Starting:
                return "An SPRT test is currently starting.";
            case SprtTournamentData::State::Running:
                return "An SPRT test is currently running.";
            case SprtTournamentData::State::GracefulStopping:
                // Deliberately not phrased as "running but stopping": a test in this state is on
                // its way out and accepts no new games, so calling it "running" invites the reader
                // to treat it like a live test (restart it, apply settings to it). It is only
                // finishing what was already in progress.
                return "An SPRT test is finishing its games after a graceful stop. No new games "
                       "start.";
            case SprtTournamentData::State::Stopping:
                return "An SPRT test is being aborted.";
            case SprtTournamentData::State::Stopped:
            default:
                return "No SPRT test is currently running.";
        }
    }

    std::string statusText(SprtTournamentData& sprtData) {
        auto names = engineNamesOf(sprtData);
        const auto& config = sprtData.sprtConfig();
        const auto& openingsFile = sprtData.tournamentOpening().openings().file;
        const auto& pgnFile = sprtData.tournamentPgn().pgnOptions().file;

        std::string runState = runStateText(sprtData);
        if (sprtData.isFinished()) {
            runState += " A decision has been reached (or the game limit was hit) -- show the "
                        "SPRT result to see it.";
        }

        return std::format(
            "Champion (comparison engine): {}. Challenger (engine under test): {}. "
            "Time control: {}. Elo bounds: H0={:.2f}, H1={:.2f}. Alpha={:.3f}, Beta={:.3f}. "
            "Max games: {}. Model: {}{}. Openings file: {}. PGN output file: {}. "
            "Concurrency: {}. {} {}",
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

    struct ConfigureOutcome {
        std::vector<std::string> applied;
        std::vector<std::string> problems;
        bool dialogShown = false;
        Remedy remedy = Remedy::None;
        // Set when the patch changed concurrency; lets configureSprt() answer a concurrency-only
        // patch with just this sentence instead of the full status.
        std::optional<std::string> concurrencyNote;
    };

    // Concurrency is the one setting that takes effect on a test that is actually running, so it
    // gets its own sentence saying whether it just did. Reporting it the usual way -- one
    // "concurrency=5" item inside the full status -- reads as if the change was ignored, and has
    // provoked stop/start cycles to force it through that were never needed.
    //
    // Switches on the exact state rather than isRunning(): that predicate is true for Starting,
    // GracefulStopping and Stopping as well, and claiming "the running test now plays 5 in
    // parallel" is wrong in every one of those. applySettings() applies the value live under
    // exactly the same condition, so text and behaviour cannot drift apart.
    std::string concurrencySummary(SprtTournamentData& data, uint32_t count) {
        switch (data.state()) {
            case SprtTournamentData::State::Running:
                return std::format("Concurrency is now {}. The running SPRT test uses it already; "
                                   "do not restart it.", count);
            case SprtTournamentData::State::Starting:
                return std::format("Concurrency is now {}. The starting SPRT test will use it.",
                    count);
            default:
                return std::format("Concurrency is now {}. Applies to the next SPRT test.", count);
        }
    }

    // Everything except concurrency is baked into the test when it starts, exactly as for the
    // classic tournament (see gui-action-tournament.cpp's settingsLockedReason()). Changing the
    // bounds, the time control or the adjudication half way through an SPRT test is worse still:
    // the decision it computes is only valid if every game was played under the same conditions.
    bool changesMoreThanConcurrency(const SprtSettings& settings) {
        return settings.timeControl || settings.eloH0 || settings.eloH1 || settings.alpha
            || settings.beta || settings.maxGames || settings.model || settings.pentanomial
            || settings.openingsFile || settings.pickOpeningsFile
            || settings.pgnFile || settings.pickPgnFile
            || settings.drawMode || settings.drawMinFullMoves
            || settings.drawRequiredConsecutiveMoves || settings.drawCentipawnThreshold
            || settings.resignMode || settings.resignRequiredConsecutiveMoves
            || settings.resignCentipawnThreshold || settings.resignTwoSided;
    }

    // A running test owns its configuration. Split into two phases because the advice differs:
    // one can still be stopped, the other is already stopping and only needs waiting out.
    enum class RunLock { None, Running, Stopping };

    RunLock runLockOf(SprtTournamentData& data) {
        switch (data.state()) {
            case SprtTournamentData::State::Running:
            case SprtTournamentData::State::Starting:
                return RunLock::Running;
            case SprtTournamentData::State::GracefulStopping:
            case SprtTournamentData::State::Stopping:
                return RunLock::Stopping;
            case SprtTournamentData::State::Stopped:
            default:
                return RunLock::None;
        }
    }

    // Why the patch is refused, or nullopt if it may go through. The rationale (an SPRT decision
    // is only valid if every game ran under the same conditions) belongs here, not in the
    // message: the model needs the rule and the next step, nothing else.
    std::optional<std::string> settingsLockedReason(SprtTournamentData& data,
                                                    const SprtSettings& settings) {
        if (!changesMoreThanConcurrency(settings)) {
            return std::nullopt;
        }
        switch (runLockOf(data)) {
            case RunLock::Running:
                return "Settings are locked while an SPRT test runs. Nothing changed. Ask the "
                       "user: stop gracefully or abruptly? Then set the values and start. Only "
                       "concurrency can be changed while running.";
            case RunLock::Stopping:
                return "Settings are locked until the test has stopped. Nothing changed. Wait, "
                       "then set the values and start.";
            case RunLock::None:
            default:
                return std::nullopt;
        }
    }

    // Same rule for the pair of engines: the whole test is a statement about them, so they are
    // fixed for as long as it runs. Adding engines to the catalog is unrelated.
    std::optional<std::string> engineSelectionLockedReason(SprtTournamentData& data) {
        switch (runLockOf(data)) {
            case RunLock::Running:
                return "Engine selection is locked while an SPRT test runs. Nothing changed. Ask "
                       "the user: stop gracefully or abruptly? Then select and start. Installing "
                       "engines still works.";
            case RunLock::Stopping:
                return "Engine selection is locked until the test has stopped. Nothing changed. "
                       "Wait, then select and start.";
            case RunLock::None:
            default:
                return std::nullopt;
        }
    }

    void pickOpeningsFile(SprtTournamentData& data, ConfigureOutcome& outcome) {
        outcome.dialogShown = true;
        auto paths = QaplaWindows::OsDialogs::openFileDialog(false);
        if (paths.empty()) {
            outcome.applied.push_back("openings file (dialog cancelled, unchanged)");
            return;
        }
        data.tournamentOpening().openings().file = paths.front();
        outcome.applied.push_back("openings file selected: " + paths.front());
    }

    void pickPgnFile(SprtTournamentData& data, ConfigureOutcome& outcome) {
        outcome.dialogShown = true;
        auto path = QaplaWindows::OsDialogs::saveFileDialog(
            {{"PGN files (*.pgn)", "pgn"}}, data.tournamentPgn().pgnOptions().file);
        if (path.empty()) {
            outcome.applied.push_back("PGN output file (dialog cancelled, unchanged)");
            return;
        }
        data.tournamentPgn().pgnOptions().file = path;
        outcome.applied.push_back("PGN output file selected: " + path);
    }

    void applySettings(SprtTournamentData& data, const SprtSettings& settings,
        ConfigureOutcome& outcome) {
        if (settings.timeControl) {
            // Not validated -- see the identical note on the tournament action's time control.
            data.getGlobalSettings().setTimeControlSettings({.timeControl = *settings.timeControl});
            outcome.applied.push_back("time control " + *settings.timeControl);
        }
        if (settings.eloH0) {
            data.sprtConfig().eloH0 = static_cast<float>(*settings.eloH0);
            outcome.applied.push_back(std::format("elo0={:.2f}", data.sprtConfig().eloH0));
        }
        if (settings.eloH1) {
            data.sprtConfig().eloH1 = static_cast<float>(*settings.eloH1);
            outcome.applied.push_back(std::format("elo1={:.2f}", data.sprtConfig().eloH1));
        }
        if (settings.alpha) {
            if (*settings.alpha <= 0.0 || *settings.alpha >= 1.0) {
                outcome.problems.push_back("alpha must be between 0 and 1 (exclusive)");
            } else {
                data.sprtConfig().alpha = *settings.alpha;
                outcome.applied.push_back(std::format("alpha={:.3f}", *settings.alpha));
            }
        }
        if (settings.beta) {
            if (*settings.beta <= 0.0 || *settings.beta >= 1.0) {
                outcome.problems.push_back("beta must be between 0 and 1 (exclusive)");
            } else {
                data.sprtConfig().beta = *settings.beta;
                outcome.applied.push_back(std::format("beta={:.3f}", *settings.beta));
            }
        }
        if (settings.maxGames) {
            if (*settings.maxGames < 1) {
                outcome.problems.push_back("max games must be at least 1");
            } else {
                data.sprtConfig().maxGames = *settings.maxGames;
                outcome.applied.push_back("max_games=" + std::to_string(*settings.maxGames));
            }
        }
        if (settings.model) {
            const auto& model = *settings.model;
            if (model != "normalized" && model != "logistic" && model != "bayesian") {
                outcome.problems.push_back(
                    "model must be \"normalized\", \"logistic\", or \"bayesian\" (got \"" + model +
                    "\")");
            } else {
                data.sprtConfig().model = model;
                outcome.applied.push_back("model=" + model);
            }
        }
        if (settings.pentanomial) {
            data.sprtConfig().pentanomial = *settings.pentanomial;
            outcome.applied.push_back(
                std::string("pentanomial=") + (*settings.pentanomial ? "yes" : "no"));
        }
        if (settings.pickOpeningsFile) {
            pickOpeningsFile(data, outcome);
        } else if (settings.openingsFile) {
            if (!std::filesystem::exists(*settings.openingsFile)) {
                outcome.problems.push_back("openings file not found: " + *settings.openingsFile);
                outcome.remedy = Remedy::AskUserForOpeningsFile;
            } else {
                data.tournamentOpening().openings().file = *settings.openingsFile;
                outcome.applied.push_back("openings file: " + *settings.openingsFile);
            }
        }
        if (settings.pickPgnFile) {
            pickPgnFile(data, outcome);
        } else if (settings.pgnFile) {
            data.tournamentPgn().pgnOptions().file = *settings.pgnFile;
            outcome.applied.push_back("PGN output file: " + *settings.pgnFile);
        }
        if (settings.concurrency) {
            if (*settings.concurrency < 1) {
                outcome.problems.push_back("concurrency must be at least 1");
            } else {
                data.setExternalConcurrency(*settings.concurrency);
                // Push it into the live pool only while the test is really running -- this is
                // what makes the change take effect mid-run instead of only at the next start.
                // Not during GracefulStopping: raising concurrency there would hand the pool room
                // for more games while the test is supposed to start none, contradicting both the
                // graceful stop and what concurrencySummary() reports for that state.
                if (data.state() == SprtTournamentData::State::Running) {
                    data.setPoolConcurrency(*settings.concurrency, true, true);
                }
                outcome.concurrencyNote = concurrencySummary(data, *settings.concurrency);
                outcome.applied.push_back(*outcome.concurrencyNote);
            }
        }

        auto& draw = data.tournamentAdjudication().drawConfig();
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

        auto& resign = data.tournamentAdjudication().resignConfig();
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

ActionResult selectSprtEngines(const std::string& championName, const std::string& challengerName) {
    if (auto locked = engineSelectionLockedReason(SprtTournamentData::instance())) {
        return failed(*locked);
    }
    auto championOutcome = resolveEngines({championName});
    auto challengerOutcome = resolveEngines({challengerName});

    std::vector<AmbiguousEngineName> ambiguous = championOutcome.ambiguous;
    ambiguous.insert(ambiguous.end(), challengerOutcome.ambiguous.begin(),
        challengerOutcome.ambiguous.end());
    if (!ambiguous.empty()) {
        return failed(formatAmbiguousEngineNames(ambiguous) + " Ask the user which one they mean.");
    }

    std::vector<std::string> notFound = championOutcome.notFound;
    notFound.insert(notFound.end(), challengerOutcome.notFound.begin(),
        challengerOutcome.notFound.end());
    if (!notFound.empty()) {
        return failed("Not installed: " + joinList(notFound) +
            ". Look up which engines are available first.");
    }

    auto champion = championOutcome.resolved.front();
    auto challenger = challengerOutcome.resolved.front();
    if (champion.getName() == challenger.getName()) {
        return failed("Champion and challenger must be two different engines.");
    }
    // resolveEngines() marks both selected + non-gauntlet; SPRT needs exactly one gauntlet engine
    // (the "engine under test") -- see engineNamesOf() for the same convention read back.
    challenger.setGauntlet(true);

    // setEngineConfigurations() self-persists (see ImGuiEngineSelect::notifyConfigurationChanged())
    // and its registered callback keeps SprtTournamentData::engineConfigurations_ in sync -- no
    // separate updateConfiguration() call needed, same as the tournament's engine selection.
    std::vector<QaplaTester::EngineConfig> configs{champion, challenger};
    SprtTournamentData::instance().getEngineSelect().setEngineConfigurations(configs);

    switchToSprtView();
    return succeeded("Champion (comparison engine): " + champion.getName() +
        ". Challenger (engine under test): " + challenger.getName() + ".");
}

ActionResult configureSprt(const SprtSettings& settings) {
    auto& sprtData = SprtTournamentData::instance();

    // Refused before anything is applied, so a mixed patch cannot leave half its fields in place:
    // partial success is the one outcome a caller has no way to recover from sensibly.
    if (auto locked = settingsLockedReason(sprtData, settings)) {
        return failed(*locked);
    }

    ConfigureOutcome outcome;
    applySettings(sprtData, settings, outcome);

    // All of the above (except time control/concurrency, which self-persist) mutate raw
    // references that don't persist on their own -- see ImGuiSprtConfiguration::
    // updateConfiguration()'s doc comment. Calling all four unconditionally is cheap and always
    // correct either way.
    sprtData.sprtConfiguration().updateConfiguration();
    sprtData.tournamentOpening().updateConfiguration();
    sprtData.tournamentPgn().updateConfiguration();
    sprtData.tournamentAdjudication().updateConfiguration();

    if (!outcome.applied.empty()) {
        switchToSprtView();
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
    message += statusText(sprtData);
    return ActionResult{.ok = outcome.problems.empty(), .text = message, .widget = nullptr,
        .endsTurn = outcome.dialogShown, .remedy = outcome.remedy};
}

ActionResult startSprt() {
    auto& sprtData = SprtTournamentData::instance();
    // "Stop it first" is only the right advice for a test that is actually running. A test that is
    // already on its way out needs the opposite -- waiting -- and telling the caller to stop it
    // again has sent them round a stop/start loop that could never succeed.
    switch (sprtData.state()) {
        case SprtTournamentData::State::Running:
            return failed("An SPRT test is already running. Stop it first if you want to start a "
                          "different one.");
        case SprtTournamentData::State::Starting:
            return failed("An SPRT test is already starting.");
        case SprtTournamentData::State::GracefulStopping:
            return failed("The previous test is still finishing its games. Wait, or stop abruptly "
                          "to end them now.");
        case SprtTournamentData::State::Stopping:
            return failed("The previous test is still stopping. Wait.");
        case SprtTournamentData::State::Stopped:
        default:
            break;
    }

    auto historyCountBefore = QaplaWindows::SnackbarManager::instance().getHistory().size();

    sprtData.startTournament(); // always verbose, same as the classic "Start" button

    if (!sprtData.isRunning() && !sprtData.isStarting()) {
        auto reason = findRecentSprtSnackbar(historyCountBefore);
        return failed(reason.empty() ? "Could not start the SPRT test." : reason);
    }

    sprtData.setPoolConcurrency(sprtData.getExternalConcurrency(), true, true);
    switchToSprtView();
    return succeeded("SPRT test started.");
}

ActionResult stopSprt(StopMode mode) {
    auto& sprtData = SprtTournamentData::instance();
    if (!sprtData.isRunning() && !sprtData.isStarting()) {
        return failed("No SPRT test is currently running.");
    }

    // Reporting "stopping the SPRT test gracefully" for a test that has been stopping gracefully
    // all along reads like the request just took effect, which invites waiting for a change that
    // will never come. Say plainly that nothing changed, and name the one thing that still would.
    if (sprtData.state() == SprtTournamentData::State::GracefulStopping
        && mode == StopMode::Graceful) {
        return succeeded("Already stopping gracefully. Nothing changed. Wait, or stop abruptly.");
    }
    if (sprtData.state() == SprtTournamentData::State::Stopping) {
        return succeeded("Already stopping. Nothing changed.");
    }

    if (mode == StopMode::Abrupt) {
        // Returns a result, not an intention -- see stopTournament() for why this blocks.
        sprtData.stopPoolAbruptlyAndWait();
        switchToSprtView();
        return succeeded("SPRT test stopped.");
    }

    sprtData.stopPool(true);
    switchToSprtView();
    return succeeded("Stopping gracefully. Running games finish first, no new ones start. Not "
                     "done yet.");
}

ActionResult sprtStatus() {
    return succeeded(statusText(SprtTournamentData::instance()));
}

ActionResult clearSprtResult() {
    auto& sprtData = SprtTournamentData::instance();
    if (!sprtData.hasResults()) {
        return succeeded("There are no SPRT results to clear.");
    }

    bool wasRunning = sprtData.isRunning() || sprtData.isStarting();
    sprtData.clear();
    switchToSprtView();
    return succeeded(wasRunning ? "SPRT test stopped and all results cleared."
                                : "All SPRT results have been cleared.");
}

ActionResult showSprtResult() {
    auto& sprtData = SprtTournamentData::instance();
    if (!sprtData.hasResults()) {
        return succeeded("No SPRT results are available yet.");
    }

    // Renders the same two live controls the classic (non-AI) chatbot's SPRT results step draws
    // (see ChatbotStepSprtTournamentResult::draw()): the SPRT test table (LLR/bounds/decision
    // status) and the raw duel win/draw/loss table -- real ImGuiTables, not a text dump. Always
    // reads SprtTournamentData::instance() fresh at draw time, so it reflects the test's current
    // state on every frame.
    return ActionResult{
        .ok = true,
        .text = "Showing the current SPRT results as tables in the chat -- they are already "
                "visible to the user, so do not restate, list, or summarize the numbers in your "
                "reply; just briefly confirm what you did. This is the ONLY way you ever learn "
                "the actual SPRT decision or duel score -- never state, type, or guess one "
                "yourself instead of calling this.",
        .widget = []() {
            auto& data = SprtTournamentData::instance();
            ImGui::Text("SPRT Test Result:");
            data.drawSprtTable(ImVec2(0.0F, 100.0F));
            ImGui::Spacing();
            ImGui::Text("Duel Result:");
            data.drawResultTable(ImVec2(0.0F, 100.0F));
        }};
}

std::string sprtActivityText() {
    using State = SprtTournamentData::State;
    switch (SprtTournamentData::instance().state()) {
        case State::Starting: return "an SPRT test is starting";
        case State::Running: return "an SPRT test is running";
        case State::GracefulStopping:
            // Same reasoning as runStateText()'s GracefulStopping case: never call this "running".
            return "an SPRT test is finishing its games after a graceful stop";
        case State::Stopping: return "an SPRT test is being aborted";
        case State::Stopped:
        default: return "";
    }
}

} // namespace QaplaLlm::Actions
