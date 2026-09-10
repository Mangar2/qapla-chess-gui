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

// Only the qapla executable links this -- analysis-data.h pulls in the ImGui stack. Engine name
// resolution reuses resolveEngines() from gui-tool-tournament.h, which is pure and tested there.

#include "gui-action-analysis.h"
#include "../gui-tool-tournament.h"
#include "../../analysis-data.h"

#include <filesystem>
#include <format>

namespace QaplaLlm::Actions {

namespace {
    using QaplaWindows::AnalysisData;

    /** @brief The backward analysis's own states in the one vocabulary every activity reports in. */
    [[nodiscard]] RunState runStateOf(const AnalysisData& data) {
        switch (data.getState()) {
            case AnalysisData::State::Starting: return RunState::Starting;
            case AnalysisData::State::Running: return RunState::Running;
            case AnalysisData::State::Gracefully: return RunState::FinishingAfterGracefulStop;
            case AnalysisData::State::Stopping: return RunState::Aborting;
            case AnalysisData::State::Stopped:
            default: return RunState::Idle;
        }
    }

    /** @brief Why a settings change is refused right now, or nothing when it may go through. */
    [[nodiscard]] std::optional<std::string> lockedReason(AnalysisData& data) {
        auto sentence = settingsLockedSentence(lockOf(runStateOf(data)), ANALYSIS_NAMES);
        if (sentence.empty()) {
            return std::nullopt;
        }
        return sentence;
    }

    [[nodiscard]] std::vector<std::string> engineNamesOf(AnalysisData& data) {
        std::vector<std::string> names;
        for (const auto& engine : data.getEngineSelect().getSelectedEngines()) {
            names.push_back(engine.getName());
        }
        return names;
    }

    /**
     * @brief Whether the file can be written where it is: its directory has to exist.
     */
    [[nodiscard]] bool directoryExists(const std::string& file) {
        try {
            const auto parent = std::filesystem::path(file).parent_path();
            return parent.empty() || std::filesystem::exists(parent);
        }
        catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }

    [[nodiscard]] std::string progressSentence(AnalysisData& data) {
        if (data.getTotalCount() == 0) {
            return {};
        }
        return std::format(" {} of {} games analysed.", data.getFinishedCount(),
            data.getTotalCount());
    }

    [[nodiscard]] std::string statusText(AnalysisData& data) {
        const auto& output = data.outputPgn().pgnOptions();
        const auto names = engineNamesOf(data);
        return std::format(
            "Engines: {}. Games read from: {}. Time per position: {} ms. Analysed games written "
            "to: {} ({}). Concurrency: {}. {}{}",
            names.empty() ? "none selected" : joinList(names),
            data.config().inputFile.empty() ? "(not set)" : data.config().inputFile,
            data.config().moveTimeMs,
            output.file.empty() ? "(not set)" : output.file,
            output.append ? "append" : "overwrite",
            data.getExternalConcurrency(),
            runStateSentence(runStateOf(data), ANALYSIS_NAMES),
            progressSentence(data));
    }
} // namespace

ActionResult selectAnalysisEngines(const std::vector<std::string>& engineNames) {
    if (engineNames.empty()) {
        return failed("No engine names were given.");
    }
    auto& data = AnalysisData::instance();
    if (auto locked = lockedReason(data)) {
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

    data.getEngineSelect().setEngineConfigurations(outcome.resolved);

    std::string message;
    if (!outcome.notFound.empty()) {
        message = "Not installed (skipped): " + joinList(outcome.notFound) + ".";
    }
    return succeeded(message);
}

ActionResult configureAnalysis(const AnalysisSettings& settings) {
    auto& data = AnalysisData::instance();

    // Concurrency is the one setting a running analysis still takes: it changes how many games
    // run at once and nothing about what is being computed.
    const bool onlyConcurrency = settings.concurrency && !settings.pgnFile && !settings.moveTimeMs
        && !settings.outputFile && !settings.appendOutput;
    if (auto locked = lockedReason(data); locked && !onlyConcurrency) {
        return failed(*locked);
    }

    std::vector<std::string> problems;

    if (settings.pgnFile) {
        if (!std::filesystem::exists(*settings.pgnFile)) {
            problems.push_back(std::format("The PGN file does not exist: {}", *settings.pgnFile));
        } else {
            data.config().inputFile = *settings.pgnFile;
        }
    }
    if (settings.moveTimeMs) {
        if (*settings.moveTimeMs == 0) {
            problems.emplace_back("The time per position has to be greater than zero.");
        } else {
            data.config().moveTimeMs = *settings.moveTimeMs;
        }
    }
    if (settings.outputFile) {
        if (!directoryExists(*settings.outputFile)) {
            problems.push_back(std::format("The directory of the output file does not exist: {}",
                *settings.outputFile));
        } else {
            data.outputPgn().pgnOptions().file = *settings.outputFile;
            data.outputPgn().updateConfiguration();
        }
    }
    if (settings.appendOutput) {
        data.outputPgn().pgnOptions().append = *settings.appendOutput;
        data.outputPgn().updateConfiguration();
    }
    if (settings.concurrency) {
        data.setExternalConcurrency(*settings.concurrency);
        data.setPoolConcurrency(*settings.concurrency, true, true);
    }
    data.updateConfiguration();

    if (!problems.empty()) {
        // Reported together with what did go through, so a caller can tell the two apart without
        // asking again.
        return failed(joinList(problems) + " " + statusText(data));
    }
    return succeeded(statusText(data));
}

ActionResult startAnalysis() {
    auto& data = AnalysisData::instance();
    switch (data.getState()) {
        case AnalysisData::State::Running:
            return failed("A backward analysis is already running. Stop it first if you want to "
                          "start a different one.");
        case AnalysisData::State::Starting:
            return failed("A backward analysis is already starting.");
        case AnalysisData::State::Gracefully:
            return failed("The previous analysis is still finishing its games. Wait, or stop it "
                          "abruptly to end them now.");
        case AnalysisData::State::Stopping:
            return failed("The previous analysis is still stopping. Wait.");
        case AnalysisData::State::Stopped:
        default:
            break;
    }

    std::string error;
    const auto gameCount = data.loadGamesFromInputFile(error);
    if (gameCount == 0) {
        return failed(error.empty() ? "The games to analyse could not be read." : error);
    }

    data.analyse();
    if (!data.isBusy()) {
        // analyse() says through the snackbar what it refused and why; the caller gets the same
        // reasons by being told what the analysis looks like right now.
        return failed("The backward analysis did not start. " + statusText(data));
    }
    return succeeded(std::format("Backward analysis started on {} games, from the last move of "
                                 "each game back to its first. {}", gameCount, statusText(data)));
}

ActionResult stopAnalysis(StopMode mode) {
    auto& data = AnalysisData::instance();
    if (!data.isBusy()) {
        return failed("No backward analysis is running.");
    }
    data.stop(mode == StopMode::Graceful);
    return succeeded(std::format("Backward analysis stopped.{}", progressSentence(data)));
}

ActionResult analysisStatus() {
    return succeeded(statusText(AnalysisData::instance()));
}

ActivityProgress analysisProgress() {
    auto& data = AnalysisData::instance();
    const auto state = runStateOf(data);
    return ActivityProgress{
        .state = state,
        // Finished means it ran to its own end, not that it merely is not running: every game it
        // was given came back.
        .finished = state == RunState::Idle && data.getTotalCount() > 0
            && data.getFinishedCount() >= data.getTotalCount()};
}

bool analysisIsReadyToStart() {
    auto& data = AnalysisData::instance();
    return runStateOf(data) == RunState::Idle
        && !data.config().inputFile.empty()
        && !data.outputPgn().pgnOptions().file.empty()
        && data.config().moveTimeMs > 0
        && !data.getEngineSelect().getSelectedEngines().empty();
}

ActionResult clearAnalysisResult() {
    auto& data = AnalysisData::instance();
    const bool wasRunning = data.isBusy();
    if (wasRunning) {
        data.stop(false);
    }
    if (!wasRunning && data.getGameCount() == 0 && data.getTotalCount() == 0) {
        return succeeded("There is no backward analysis to clear.");
    }
    data.setGames({});
    return succeeded(wasRunning
        ? "Backward analysis stopped and its games forgotten. The games it had already analysed "
          "are in the output file; that file is left as it is."
        : "The games of the last backward analysis have been forgotten. The output file is left "
          "as it is.");
}

} // namespace QaplaLlm::Actions
