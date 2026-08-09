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

namespace QaplaLlm::Actions {

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

    std::string statusText(EpdData& epdData) {
        std::vector<std::string> engineNames;
        for (const auto& engine : epdData.getEngineSelect().getSelectedEngines()) {
            engineNames.push_back(engine.getName());
        }
        const auto& config = epdData.config();

        // isStopping() alone can't tell graceful from abrupt (see its doc comment); the
        // distinction matters here since a gracefully-stopping analysis is still actively
        // finishing its in-progress positions, just declining to start new ones -- reporting it
        // as plain "running" (or an undifferentiated "stopping") would hide that a stop was
        // already requested.
        std::string runState = "No EPD analysis is currently running.";
        if (epdData.isStarting()) {
            runState = "An EPD analysis is currently starting.";
        } else if (epdData.isRunning()) {
            runState = "An EPD analysis is currently running.";
        } else if (epdData.state == EpdData::State::Gracefully) {
            runState = "An EPD analysis is currently running but stopping gracefully -- "
                       "in-progress positions will finish, no new ones will start.";
        } else if (epdData.state == EpdData::State::Stopping) {
            runState = "An EPD analysis is currently stopping abruptly -- in-progress positions "
                       "are being aborted.";
        }
        if (epdData.isFinished()) {
            runState += " All positions have been analyzed -- show the EPD result to see it.";
        } else if (epdData.totalTests > 0) {
            runState += std::format(" Progress: {}/{} positions remaining.",
                epdData.remainingTests, epdData.totalTests);
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
    };

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
                outcome.applied.push_back("concurrency=" + std::to_string(*settings.concurrency));
            }
        }
    }
} // namespace

ActionResult selectEpdEngines(const std::vector<std::string>& engineNames) {
    if (engineNames.empty()) {
        return failed("No engine names were given.");
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

    std::vector<std::string> selectedNames;
    for (const auto& engine : outcome.resolved) {
        selectedNames.push_back(engine.getName());
    }

    std::string message = "Selected for EPD analysis: " + joinList(selectedNames) + ".";
    if (!outcome.notFound.empty()) {
        message += " Not installed (skipped): " + joinList(outcome.notFound) + ".";
    }
    switchToEpdView();
    return succeeded(message);
}

ActionResult configureEpd(const EpdSettings& settings) {
    auto& epdData = EpdData::instance();

    ConfigureOutcome outcome;
    applySettings(epdData, settings, outcome);

    // config() is a raw reference -- mutating it directly doesn't persist on its own. Unlike the
    // tournament (split across three sub-components' updateConfiguration()), EpdData's own
    // updateConfiguration() alone already covers everything set above in one call.
    epdData.updateConfiguration();

    if (!outcome.applied.empty()) {
        switchToEpdView();
    }

    std::string message;
    if (!outcome.applied.empty()) {
        message = "Configured: " + joinList(outcome.applied) + ".";
    }
    if (!outcome.problems.empty()) {
        if (!message.empty()) {
            message += " ";
        }
        message += "Problems: " + joinList(outcome.problems) + ".";
    }
    if (message.empty()) {
        message = "No configuration changes were provided.";
    }
    return ActionResult{.ok = outcome.problems.empty(), .text = message, .widget = nullptr,
        .endsTurn = outcome.dialogShown, .remedy = outcome.remedy};
}

ActionResult startEpd() {
    auto& epdData = EpdData::instance();
    if (epdData.isRunning() || epdData.isStarting()) {
        return failed("An EPD analysis is already running. Stop it first if you want to start a "
                      "different one.");
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
    if (!epdData.isRunning() && !epdData.isStarting()) {
        return failed("No EPD analysis is currently running.");
    }

    bool graceful = mode == StopMode::Graceful;
    epdData.stopPool(graceful);
    switchToEpdView();
    return succeeded(graceful
            ? "Stopping the EPD analysis gracefully: positions already being analyzed will "
              "finish, no new ones will start."
            : "Stopping the EPD analysis abruptly: all in-progress positions are being aborted "
              "immediately.");
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
        .text = "Showing the current EPD analysis results as a table in the chat -- it is "
                "already visible to the user, so do not restate, list, or summarize the numbers "
                "in your reply; just briefly confirm what you did. This is the ONLY way you ever "
                "learn which positions were solved or not -- you have no other source for that. "
                "Never state, type, or guess a result yourself instead of calling this; that "
                "would be fabricated information, not a real result.",
        .widget = []() {
            auto& data = EpdData::instance();
            ImGui::Text("EPD Analysis Progress: %zu / %zu positions remaining",
                data.remainingTests, data.totalTests);
            ImGui::Spacing();
            data.drawTable(ImVec2(0.0F, 3000.0F));
        }};
}

std::string epdActivityText() {
    auto& epdData = EpdData::instance();
    using State = EpdData::State;
    switch (epdData.state) {
        case State::Starting: return "an EPD analysis is starting";
        case State::Running: return "an EPD analysis is running";
        case State::Gracefully:
            return "an EPD analysis is running but stopping gracefully (finishing in-progress "
                   "positions)";
        case State::Stopping: return "an EPD analysis is stopping abruptly";
        case State::Stopped:
        case State::Cleared:
        default: return "";
    }
}

} // namespace QaplaLlm::Actions
