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

// Only the qapla executable links this: OsDialogs and Configuration transitively pull in the
// ImGui/GLFW stack. The pure part (addEnginesFromPaths/listInstalledEnginesJson) lives in
// gui-tool-engine-management.cpp and is unit-tested there.

#include "gui-action-engines.h"
#include "gui-action-epd.h"
#include "gui-action-sprt.h"
#include "gui-action-tournament.h"
#include "../gui-tool-engine-management.h"
#include "../gui-tool-tournament.h"
#include "../../callback-manager.h"
#include "../../configuration.h"
#include "../../epd-data.h"
#include "../../os-dialogs.h"
#include "../../sprt-tournament-data.h"
#include "../../tournament-data.h"

#include <engine-handling/engine-worker-factory.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <optional>
#include <system_error>

namespace QaplaLlm::Actions {

namespace {

    /** @brief What one option target is called and how it reports its own state. */
    struct TargetTraits {
        const ActivityNames* names;              ///< nullptr for the catalog, which never runs.
        std::string_view label;                  ///< "the engine catalog", "the SPRT test"
        std::string_view tabMessage;             ///< Tab to flip to so the change is visible; "" for none.
    };

    TargetTraits traitsOf(EngineTarget target) {
        switch (target) {
            case EngineTarget::Tournament:
                return {&TOURNAMENT_NAMES, "the tournament", "switch_to_tournament_tab"};
            case EngineTarget::Sprt:
                return {&SPRT_NAMES, "the SPRT test", "switch_to_sprt_tab"};
            case EngineTarget::Epd:
                return {&EPD_NAMES, "the EPD analysis", "switch_to_epd_tab"};
            case EngineTarget::Catalog:
            default:
                return {nullptr, "the engine catalog", ""};
        }
    }

    /** @brief The run state of a target, or Idle for the catalog. */
    RunState stateOf(EngineTarget target) {
        switch (target) {
            case EngineTarget::Tournament: return tournamentProgress().state;
            case EngineTarget::Sprt: return sprtProgress().state;
            case EngineTarget::Epd: return epdProgress().state;
            case EngineTarget::Catalog:
            default: return RunState::Idle;
        }
    }

    /**
     * @brief The engine selection of a run target.
     *
     * Returned as a pointer to the live ImGuiEngineSelect rather than a copy, because writing the
     * change back has to go through the same object: setEngineConfigurations() self-persists and
     * notifies the data singleton (see selectSprtEngines() for the same round trip).
     */
    QaplaWindows::ImGuiEngineSelect* selectionOf(EngineTarget target) {
        switch (target) {
            case EngineTarget::Tournament:
                return &QaplaWindows::TournamentData::instance().getEngineSelect();
            case EngineTarget::Sprt:
                return &QaplaWindows::SprtTournamentData::instance().getEngineSelect();
            case EngineTarget::Epd:
                return &QaplaWindows::EpdData::instance().getEngineSelect();
            case EngineTarget::Catalog:
            default:
                return nullptr;
        }
    }

    /** @brief Renders an ApplyOptionsOutcome as the one sentence group a caller has to read. */
    std::string describeOutcome(
        const ApplyOptionsOutcome& outcome, const std::string& engineName, std::string_view where) {
        std::string text;
        if (!outcome.applied.empty()) {
            text += std::format(
                "Set on {} in {}: {}.", engineName, where, joinList(outcome.applied));
        }
        if (!outcome.unknown.empty()) {
            if (!text.empty()) {
                text += " ";
            }
            text += std::format("{} does not offer: {}. Nothing was set for those.", engineName,
                joinList(outcome.unknown));
        }
        if (!outcome.rejected.empty()) {
            if (!text.empty()) {
                text += " ";
            }
            text += "Out of range, left unchanged: " + joinList(outcome.rejected) + ".";
        }
        return text;
    }

} // namespace

ActionResult listInstalledEngines() {
    return succeeded(listInstalledEnginesJson());
}

ActionResult addEnginesViaDialog() {
    auto paths = QaplaWindows::OsDialogs::openFileDialog(true);
    if (paths.empty()) {
        return ActionResult{.ok = true,
            .text = "The user cancelled the dialog; no engine was added.",
            .widget = nullptr, .endsTurn = true};
    }

    auto outcome = addEnginesFromPaths(paths);
    if (!outcome.addedNames.empty()) {
        auto& configuration = QaplaConfiguration::Configuration::instance();
        configuration.setModified();
        // Synchronous and blocking (not the fire-and-forget autoDetect() the "Detect" button
        // uses): detection is normally sub-second per engine, and this result must reflect the
        // *actual* final outcome, not a promise of one -- any out-of-band "done" signal is not
        // correlated with this call, so it could arrive before or long after the result does.
        configuration.getEngineCapabilities().autoDetectSync();
    }

    std::string message;
    if (!outcome.addedNames.empty()) {
        message += "Added and detected: " + joinList(outcome.addedNames) + ". Ready to use.";
    }
    if (!outcome.duplicateNames.empty()) {
        if (!message.empty()) {
            message += "\n";
        }
        message += "Already configured (skipped): " + joinList(outcome.duplicateNames) + ".";
    }
    if (message.empty()) {
        message = "No engines were added.";
    }
    return ActionResult{.ok = true, .text = message, .widget = nullptr, .endsTurn = true};
}

ActionResult installEngines(const std::vector<NamedEnginePath>& engines) {
    // Checked here rather than in addNamedEngines(), which stays free of the filesystem so it can
    // be unit-tested with fabricated paths. A catalog entry outlives the call that made it, so a
    // typo that got stored would come back as a mysterious start failure much later.
    std::vector<NamedEnginePath> usable;
    std::vector<std::string> missing;
    for (const auto& engine : engines) {
        std::error_code error;
        if (std::filesystem::exists(engine.path, error) &&
            !std::filesystem::is_directory(engine.path, error)) {
            usable.push_back(engine);
        } else {
            missing.push_back(engine.path);
        }
    }

    AddNamedEnginesOutcome outcome;
    if (!usable.empty()) {
        outcome = addNamedEngines(usable);
    }

    std::vector<std::string> reports;
    if (!outcome.addedNames.empty()) {
        auto& configuration = QaplaConfiguration::Configuration::instance();
        configuration.setModified();
        // Synchronous, like addEnginesViaDialog(): detection is what turns an executable into
        // something configurable, and a caller that installs an engine is about to set its
        // options. Reporting "added, ask again later" would make that a guessing game.
        configuration.getEngineCapabilities().autoDetectSync();

        const auto& capabilities = configuration.getEngineCapabilities();
        const auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
        for (const auto& name : outcome.addedNames) {
            const auto* config = configManager.getConfig(name);
            if (config == nullptr) {
                // Cannot happen while names are the catalog's key and detection leaves a chosen
                // name alone -- reported rather than skipped, because a silently dropped entry
                // reads as "nothing was installed" for an engine that in fact was.
                reports.push_back(std::format("{} (added, but it can no longer be found by name)",
                    name));
                continue;
            }
            const auto capability =
                capabilities.getCapability(config->getCmd(), config->getProtocol());
            if (!capability) {
                reports.push_back(std::format("{} (added, but it did not answer -- it reports no "
                                              "protocol and no options)", name));
                continue;
            }
            reports.push_back(std::format("{} ({}, {} options)", name,
                QaplaTester::to_string(config->getProtocol()),
                capability->getSupportedOptions().size()));
        }
    }

    std::string message;
    if (!reports.empty()) {
        message += "Installed and detected: " + joinList(reports) + ".";
    }
    if (!outcome.takenNames.empty()) {
        if (!message.empty()) {
            message += " ";
        }
        message += "These names are already in use, so nothing was installed under them: " +
            joinList(outcome.takenNames) + ". Pick different names, or use the engines already "
            "there.";
    }
    if (!missing.empty()) {
        if (!message.empty()) {
            message += " ";
        }
        message += "No such file: " + joinList(missing) + ".";
    }
    if (message.empty()) {
        message = "No engines were installed.";
    }
    return missing.empty() && outcome.takenNames.empty() ? succeeded(message) : failed(message);
}

ActionResult engineDetails(const std::string& name) {
    auto outcome = resolveEngines({name});
    if (!outcome.ambiguous.empty()) {
        return failed(
            formatAmbiguousEngineNames(outcome.ambiguous) + " Ask the user which one they mean.");
    }
    if (outcome.resolved.empty()) {
        return failed(std::format("\"{}\" is not in the engine catalog. List the installed engines "
                                  "to see what is there.", name));
    }
    return succeeded(engineDetailsText(outcome.resolved.front(),
        QaplaConfiguration::Configuration::instance().getEngineCapabilities()));
}

ActionResult setEngineOptions(EngineTarget target, const std::string& engineName,
    const std::vector<EngineOptionAssignment>& options) {
    const auto traits = traitsOf(target);

    // A run's engine configuration is frozen for exactly as long as its settings are, and for the
    // same reason: one result table measured under two option sets is not a result.
    if (traits.names != nullptr) {
        if (const auto lock = lockOf(stateOf(target)); lock != RunLock::None) {
            return failed(settingsLockedSentence(lock, *traits.names));
        }
    }

    auto resolution = resolveEngines({engineName});
    if (!resolution.ambiguous.empty()) {
        return failed(formatAmbiguousEngineNames(resolution.ambiguous) +
            " Ask the user which one they mean.");
    }
    if (resolution.resolved.empty()) {
        return failed(std::format("\"{}\" is not in the engine catalog. List the installed engines "
                                  "to see what is there.", engineName));
    }
    const auto canonicalName = resolution.resolved.front().getName();

    auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
    ApplyOptionsOutcome outcome;

    if (target == EngineTarget::Catalog) {
        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
        auto* config = configManager.getConfigMutable(canonicalName);
        if (config == nullptr) {
            return failed(std::format("\"{}\" is not in the engine catalog.", canonicalName));
        }
        outcome = applyEngineOptions(*config, options, capabilities);
    } else {
        auto* selection = selectionOf(target);
        auto configs = selection->getEngineConfigurations();
        auto match = std::ranges::find_if(configs, [&](const auto& config) {
            return config.getName() == canonicalName;
        });
        if (match == configs.end()) {
            return failed(std::format("{} is not among the engines of {}. Select it there first; "
                                      "selecting copies the engine out of the catalog, and this "
                                      "sets options on that copy.", canonicalName, traits.label));
        }
        outcome = applyEngineOptions(*match, options, capabilities);
        if (!outcome.applied.empty()) {
            // Write the whole list back, not just the one entry: setEngineConfigurations() is what
            // persists and notifies (see selectSprtEngines()), and it takes the set as a whole.
            selection->setEngineConfigurations(configs);
        }
    }

    if (!outcome.detected) {
        return failed(std::format("{} has not reported what it supports, so nothing was set. It "
                                  "did not answer when it was installed.", canonicalName));
    }

    if (outcome.applied.empty() && outcome.unknown.empty() && outcome.rejected.empty()) {
        return failed("No options were given, so nothing changed.");
    }

    if (!outcome.applied.empty()) {
        if (target == EngineTarget::Catalog) {
            QaplaConfiguration::Configuration::instance().setModified();
        } else if (!traits.tabMessage.empty()) {
            QaplaWindows::StaticCallbacks::message().invokeAll(std::string(traits.tabMessage));
        }
    }

    auto message = describeOutcome(outcome, canonicalName, traits.label);
    return outcome.unknown.empty() && outcome.rejected.empty() ? succeeded(message)
                                                               : failed(message);
}

} // namespace QaplaLlm::Actions
