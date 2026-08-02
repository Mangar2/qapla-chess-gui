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

#include "gui-tool-status.h"
#include "gui-tool-tournament.h"
#include "gui-tool-sprt.h"
#include "gui-tool-epd.h"
#include "../tournament-data.h"
#include "../sprt-tournament-data.h"
#include "../epd-data.h"

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;

    // Empty string means "not active" -- callers filter those out rather than special-casing
    // them here, so this stays a plain state->text mapping. Distinguishing GracefulStopping (and,
    // for tournament/EPD, the separate abrupt Stopping state) from plain Running matters: a
    // gracefully-stopping run is still actively finishing whatever's in progress, just declining
    // to start anything new, which "running" alone would hide.
    std::string describeTournamentActivity(QaplaWindows::TournamentData& data) {
        using State = QaplaWindows::TournamentData::State;
        switch (data.getState()) {
            case State::Starting: return "a tournament is starting";
            case State::Running: return "a tournament is running";
            case State::GracefulStopping: return "a tournament is running but stopping gracefully "
                                                  "(finishing in-progress games)";
            case State::Stopping: return "a tournament is stopping abruptly";
            case State::Stopped:
            default: return "";
        }
    }

    std::string describeSprtActivity(QaplaWindows::SprtTournamentData& data) {
        using State = QaplaWindows::SprtTournamentData::State;
        switch (data.state()) {
            case State::Starting: return "an SPRT test is starting";
            case State::Running: return "an SPRT test is running";
            case State::GracefulStopping: return "an SPRT test is running but stopping "
                                                  "gracefully (finishing its in-progress game)";
            case State::Stopped:
            default: return "";
        }
    }

    std::string describeEpdActivity(QaplaWindows::EpdData& data) {
        using State = QaplaWindows::EpdData::State;
        switch (data.state) {
            case State::Starting: return "an EPD analysis is starting";
            case State::Running: return "an EPD analysis is running";
            case State::Gracefully: return "an EPD analysis is running but stopping gracefully "
                                            "(finishing in-progress positions)";
            case State::Stopping: return "an EPD analysis is stopping abruptly";
            case State::Stopped:
            case State::Cleared:
            default: return "";
        }
    }

    // Shared by get_running_status and the unified start/stop tool (the latter always ends
    // with this too -- see handleStartOrStop -- so the model never needs a separate
    // get_running_status round-trip just to confirm what it started/stopped).
    std::string buildRunningStatusText() {
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        auto& sprtData = QaplaWindows::SprtTournamentData::instance();
        auto& epdData = QaplaWindows::EpdData::instance();

        std::vector<std::string> active;
        for (auto& text : {describeTournamentActivity(tournamentData), describeSprtActivity(sprtData),
                 describeEpdActivity(epdData)}) {
            if (!text.empty()) {
                active.push_back(text);
            }
        }

        if (active.empty()) {
            return "Nothing is currently running -- no tournament, no SPRT test, no EPD analysis.";
        }
        std::string joined;
        if (active.size() == 1) {
            joined = active.front();
        } else if (active.size() == 2) {
            joined = active[0] + "; " + active[1];
        } else {
            joined = active[0] + "; " + active[1] + "; " + active[2];
        }
        return "Currently: " + joined + ".";
    }

    GuiToolResult handleGetRunningStatus(const Json::JsonValue&) {
        return GuiToolResult{.success = true, .content = buildRunningStatusText()};
    }

    Json::JsonValue typeSchemaProperty() {
        auto prop = Json::JsonValue::object();
        prop["type"] = "string";
        auto enumValues = Json::JsonValue::array();
        enumValues.push_back("tournament");
        enumValues.push_back("sprt");
        enumValues.push_back("epd");
        prop["enum"] = enumValues;
        prop["description"] =
            "Which of the three independent activities to act on. \"tournament\": classic "
            "multi-engine round robin. \"sprt\": champion-vs-challenger SPRT test. \"epd\": "
            "move-finding analysis vs a fixed position set. If unclear which the user means, "
            "ask, don't guess -- wrong guess silently starts/stops the wrong one.";
        return prop;
    }

    Json::JsonValue buildStartSchema() {
        auto schema = noArgsToolSchema();
        schema["properties"]["type"] = typeSchemaProperty();
        schema["required"] = Json::JsonValue::array();
        schema["required"].push_back("type");
        return schema;
    }

    Json::JsonValue buildStopSchema() {
        auto schema = buildStartSchema();
        auto mode = Json::JsonValue::object();
        mode["type"] = "string";
        auto enumValues = Json::JsonValue::array();
        enumValues.push_back("graceful");
        enumValues.push_back("abrupt");
        mode["enum"] = enumValues;
        mode["description"] =
            "\"graceful\" (default): finish whatever's already in progress, then stop, nothing "
            "new starts. \"abrupt\": abort in-progress work immediately. If user just says "
            "\"stop\" unqualified, use \"graceful\" -- ask only if they've previously shown "
            "they care about the distinction.";
        schema["properties"]["mode"] = mode;
        return schema;
    }

    GuiToolResult handleStart(const Json::JsonValue& arguments) {
        if (!arguments.contains("type") || !arguments.at("type").is_string()) {
            return GuiToolResult{.success = false, .content = "\"type\" is required: \"tournament\", \"sprt\", or \"epd\"."};
        }
        const auto& type = arguments.at("type").as_string();

        GuiToolResult result;
        if (type == "tournament") {
            result = handleStartTournament(arguments);
        } else if (type == "sprt") {
            result = handleStartSprtTournament(arguments);
        } else if (type == "epd") {
            result = handleStartEpdAnalysis(arguments);
        } else {
            return GuiToolResult{.success = false, .content = "Unknown type \"" + type + "\" -- must be \"tournament\", \"sprt\", or \"epd\"."};
        }

        result.content += " " + buildRunningStatusText();
        return result;
    }

    GuiToolResult handleStop(const Json::JsonValue& arguments) {
        if (!arguments.contains("type") || !arguments.at("type").is_string()) {
            return GuiToolResult{.success = false, .content = "\"type\" is required: \"tournament\", \"sprt\", or \"epd\"."};
        }
        const auto& type = arguments.at("type").as_string();

        GuiToolResult result;
        if (type == "tournament") {
            result = handleStopTournament(arguments);
        } else if (type == "sprt") {
            result = handleStopSprtTournament(arguments);
        } else if (type == "epd") {
            result = handleStopEpdAnalysis(arguments);
        } else {
            return GuiToolResult{.success = false, .content = "Unknown type \"" + type + "\" -- must be \"tournament\", \"sprt\", or \"epd\"."};
        }

        result.content += " " + buildRunningStatusText();
        return result;
    }
}

void registerStatusTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "get_running_status",
        .description = "Reports what's currently running: classic tournament, SPRT test, EPD "
                        "analysis checked/reported separately (run independently, see "
                        "configure_sprt's/configure_epd's notes). Call whenever user asks "
                        "broadly if \"something\"/\"a test\"/\"anything\" running, or "
                        "specifically if TOURNAMENT running -- people often call SPRT test or "
                        "EPD analysis \"tournament\" informally (all look same: engines running "
                        "in background), so checking only get_tournament_status could wrongly "
                        "say nothing happening while SPRT test or EPD analysis actually active. "
                        "start/stop already return this same summary, so this is only needed "
                        "for a pure check that changes nothing. Distinguishes a graceful stop "
                        "still in progress from plain running/not running.",
        .handler = handleGetRunningStatus
    });

    registry.registerTool(GuiToolDefinition{
        .name = "start",
        .description = "Starts a tournament, SPRT test, or EPD analysis (pick via \"type\") "
                        "using whatever engines/settings were configured via select_engines/"
                        "configure_tournament, select_sprt_engines/configure_sprt, or "
                        "select_epd_engines/configure_epd respectively. Requires that type's "
                        "own preconditions already met (engines selected, openings/EPD file "
                        "configured) -- result states exactly which is missing if it can't "
                        "start. EPD-specific: auto-resumes from a previous incomplete run "
                        "instead of restarting if its engines/file/timing are unchanged since; "
                        "if that previous run already completed, or its settings changed after "
                        "it stopped, starting fails until clear_epd_result is called first. "
                        "Response always reports what's running across all three afterward, so "
                        "no separate get_running_status call is normally needed.",
        .parametersSchema = buildStartSchema(),
        .handler = handleStart,
        // Engine processes need to launch and initialize; a handful of engines can legitimately
        // take longer than the default 30s.
        .timeout = std::chrono::seconds(60)
    });

    registry.registerTool(GuiToolDefinition{
        .name = "stop",
        .description = "Stops a running tournament, SPRT test, or EPD analysis (pick via "
                        "\"type\"). Optional \"mode\": graceful (default) or abrupt. Fails if "
                        "that type isn't currently running. For EPD, progress is kept (not "
                        "cleared) -- starting again resumes from here. Response always reports "
                        "what's running across all three afterward, so no separate "
                        "get_running_status call is normally needed.",
        .parametersSchema = buildStopSchema(),
        .handler = handleStop
    });
}

} // namespace QaplaLlm
