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

#include "gui-tool-epd.h"
#include "gui-tool-tournament.h" // resolveEngines() -- pure logic, reused as-is
#include "../epd-data.h"
#include "../snackbar.h"
#include "../os-dialogs.h"
#include "../callback-manager.h"

#include <imgui.h>

#include <chrono>
#include <filesystem>
#include <format>

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;
    using QaplaWindows::EpdData;

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

    // Same message the classic (non-AI) EPD chatbot flow's own "Switch to EPD View" button
    // sends (see chatbot-step-epd-start.cpp) -- ImGuiTabBar subscribes and flips to the Epd
    // tab on the next frame. Called whenever a tool actually changes EPD settings or its run
    // state.
    void switchToEpdView() {
        QaplaWindows::StaticCallbacks::message().invokeAll("switch_to_epd_view");
    }

    // Looks for the most recent "epd"-topic snackbar since countBefore entries existed --
    // the specific reason EpdData::analyse() showed via SnackbarManager for why it silently
    // declined to start (reports failures only that way, same as TournamentData/
    // SprtTournamentData -- see mayAnalyze() in epd-data.cpp).
    std::string findRecentEpdSnackbar(std::size_t countBefore) {
        const auto& history = QaplaWindows::SnackbarManager::instance().getHistory();
        for (auto i = history.size(); i > countBefore; --i) {
            if (history[i - 1].topic == "epd") {
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
    // select_epd_engines
    // ------------------------------------------------------------------

    Json::JsonValue buildSelectEpdEnginesSchema() {
        auto schema = noArgsToolSchema();
        auto engines = Json::JsonValue::object();
        engines["type"] = "array";
        auto items = Json::JsonValue::object();
        items["type"] = "string";
        engines["items"] = items;
        engines["description"] =
            "Engine display names to test vs EPD positions, e.g. [\"Stockfish\", "
            "\"Qapla\"]. All tested vs SAME position set, side by side (one result column "
            "each) -- unlike SPRT, no champion/challenger role; any number of engines "
            "(incl. one) fine.";
        schema["properties"]["engines"] = engines;
        schema["required"] = Json::JsonValue::array();
        schema["required"].push_back("engines");
        return schema;
    }

    GuiToolResult handleSelectEpdEngines(const Json::JsonValue& arguments) {
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

        // setEngineConfigurations() self-persists (see ImGuiEngineSelect::notifyConfigurationChanged())
        // and its registered callback keeps EpdData's own engine list in sync -- no separate
        // persistence call needed here, same as tournament's/SPRT's select_*_engines.
        EpdData::instance().getEngineSelect().setEngineConfigurations(outcome.resolved);

        std::vector<std::string> selectedNames;
        for (const auto& engine : outcome.resolved) {
            selectedNames.push_back(engine.getName());
        }

        std::string message = "Selected for EPD analysis: " + joinStrings(selectedNames) + ".";
        if (!outcome.notFound.empty()) {
            message += " Not installed (skipped): " + joinStrings(outcome.notFound) + ".";
        }
        switchToEpdView();
        return GuiToolResult{.success = true, .content = message};
    }

    // ------------------------------------------------------------------
    // configure_epd
    // ------------------------------------------------------------------

    Json::JsonValue buildConfigureEpdSchema() {
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

        properties["epd_file"] = stringProp("Path to existing EPD (or RAW position) file on disk.");
        properties["max_time_seconds"] = integerProp(
            "Max seconds engine may search each position. NOT tournament/SPRT time control -- "
            "EPD analysis has no clock string, just plain per-position time budget. Default 10.");
        properties["min_time_seconds"] = integerProp(
            "Min seconds engine must keep searching each position even after finding apparent "
            "right move (guards vs solving by luck on shallow search). Default 1.");
        properties["seen_plies"] = integerProp(
            "Consecutive plies engine's PV must keep showing correct best move before position "
            "counted solved, analysis moves on early -- saves time on easy positions. Default 3.");
        properties["concurrency"] = integerProp("Positions to analyze in parallel.");
        return schema;
    }

    void applyEpdFile(EpdData& data, const std::string& path, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (!std::filesystem::exists(path)) {
            problems.push_back("EPD file not found: " + path +
                " -- call open_epd_file_dialog to let the user pick a valid one");
            return;
        }
        data.config().filepath = path;
        applied.push_back("EPD file");
    }

    GuiToolResult handleOpenEpdFileDialog(const Json::JsonValue&) {
        auto paths = QaplaWindows::OsDialogs::openFileDialog(false);
        if (paths.empty()) {
            return GuiToolResult{
                .success = true,
                .content = "The user cancelled the dialog; the EPD file was not changed."
            };
        }

        auto& epdData = EpdData::instance();
        epdData.config().filepath = paths.front();
        epdData.updateConfiguration();
        switchToEpdView();
        return GuiToolResult{.success = true, .content = "EPD file set to: " + paths.front()};
    }

    void applyMaxTime(EpdData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value < 1) {
            problems.push_back("max_time_seconds must be at least 1");
            return;
        }
        data.config().maxTimeInS = static_cast<uint64_t>(value);
        applied.push_back("max time per position=" + std::to_string(data.config().maxTimeInS) + "s");
    }

    void applyMinTime(EpdData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value < 0) {
            problems.push_back("min_time_seconds must be at least 0");
            return;
        }
        data.config().minTimeInS = static_cast<uint64_t>(value);
        applied.push_back("min time per position=" + std::to_string(data.config().minTimeInS) + "s");
    }

    void applySeenPlies(EpdData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value < 0) {
            problems.push_back("seen_plies must be at least 0");
            return;
        }
        data.config().seenPlies = static_cast<uint32_t>(value);
        applied.push_back("seen plies=" + std::to_string(data.config().seenPlies));
    }

    void applyConcurrency(EpdData& data, double value, std::vector<std::string>& applied, std::vector<std::string>& problems) {
        if (value < 1) {
            problems.push_back("concurrency must be at least 1");
            return;
        }
        auto concurrency = static_cast<uint32_t>(value);
        data.setExternalConcurrency(concurrency);
        applied.push_back("concurrency=" + std::to_string(concurrency));
    }

    GuiToolResult handleConfigureEpd(const Json::JsonValue& arguments) {
        auto& epdData = EpdData::instance();
        std::vector<std::string> applied;
        std::vector<std::string> problems;

        if (arguments.contains("epd_file") && arguments.at("epd_file").is_string()) {
            applyEpdFile(epdData, arguments.at("epd_file").as_string(), applied, problems);
        }
        if (arguments.contains("max_time_seconds") && arguments.at("max_time_seconds").is_number()) {
            applyMaxTime(epdData, arguments.at("max_time_seconds").as_number(), applied, problems);
        }
        if (arguments.contains("min_time_seconds") && arguments.at("min_time_seconds").is_number()) {
            applyMinTime(epdData, arguments.at("min_time_seconds").as_number(), applied, problems);
        }
        if (arguments.contains("seen_plies") && arguments.at("seen_plies").is_number()) {
            applySeenPlies(epdData, arguments.at("seen_plies").as_number(), applied, problems);
        }
        if (arguments.contains("concurrency") && arguments.at("concurrency").is_number()) {
            applyConcurrency(epdData, arguments.at("concurrency").as_number(), applied, problems);
        }

        // config() is a raw reference -- mutating it directly doesn't persist on its own.
        // Unlike tournament (split across three sub-components' updateConfiguration()),
        // EpdData::updateConfiguration() alone already covers everything set above in one call.
        epdData.updateConfiguration();

        if (!applied.empty()) {
            switchToEpdView();
        }
        return buildConfigureResult(applied, problems);
    }

    // ------------------------------------------------------------------
    // get_epd_status
    // ------------------------------------------------------------------

    GuiToolResult handleGetEpdStatus(const Json::JsonValue&) {
        auto& epdData = EpdData::instance();
        auto selectedEngines = epdData.getEngineSelect().getSelectedEngines();
        std::vector<std::string> engineNames;
        for (const auto& engine : selectedEngines) {
            engineNames.push_back(engine.getName());
        }
        const auto& config = epdData.config();

        std::string runState = "No EPD analysis is currently running.";
        if (epdData.isStarting()) {
            runState = "An EPD analysis is currently starting.";
        } else if (epdData.isRunning()) {
            runState = "An EPD analysis is currently running.";
        } else if (epdData.isStopping()) {
            runState = "An EPD analysis is currently stopping.";
        }
        if (epdData.isFinished()) {
            runState += " All positions have been analyzed -- call show_epd_result to see it.";
        } else if (epdData.totalTests > 0) {
            runState += std::format(" Progress: {}/{} positions remaining.",
                epdData.remainingTests, epdData.totalTests);
        }

        std::string message = std::format(
            "Engines: {}. EPD file: {}. Max time per position: {}s. Min time per position: {}s. "
            "Seen plies (early stop): {}. Concurrency: {}. {}",
            engineNames.empty() ? "none selected" : joinStrings(engineNames),
            config.filepath.empty() ? "(not set)" : config.filepath,
            config.maxTimeInS, config.minTimeInS, config.seenPlies,
            epdData.getExternalConcurrency(),
            runState);

        return GuiToolResult{.success = true, .content = message};
    }

    // ------------------------------------------------------------------
    // start_epd_analysis
    // ------------------------------------------------------------------

    GuiToolResult handleStartEpdAnalysis(const Json::JsonValue&) {
        auto& epdData = EpdData::instance();
        if (epdData.isRunning() || epdData.isStarting()) {
            return GuiToolResult{
                .success = false,
                .content = "An EPD analysis is already running. Stop it first if you want to start a different one."
            };
        }

        auto historyCountBefore = QaplaWindows::SnackbarManager::instance().getHistory().size();

        // A single call covers both a fresh start and resuming an incomplete analysis --
        // EpdData::analyse() decides internally which one this is (see its doc comment / the
        // gui-tool-epd.h header comment): if the configured engines/EPD file/timing haven't
        // changed since the last run, it picks up where it left off instead of starting over.
        epdData.analyse();

        if (!epdData.isRunning() && !epdData.isStarting()) {
            auto reason = findRecentEpdSnackbar(historyCountBefore);
            if (reason.empty()) {
                reason = "Could not start the EPD analysis.";
            } else if (reason.find("Clear data before re-analyzing") != std::string::npos) {
                // This exact rejection (see EpdData::mayAnalyze()) has no tournament/SPRT
                // equivalent, so spell out the fix rather than relying on the model to
                // connect "clear data" to the clear_epd_result tool on its own.
                reason += " Call clear_epd_result, then call start_epd_analysis again.";
            }
            return GuiToolResult{.success = false, .content = reason};
        }

        switchToEpdView();
        return GuiToolResult{.success = true, .content = "EPD analysis started."};
    }

    // ------------------------------------------------------------------
    // stop_epd_analysis
    // ------------------------------------------------------------------

    Json::JsonValue buildStopEpdAnalysisSchema() {
        auto schema = noArgsToolSchema();
        auto mode = Json::JsonValue::object();
        mode["type"] = "string";
        auto enumValues = Json::JsonValue::array();
        enumValues.push_back("graceful");
        enumValues.push_back("abrupt");
        mode["enum"] = enumValues;
        mode["description"] =
            "\"graceful\" (default if omitted): let in-progress positions finish, then stop -- "
            "no new ones started. \"abrupt\": abort all in-progress positions immediately. User "
            "says \"stop\"/\"end analysis\" unqualified -> use \"graceful\".";
        schema["properties"]["mode"] = mode;
        return schema;
    }

    GuiToolResult handleStopEpdAnalysis(const Json::JsonValue& arguments) {
        auto& epdData = EpdData::instance();
        if (!epdData.isRunning() && !epdData.isStarting()) {
            return GuiToolResult{.success = false, .content = "No EPD analysis is currently running."};
        }

        bool graceful = true;
        if (arguments.contains("mode") && arguments.at("mode").is_string()) {
            graceful = arguments.at("mode").as_string() != "abrupt";
        }

        epdData.stopPool(graceful);
        switchToEpdView();
        return GuiToolResult{
            .success = true,
            .content = graceful
                ? "Stopping the EPD analysis gracefully: positions already being analyzed will "
                  "finish, no new ones will start."
                : "Stopping the EPD analysis abruptly: all in-progress positions are being "
                  "aborted immediately."
        };
    }

    // ------------------------------------------------------------------
    // clear_epd_result
    // ------------------------------------------------------------------

    GuiToolResult handleClearEpdResult(const Json::JsonValue&) {
        auto& epdData = EpdData::instance();
        if (epdData.totalTests == 0) {
            return GuiToolResult{.success = true, .content = "There are no EPD analysis results to clear."};
        }

        bool wasRunning = epdData.isRunning() || epdData.isStarting();
        epdData.clear();
        switchToEpdView();
        return GuiToolResult{
            .success = true,
            .content = wasRunning
                ? "EPD analysis stopped and all results cleared."
                : "All EPD analysis results have been cleared."
        };
    }

    // ------------------------------------------------------------------
    // show_epd_result
    // ------------------------------------------------------------------

    GuiToolResult handleShowEpdResult(const Json::JsonValue&) {
        auto& epdData = EpdData::instance();
        if (epdData.totalTests == 0) {
            return GuiToolResult{.success = true, .content = "No EPD analysis results are available yet."};
        }

        // Renders the same live control the classic (non-AI) EPD view draws (see
        // EpdData::drawTable(), used from EpdWindow::draw()) -- a real ImGuiTable with one row
        // per position and one column per engine, not a text dump. Always reads
        // EpdData::instance() fresh at draw time (every frame the ChatEntry stays visible), so
        // it reflects the analysis's current state, exactly like that classic view does.
        return GuiToolResult{
            .success = true,
            .content = "Showing the current EPD analysis results as a table in the chat -- it is "
                        "already visible to the user, so do not restate, list, or summarize the "
                        "numbers in your reply; just briefly confirm what you did. This is the "
                        "ONLY way you ever learn which positions were solved or not -- you have "
                        "no other source for that. Never state, type, or guess a result yourself "
                        "instead of calling this; that would be fabricated information, not a "
                        "real result.",
            .renderWidget = []() {
                auto& data = EpdData::instance();
                ImGui::Text("EPD Analysis Progress: %zu / %zu positions remaining",
                    data.remainingTests, data.totalTests);
                ImGui::Spacing();
                data.drawTable(ImVec2(0.0F, 3000.0F));
            }
        };
    }
}

void registerEpdTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "select_epd_engines",
        .description = "Selects configured chess engines tested vs EPD positions, replacing any "
                        "previous selection. Names matched case-insensitively vs installed "
                        "engine catalog; informal/shortened name (e.g. \"spike\") auto-matched "
                        "to the one installed engine it can only mean (e.g. \"Spike 1.4.1\") -- "
                        "pass name user actually said, no need to call list_installed_engines "
                        "first just to look up full name. If name could mean more than one "
                        "installed engine, result lists candidates -- ask user which one meant, "
                        "never guess. Entirely separate from select_engines/select_sprt_engines "
                        "(classic tournament/SPRT) -- selecting EPD engines never changes those, "
                        "vice versa.",
        .parametersSchema = buildSelectEpdEnginesSchema(),
        .handler = handleSelectEpdEngines
    });

    registry.registerTool(GuiToolDefinition{
        .name = "configure_epd",
        .description = "Sets EPD analysis options: epd_file, max_time_seconds, "
                        "min_time_seconds, seen_plies, concurrency. Each field independent, "
                        "optional -- pass ONLY what user asked to change, don't require/ask for "
                        "others first. Unset fields keep prior value (this session or earlier) "
                        "-- call get_epd_status first if unsure what's current. IMPORTANT: "
                        "completely separate from configure_tournament/configure_sprt -- EPD "
                        "has no shared time_control string (just plain per-position second "
                        "counts), no adjudication concept, despite sounding like another "
                        "engine-testing mode. If request could mean tournament, SPRT, or EPD "
                        "and unclear which, ask, don't guess. epd_file must be set (here or "
                        "earlier session) before start_epd_analysis succeeds. If missing, "
                        "invalid, or user wants to browse, call open_epd_file_dialog instead of "
                        "asking them to type path.",
        .parametersSchema = buildConfigureEpdSchema(),
        .handler = handleConfigureEpd
    });

    registry.registerTool(GuiToolDefinition{
        .name = "open_epd_file_dialog",
        .description = "Opens GUI's native file picker for user to choose EPD (or RAW position) "
                        "file -- you have no filesystem access, never guess or invent a path. "
                        "Use instead of asking user to type/paste path whenever missing, "
                        "reported invalid, or user wants to browse. Chosen path applied "
                        "immediately, same as configure_epd's epd_file.",
        .handler = handleOpenEpdFileDialog,
        .timeout = std::chrono::minutes(10)
    });

    registry.registerTool(GuiToolDefinition{
        .name = "get_epd_status",
        .description = "Reports current EPD analysis config/state: selected engines, EPD file, "
                        "max/min time per position, seen_plies, concurrency, progress "
                        "(positions remaining), running/finished. Entirely separate from "
                        "get_tournament_status/get_sprt_status -- use this one for EPD "
                        "questions. Call FIRST when request changes only one EPD setting and "
                        "rest of config uncertain.",
        .handler = handleGetEpdStatus
    });

    registry.registerTool(GuiToolDefinition{
        .name = "start_epd_analysis",
        .description = "Starts (or resumes incomplete) EPD analysis with engines/settings from "
                        "select_epd_engines/configure_epd. Auto-resumes from previous run's "
                        "stopping point instead of restarting, if engines/file/timing unchanged "
                        "since. Requires at least one selected engine + configured EPD file; "
                        "result states exactly which precondition missing if it can't start. "
                        "EPD-specific precondition, no tournament/SPRT equivalent: if previous "
                        "analysis already completed, or settings changed via "
                        "select_epd_engines/configure_epd after it stopped, call fails until "
                        "clear_epd_result called first -- if so, call clear_epd_result, then "
                        "retry this.",
        .handler = handleStartEpdAnalysis,
        // Engine processes need to launch and initialize; a handful of
        // engines can legitimately take longer than the default 30s.
        .timeout = std::chrono::seconds(60)
    });

    registry.registerTool(GuiToolDefinition{
        .name = "stop_epd_analysis",
        .description = "Stops currently running EPD analysis. Optional \"mode\": \"graceful\" "
                        "(default) finishes in-progress positions, starts no new ones; "
                        "\"abrupt\" aborts every in-progress position immediately. Fails if no "
                        "EPD analysis running. Progress kept (not cleared) -- "
                        "start_epd_analysis resumes from here.",
        .parametersSchema = buildStopEpdAnalysisSchema(),
        .handler = handleStopEpdAnalysis
    });

    registry.registerTool(GuiToolDefinition{
        .name = "clear_epd_result",
        .description = "Discards current EPD analysis results (stops it first if still "
                        "running). Use when user wants to throw away progress so far, e.g. "
                        "before reconfiguring and starting fresh analysis.",
        .handler = handleClearEpdResult
    });

    registry.registerTool(GuiToolDefinition{
        .name = "show_epd_result",
        .description = "Displays current EPD analysis results as table in chat: one row per "
                        "position, one column per tested engine, showing whether each engine "
                        "found correct move (and how fast) or not. Renders table control in "
                        "chat UI -- not for you to read data and describe in own words; just "
                        "call it, briefly say you're showing results, don't restate numbers "
                        "from response. Works while running (partial results) and after "
                        "finished; reports no results yet if nothing analyzed.",
        .handler = handleShowEpdResult
    });
}

} // namespace QaplaLlm
