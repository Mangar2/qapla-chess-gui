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

// Only the qapla executable links this -- epd-data.h pulls in the ImGui/GLFW stack. Engine name
// resolution reuses resolveEngines() from gui-tool-tournament.h, which is pure and tested there.

#include "gui-action-epd.h"
#include "../gui-tool-tournament.h"
#include "../../epd-data.h"
#include "../../snackbar.h"
#include "../../os-dialogs.h"
#include "../../callback-manager.h"

#include <imgui.h>

#include <filesystem>
#include <format>
#include <optional>

namespace QaplaLlm::Actions {

namespace {

/** @brief Carries a drawn table's contents into the actions layer's own, GUI-free vocabulary. */
[[nodiscard]] std::optional<ResultTable> asResultTable(QaplaWindows::TableContents contents) {
    if (contents.rows.empty()) {
        return std::nullopt;
    }
    return ResultTable{.headers = std::move(contents.headers), .rows = std::move(contents.rows)};
}

} // namespace


namespace {
    using QaplaWindows::EpdData;

    // Same message the classic (non-AI) EPD chatbot flow's own "Switch to EPD View" button sends
    // (see chatbot-step-epd-start.cpp) -- ImGuiTabBar subscribes and flips to the Epd tab on the
    // next frame. Called whenever an action actually changes EPD settings or its run state.
    void switchToEpdView() {
        QaplaWindows::StaticCallbacks::message().invokeAll("switch_to_epd_view");
    }

    // Looks for the most recent "epd"-topic snackbar since countBefore entries existed -- the
    // specific reason EpdData::analyse() showed via SnackbarManager for why it silently declined
    // to start (reports failures only that way, same as TournamentData/SprtTournamentData -- see
    // mayAnalyze() in epd-data.cpp).
    std::string findRecentEpdSnackbar(std::size_t countBefore) {
        const auto& history = QaplaWindows::SnackbarManager::instance().getHistory();
        for (auto i = history.size(); i > countBefore; --i) {
            if (history[i - 1].topic == "epd") {
                return history[i - 1].message;
            }
        }
        return "";
    }

    // isStopping() alone can't tell graceful from abrupt (see its doc comment); the distinction
    // matters since a gracefully-stopping analysis is still actively finishing its in-progress
    // positions, just declining to start new ones -- reporting it as plain "running" (or an
    // undifferentiated "stopping") would hide that a stop was already requested. The wording
    // itself lives in gui-action-types.cpp, shared with the tournament and SPRT.
    RunState runStateOf(EpdData& epdData) {
        switch (epdData.state) {
            case EpdData::State::Starting: return RunState::Starting;
            case EpdData::State::Running: return RunState::Running;
            case EpdData::State::Gracefully: return RunState::FinishingAfterGracefulStop;
            case EpdData::State::Stopping: return RunState::Aborting;
            case EpdData::State::Stopped:
            case EpdData::State::Cleared:
            default: return RunState::Idle;
        }
    }

    std::string statusText(EpdData& epdData) {
        std::vector<std::string> engineNames;
        for (const auto& engine : epdData.getEngineSelect().getSelectedEngines()) {
            engineNames.push_back(engine.getName());
        }
        const auto& config = epdData.config();

        std::string runState = runStateSentence(runStateOf(epdData), EPD_NAMES);
        if (epdData.isFinished()) {
            runState += " All positions have been analyzed.";
        } else if (epdData.totalTests > 0) {
            runState += std::format(" Progress: {}/{} positions remaining.",
                epdData.remainingTests, epdData.totalTests);
        }
        // The lock note and the readiness both ride along with the run state -- see the same
        // spot in gui-action-tournament.cpp for why.
        auto lockNote = settingsLockNote(lockOf(runStateOf(epdData)));
        if (!lockNote.empty()) {
            runState += " " + lockNote;
        }
        if (runStateOf(epdData) == RunState::Idle && epdIsReadyToStart()) {
            runState += " " + readyToStartSentence();
        }

        return std::format(
            "Engines: {}. EPD file: {}. Max time per position: {}s. Min time per position: {}s. "
            "Seen plies (early stop): {}. Concurrency: {}. {}",
            engineNames.empty() ? "none selected" : joinList(engineNames),
            config.filepath.empty() ? "(not set)" : config.filepath,
            config.maxTimeInS, config.minTimeInS, config.seenPlies,
            epdData.getExternalConcurrency(),
            runState);
    }

    struct ConfigureOutcome {
        std::vector<std::string> applied;
        std::vector<std::string> problems;
        bool dialogShown = false;
        Remedy remedy = Remedy::None;
        // Set when the patch changed concurrency; lets configureEpd() answer a concurrency-only
        // patch with just this sentence instead of the full status.
        std::optional<std::string> concurrencyNote;
    };

    // See concurrencySentence() for why concurrency answers with a sentence of its own, and what
    // that sentence has to do. applySettings() applies the value live under exactly the same
    // condition, so text and behaviour cannot drift apart.
    std::string concurrencySummary(EpdData& data, uint32_t count) {
        return concurrencySentence(runStateOf(data), EPD_NAMES, count);
    }

    // Everything except concurrency is fixed once the analysis starts, exactly as for the classic
    // tournament (see gui-action-tournament.cpp's settingsLockedReason()). Per-position time
    // limits in particular decide what "solved" means, so changing them mid-run would leave one
    // result table whose rows were measured under two different rules.
    bool changesMoreThanConcurrency(const EpdSettings& settings) {
        return settings.epdFile || settings.pickEpdFile || settings.maxTimeInSeconds
            || settings.minTimeInSeconds || settings.seenPlies;
    }

    // Why the patch is refused, or nullopt if it may go through. The rationale (the per-position
    // time limits decide what "solved" means, so mixing them mixes the result table) belongs
    // here, not in the message: the model needs the rule and the next step, nothing else.
    std::optional<std::string> settingsLockedReason(EpdData& data, const EpdSettings& settings) {
        if (!changesMoreThanConcurrency(settings)) {
            return std::nullopt;
        }
        auto sentence = settingsLockedSentence(lockOf(runStateOf(data)), EPD_NAMES);
        if (sentence.empty()) {
            return std::nullopt;
        }
        return sentence;
    }

    // The engines are what the result table reports on, so they are frozen for exactly as long as
    // the settings are, and refused in the same words.
    std::optional<std::string> engineSelectionLockedReason(EpdData& data) {
        auto sentence = settingsLockedSentence(lockOf(runStateOf(data)), EPD_NAMES);
        if (sentence.empty()) {
            return std::nullopt;
        }
        return sentence;
    }

    void pickEpdFile(EpdData& data, ConfigureOutcome& outcome) {
        outcome.dialogShown = true;
        auto paths = QaplaWindows::OsDialogs::openFileDialog(false);
        if (paths.empty()) {
            outcome.applied.push_back("EPD file (dialog cancelled, unchanged)");
            return;
        }
        data.config().filepath = paths.front();
        outcome.applied.push_back("EPD file selected: " + paths.front());
    }

    void applySettings(EpdData& data, const EpdSettings& settings, ConfigureOutcome& outcome) {
        if (settings.pickEpdFile) {
            pickEpdFile(data, outcome);
        } else if (settings.epdFile) {
            if (!std::filesystem::exists(*settings.epdFile)) {
                outcome.problems.push_back("EPD file not found: " + *settings.epdFile);
                outcome.remedy = Remedy::AskUserForEpdFile;
            } else {
                data.config().filepath = *settings.epdFile;
                outcome.applied.push_back("EPD file: " + *settings.epdFile);
            }
        }
        if (settings.maxTimeInSeconds) {
            if (*settings.maxTimeInSeconds < 1) {
                outcome.problems.push_back("max time per position must be at least 1 second");
            } else {
                data.config().maxTimeInS = *settings.maxTimeInSeconds;
                outcome.applied.push_back(
                    "max time per position=" + std::to_string(data.config().maxTimeInS) + "s");
            }
        }
        if (settings.minTimeInSeconds) {
            data.config().minTimeInS = *settings.minTimeInSeconds;
            outcome.applied.push_back(
                "min time per position=" + std::to_string(data.config().minTimeInS) + "s");
        }
        if (settings.seenPlies) {
            data.config().seenPlies = *settings.seenPlies;
            outcome.applied.push_back("seen plies=" + std::to_string(data.config().seenPlies));
        }
        if (settings.concurrency) {
            if (*settings.concurrency < 1) {
                outcome.problems.push_back("concurrency must be at least 1");
            } else {
                data.setExternalConcurrency(*settings.concurrency);
                // Push it into the live pool only while the analysis is really running -- this is
                // what makes the change take effect mid-run instead of only at the next start.
                // Not during Gracefully: raising concurrency there would hand the pool room for
                // more positions while the analysis is supposed to start none, contradicting both
                // the graceful stop and what concurrencySummary() reports for that state.
                if (data.state == EpdData::State::Running) {
                    data.setPoolConcurrency(*settings.concurrency, true, true);
                }
                outcome.concurrencyNote = concurrencySummary(data, *settings.concurrency);
                outcome.applied.push_back(*outcome.concurrencyNote);
            }
        }
    }
} // namespace

ActionResult selectEpdEngines(const std::vector<std::string>& engineNames) {
    if (engineNames.empty()) {
        return failed("No engine names were given.");
    }
    if (auto locked = engineSelectionLockedReason(EpdData::instance())) {
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

    // setEngineConfigurations() self-persists (see ImGuiEngineSelect::notifyConfigurationChanged())
    // and its registered callback keeps EpdData's own engine list in sync -- no separate
    // persistence call needed, same as the tournament's and SPRT's engine selection.
    EpdData::instance().getEngineSelect().setEngineConfigurations(outcome.resolved);

    // Reports only what the caller cannot see anyway -- see selectTournamentEngines() for why.
    std::string message;
    if (!outcome.notFound.empty()) {
        message = "Not installed (skipped): " + joinList(outcome.notFound) + ".";
    }
    switchToEpdView();
    return succeeded(message);
}

ActionResult configureEpd(const EpdSettings& settings) {
    auto& epdData = EpdData::instance();

    // Refused before anything is applied, so a mixed patch cannot leave half its fields in place:
    // partial success is the one outcome a caller has no way to recover from sensibly.
    if (auto locked = settingsLockedReason(epdData, settings)) {
        return failed(*locked);
    }

    ConfigureOutcome outcome;
    applySettings(epdData, settings, outcome);

    // config() is a raw reference -- mutating it directly doesn't persist on its own. Unlike the
    // tournament (split across three sub-components' updateConfiguration()), EpdData's own
    // updateConfiguration() alone already covers everything set above in one call.
    epdData.updateConfiguration();

    if (!outcome.applied.empty()) {
        switchToEpdView();
    }

    // A patch that changed nothing but concurrency answers with just that one sentence, rather
    // than wrapping it into the "Configured: ..." list where it reads like a queued-up setting.
    if (outcome.problems.empty() && outcome.concurrencyNote && outcome.applied.size() == 1) {
        return ActionResult{.ok = true, .text = *outcome.concurrencyNote, .widget = nullptr,
            .endsTurn = outcome.dialogShown, .remedy = outcome.remedy};
    }

    // Ends with the full status, exactly as the tournament and SPRT configuration actions do: a
    // caller then never needs a second round-trip to see what its own change produced, and the
    // three read alike. The list of applied fields is dropped for the same reason -- the status
    // below states every one of them as it now stands.
    std::string message;
    if (!outcome.problems.empty()) {
        message = "Problems: " + joinList(outcome.problems) + ". ";
    }
    message += statusText(epdData);
    return ActionResult{.ok = outcome.problems.empty(), .text = message, .widget = nullptr,
        .endsTurn = outcome.dialogShown, .remedy = outcome.remedy};
}

ActionResult startEpd() {
    auto& epdData = EpdData::instance();
    // Switches on the state rather than isRunning()/isStarting(): EpdData::isRunning() is strictly
    // state == Running, so neither predicate covers Gracefully or Stopping -- a start requested
    // while the previous analysis was still winding down slipped straight through this guard and
    // into analyse(). Beyond that, "stop it first" is only the right advice for an analysis that
    // is actually running; one already on its way out needs the opposite, waiting.
    switch (epdData.state) {
        case EpdData::State::Running:
            return failed("An EPD analysis is already running. Stop it first if you want to start "
                          "a different one.");
        case EpdData::State::Starting:
            return failed("An EPD analysis is already starting.");
        case EpdData::State::Gracefully:
            return failed("The previous analysis is still finishing its positions. Wait, or stop "
                          "abruptly to end them now.");
        case EpdData::State::Stopping:
            return failed("The previous analysis is still stopping. Wait.");
        case EpdData::State::Stopped:
        case EpdData::State::Cleared:
        default:
            break;
    }

    auto historyCountBefore = QaplaWindows::SnackbarManager::instance().getHistory().size();

    epdData.analyse(); // starts fresh or resumes -- see this action's doc comment

    if (!epdData.isRunning() && !epdData.isStarting()) {
        auto reason = findRecentEpdSnackbar(historyCountBefore);
        if (reason.empty()) {
            return failed("Could not start the EPD analysis.");
        }
        // This exact rejection (see EpdData::mayAnalyze()) has no tournament/SPRT equivalent, so
        // name the way out rather than relying on the reader to connect "clear data" to the
        // clear-results action on its own.
        auto remedy = reason.find("Clear data before re-analyzing") != std::string::npos
            ? Remedy::ClearEpdResultFirst
            : Remedy::None;
        return ActionResult{.ok = false, .text = reason, .widget = nullptr, .endsTurn = false,
            .remedy = remedy};
    }

    switchToEpdView();
    return succeeded("EPD analysis started.");
}

ActionResult stopEpd(StopMode mode) {
    auto& epdData = EpdData::instance();
    // Same reason startEpd() switches on the state: isRunning() is strictly state == Running, so
    // this used to answer "No EPD analysis is currently running" for an analysis that was still
    // finishing its positions after a graceful stop -- and then refuse the abrupt stop that would
    // actually have ended it.
    if (epdData.state == EpdData::State::Gracefully && mode == StopMode::Graceful) {
        return succeeded("Already stopping gracefully. Nothing changed. Wait, or stop abruptly.");
    }
    if (epdData.state == EpdData::State::Stopping) {
        return succeeded("Already stopping. Nothing changed.");
    }
    if (!epdData.isRunning() && !epdData.isStarting()
        && epdData.state != EpdData::State::Gracefully) {
        return failed("No EPD analysis is currently running.");
    }

    if (mode == StopMode::Abrupt) {
        // Returns a result, not an intention -- see stopTournament() for why this blocks.
        epdData.stopPoolAbruptlyAndWait();
        switchToEpdView();
        return succeeded("EPD analysis stopped.");
    }

    epdData.stopPool(true);
    switchToEpdView();
    return succeeded("Stopping gracefully. Running positions finish first, no new ones start. Not "
                     "done yet.");
}

ActionResult epdStatus() {
    return succeeded(statusText(EpdData::instance()));
}

ActionResult clearEpdResult() {
    auto& epdData = EpdData::instance();
    if (epdData.totalTests == 0) {
        return succeeded("There are no EPD analysis results to clear.");
    }

    bool wasRunning = epdData.isRunning() || epdData.isStarting();
    epdData.clear();
    switchToEpdView();
    return succeeded(wasRunning ? "EPD analysis stopped and all results cleared."
                                : "All EPD analysis results have been cleared.");
}

ActionResult showEpdResult() {
    auto& epdData = EpdData::instance();
    if (epdData.totalTests == 0) {
        return succeeded("No EPD analysis results are available yet.");
    }

    // Renders the same live control the classic (non-AI) EPD view draws (see EpdData::drawTable(),
    // used from EpdWindow::draw()) -- a real ImGuiTable with one row per position and one column
    // per engine, not a text dump. Always reads EpdData::instance() fresh at draw time, so it
    // reflects the analysis's current state.
    return ActionResult{
        .ok = true,
        // Table and text from one set of numbers -- see showTournamentResult() for why both.
        .text = "The analyzed positions are now shown as a table in the chat, and repeated below "
                "so you can answer questions about them. The user can already see them, so don't "
                "list or summarize the rows back -- just answer what was asked, and never state "
                "which positions were solved unless it came from here.\n"
            + epdData.resultsAsText(),
        .widget = []() {
            auto& data = EpdData::instance();
            ImGui::Text("EPD Analysis Progress: %zu / %zu positions remaining",
                data.remainingTests, data.totalTests);
            ImGui::Spacing();
            data.drawTable(ImVec2(0.0F, 3000.0F));
        }};
}

ActionResult saveEpdToFile(const std::string& file) {
    auto& epdData = EpdData::instance();
    auto locked = fileLockedSentence(lockOf(runStateOf(epdData)), EPD_NAMES, FileAccess::Save);
    if (!locked.empty()) {
        return failed(locked);
    }

    // Refused rather than written, unlike the tournament and SPRT files: those carry the whole
    // configuration, so saving one before a single game is a perfectly good way to keep a setup,
    // whereas this file holds nothing but results. Writing the empty one and reporting success
    // would leave the caller believing a run had been kept, and only the eventual load -- which
    // cannot tell an empty file from a mismatched one -- would say otherwise.
    if (epdData.totalTests == 0) {
        return failed("There are no EPD results to save yet; nothing was written. This file would "
                      "hold results only, so there would be nothing in it. Run the analysis "
                      "first (start, type=\"epd\").");
    }

    auto reason = epdData.saveResultsTo(file);
    if (!reason.empty()) {
        return failed("The EPD results were not saved: " + reason + ".");
    }
    // The one thing a caller must know before it puts the file somewhere and forgets about it:
    // read back against a different position file it matches nothing, and there is no error to
    // find out from -- the file does not name the positions it was recorded against.
    return succeeded("EPD results saved to " + file
        + ". The file holds the per-position results only, not the position file, engines or "
          "timing -- reading it back needs the same epd_file configured.");
}

ActionResult loadEpdFromFile(const std::string& file) {
    auto& epdData = EpdData::instance();
    auto locked = fileLockedSentence(lockOf(runStateOf(epdData)), EPD_NAMES, FileAccess::Load);
    if (!locked.empty()) {
        return failed(locked);
    }

    auto reason = epdData.loadResultsFrom(file);
    if (!reason.empty()) {
        return failed("Nothing was loaded: " + reason
            + ". The EPD analysis is unchanged. Have the user check the path, and that epd_file "
              "is the position file these results were recorded against.");
    }
    switchToEpdView();
    return succeeded("EPD results loaded from " + file + ". " + statusText(epdData));
}

std::string epdActivityText() {
    auto& epdData = EpdData::instance();
    return runStatePhrase(runStateOf(epdData), EPD_NAMES, epdData.getExternalConcurrency());
}

std::optional<ResultTable> epdResultTable() {
    return asResultTable(EpdData::instance().resultsAsTable());
}

ActivityProgress epdProgress() {
    auto& epdData = EpdData::instance();
    return ActivityProgress{.state = runStateOf(epdData), .finished = epdData.isFinished()};
}

bool epdIsReadyToStart() {
    // The whole check, and the authority on it: mayAnalyze() is the very predicate analyse()
    // consults, covering the EPD file, the engines, the per-position time and the resume rule.
    // Its yes/no is all that is reported; the reasons it keeps to its snackbar stay there.
    return EpdData::instance().mayAnalyze(false);
}

} // namespace QaplaLlm::Actions
