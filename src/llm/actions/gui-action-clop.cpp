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

#include "gui-action-clop.h"
#include "gui-action-activity.h"
#include "../gui-tool-tournament.h"
#include "../../clop-data.h"

#include <imgui.h>

#include <filesystem>
#include <format>
#include <system_error>

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
    using QaplaWindows::ClopData;

    RunState runStateOf(ClopData::State state) {
        switch (state) {
            case ClopData::State::Starting: return RunState::Starting;
            case ClopData::State::Running: return RunState::Running;
            // CLOP stops one way only: it schedules no further sample and lets the games in
            // flight play out, which is what FinishingAfterGracefulStop describes. There is no
            // abrupt variant to map, so both stop modes arrive here.
            case ClopData::State::Stopping: return RunState::FinishingAfterGracefulStop;
            case ClopData::State::Stopped:
            case ClopData::State::Idle:
            default: return RunState::Idle;
        }
    }

    /** @brief Resolves catalog names, reporting the same way every other engine selection does. */
    struct ResolvedEngines {
        std::optional<ActionResult> refusal{};
        std::vector<QaplaTester::EngineConfig> configs{};
    };

    ResolvedEngines resolve(const std::vector<std::string>& names) {
        auto outcome = resolveEngines(names);
        if (!outcome.ambiguous.empty()) {
            return {.refusal = failed(formatAmbiguousEngineNames(outcome.ambiguous) +
                        " Ask the user which one they mean.")};
        }
        if (!outcome.notFound.empty()) {
            return {.refusal = failed("Not installed: " + joinList(outcome.notFound) +
                        ". Look up which engines are available first.")};
        }
        return {.configs = std::move(outcome.resolved)};
    }

    std::string parameterList(const QaplaTester::CLOPParameterList& parameters) {
        if (parameters.empty()) {
            return "none";
        }
        std::vector<std::string> entries;
        entries.reserve(parameters.size());
        for (const auto& parameter : parameters) {
            entries.push_back(
                std::format("{} ({}..{})", parameter.name, parameter.minValue, parameter.maxValue));
        }
        return joinList(entries);
    }

    std::string estimateText(ClopData& data) {
        const auto estimate = data.estimatedParameters();
        if (estimate.empty()) {
            return "";
        }
        std::vector<std::string> entries;
        entries.reserve(estimate.size());
        for (const auto& [name, value] : estimate) {
            entries.push_back(std::format("{} = {:.2f}", name, value));
        }
        return " Best estimate so far: " + joinList(entries) +
            ". Apply it with the engine tool when you are satisfied with it; nothing is written to "
            "any engine by the run itself.";
    }

    std::string configurationText(ClopData& data) {
        const auto& config = data.config();
        std::string text = std::format(
            "Engine under tuning: {}. Opponents: {}. Parameters: {}. Openings file: {}. "
            "Time control: {}. Samples: {} ({} games each, {} warm-up). Active pairs: {}. "
            "H={:.2f}, prior variance={:.1f}, seed={}. Concurrency: {}.",
            data.optimizedEngineName().empty() ? "none" : data.optimizedEngineName(),
            data.opponentNames().empty() ? "none" : joinList(data.opponentNames()),
            parameterList(config.parameters),
            config.openingsFile.empty() ? "none" : config.openingsFile,
            data.timeControl().empty() ? "not set" : data.timeControl(), config.samples,
            config.gamesPerSample, config.warmupSamples, config.maxActivePairs, config.h,
            config.priorVariance, config.openingsSeed, data.concurrency());

        text += " " + runStateSentence(runStateOf(data.state()), CLOP_NAMES);
        if (data.state() != ClopData::State::Idle) {
            text += std::format(" {} of {} samples completed.", data.completedSamples(),
                config.samples);
        }
        if (const auto lock = lockOf(runStateOf(data.state())); lock != RunLock::None) {
            text += " " + settingsLockNote(lock);
        } else if (data.isReadyToStart()) {
            text += " " + readyToStartSentence();
        }
        return text;
    }
} // namespace

ActionResult configureClop(const ClopSettings& settings) {
    auto& data = ClopData::instance();
    const auto previousTimeControl = data.timeControl();

    if (const auto lock = lockOf(runStateOf(data.state())); lock != RunLock::None) {
        // Concurrency is the exception every activity makes, and for the same reason: it is the
        // only setting that takes effect on a run already going.
        if (settings.concurrency && !settings.engine && settings.opponents.empty() &&
            settings.parameters.empty() && !settings.openingsFile && !settings.samples) {
            data.setConcurrency(*settings.concurrency);
            return succeeded(
                concurrencySentence(runStateOf(data.state()), CLOP_NAMES, *settings.concurrency));
        }
        return failed(settingsLockedSentence(lock, CLOP_NAMES));
    }

    // Engines are written as a pair, like the SPRT test's two roles: the tuned engine carries the
    // gauntlet flag and the rest must not, so naming only one of the two sides would leave the
    // other holding a flag from a previous configuration.
    if (settings.engine || !settings.opponents.empty()) {
        if (!settings.engine || settings.opponents.empty()) {
            return failed("Name the engine under tuning and its opponents together. Nothing "
                          "changed.");
        }
        auto tuned = resolve({*settings.engine});
        if (tuned.refusal) {
            return *tuned.refusal;
        }
        auto opponents = resolve(settings.opponents);
        if (opponents.refusal) {
            return *opponents.refusal;
        }
        data.setEngines(tuned.configs.front(), opponents.configs);
        // Re-applied after a change of engines, because the fresh catalog copies bring whatever
        // clock they were stored with -- usually none.
        if (!settings.timeControl && !previousTimeControl.empty()
            && !data.setTimeControl(previousTimeControl)) {
            return failed("The engines were changed but the time control could not be carried "
                          "over. Set it again.");
        }
    }

    if (settings.timeControl && !data.setTimeControl(*settings.timeControl)) {
        return failed(std::format("\"{}\" is not a usable time control. Use something like "
                                  "\"20+0.1\" (seconds plus increment). Nothing changed.",
            *settings.timeControl));
    }

    auto& config = data.config();
    if (!settings.parameters.empty()) {
        config.parameters.clear();
        for (const auto& parameter : settings.parameters) {
            if (parameter.minimum >= parameter.maximum) {
                return failed(std::format("Parameter {} has no range to search: its lowest value "
                                          "must be below its highest. Nothing changed.",
                    parameter.name));
            }
            config.parameters.push_back({.name = parameter.name,
                .minValue = parameter.minimum,
                .maxValue = parameter.maximum});
        }
    }

    if (settings.openingsFile) {
        std::error_code error;
        if (!std::filesystem::exists(*settings.openingsFile, error)) {
            return failed(std::format("There is no file at {}. Nothing changed.",
                *settings.openingsFile));
        }
        config.openingsFile = *settings.openingsFile;
    }
    if (settings.samples) {
        config.samples = *settings.samples;
    }
    if (settings.gamesPerSample) {
        config.gamesPerSample = *settings.gamesPerSample;
    }
    if (settings.warmupSamples) {
        config.warmupSamples = *settings.warmupSamples;
    }
    if (settings.maxActivePairs) {
        config.maxActivePairs = *settings.maxActivePairs;
    }
    if (settings.seed) {
        config.openingsSeed = *settings.seed;
    }
    if (settings.h) {
        config.h = *settings.h;
    }
    if (settings.priorVariance) {
        config.priorVariance = *settings.priorVariance;
    }
    if (settings.concurrency) {
        data.setConcurrency(*settings.concurrency);
    }

    return succeeded(configurationText(data));
}

ActionResult startClop() {
    auto& data = ClopData::instance();
    if (data.isRunning()) {
        return failed(runStateSentence(runStateOf(data.state()), CLOP_NAMES));
    }
    if (const auto reason = data.start(); !reason.empty()) {
        return failed(reason + " " + configurationText(data));
    }
    // No cross-activity summary here: the start/stop tools append one to every result (see
    // withRunningSummary), so adding it makes the caller read the same sentence twice.
    return succeeded("CLOP tuning run started.");
}

ActionResult stopClop(StopMode mode) {
    auto& data = ClopData::instance();
    if (!data.isRunning()) {
        return failed("No CLOP tuning run is running.");
    }
    // Both modes do the same thing here, and saying so beats silently treating one as the other:
    // CLOP has no abrupt variant, the games in flight always play out.
    (void)mode;
    data.stop();
    return succeeded("CLOP tuning run stopped." + estimateText(data));
}

ActionResult clopStatus() {
    return succeeded(configurationText(ClopData::instance()));
}

ActionResult showClopResult() {
    // No table and no widget, deliberately, and this is where CLOP differs from the other three.
    // Theirs render a results table into the chat and repeat it as text for the model, because
    // the model would otherwise have no numbers at all and was caught inventing them. CLOP's
    // numbers are already in prose: the estimate below, the progress and phase in the status
    // sentence next to it. Repeating them as an ASCII table would cost the caller tokens on every
    // status call for nothing, and would pile duplicate copies into the remote-control window
    // that push the live tables out of view -- those are drawn once, permanently, by
    // ClopData::drawTables().
    auto text = estimateText(ClopData::instance());
    return succeeded(text.empty() ? " No samples have been taken yet." : text);
}

ActionResult clearClopResult() {
    auto& data = ClopData::instance();
    data.clear();
    return succeeded("The CLOP samples were discarded. " + configurationText(data));
}

std::string clopActivityText() {
    auto& data = ClopData::instance();
    return runStatePhrase(runStateOf(data.state()), CLOP_NAMES, data.concurrency());
}

std::optional<ResultTable> clopResultTable() {
    return asResultTable(ClopData::instance().resultsAsTable());
}

ActivityProgress clopProgress() {
    auto& data = ClopData::instance();
    return ActivityProgress{
        .state = runStateOf(data.state()), .finished = data.isFinished()};
}

bool clopIsReadyToStart() {
    return ClopData::instance().isReadyToStart();
}

} // namespace QaplaLlm::Actions
