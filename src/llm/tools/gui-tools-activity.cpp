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

#include "gui-tools.h"
#include "gui-tools-shared.h"
#include "../actions/gui-action-activity.h"

namespace QaplaLlm {

namespace {
    using Actions::Activity;
    using Actions::StopMode;

    /** @brief Arguments of a tool that takes none. */
    struct NoArguments {};

    /**
     * @brief Arguments shared by start/stop/get_status/clear_result/show_result.
     *
     * `type` is required on all five; `mode` is only declared on stop. Both are optionals purely
     * because that is how the mapper reads a parameter -- a required one is guaranteed present by
     * the time invoke() runs.
     */
    struct ActivityRequest {
        std::optional<Activity> type;
        std::optional<StopMode> mode;
    };

    Api::Param<ActivityRequest> typeParam() {
        return Api::enumParam<ActivityRequest>("type", &ActivityRequest::type,
            "Which of the three independent activities to act on. \"tournament\": classic "
            "multi-engine round robin. \"sprt\": champion-vs-challenger SPRT test. \"epd\": "
            "move-finding analysis vs a fixed position set. If unclear which the user means, "
            "ask, don't guess -- wrong guess silently starts/stops the wrong one.",
            {{"tournament", Activity::Tournament}, {"sprt", Activity::Sprt},
                {"epd", Activity::Epd}},
            true);
    }

    Api::Param<ActivityRequest> stopModeParam() {
        return Api::enumParam<ActivityRequest>("mode", &ActivityRequest::mode,
            "\"graceful\" (default): finish whatever's already in progress, then stop, nothing "
            "new starts. \"abrupt\": abort in-progress work immediately. If user just says "
            "\"stop\" unqualified, use \"graceful\" -- ask only if they've previously shown "
            "they care about the distinction.",
            {{"graceful", StopMode::Graceful}, {"abrupt", StopMode::Abrupt}});
    }

    // start and stop always end with the cross-activity summary, so the model never needs a
    // separate get_running_status round-trip just to confirm what it started or stopped. That is
    // an external packaging choice about round-trip cost, not something the actions should know.
    Actions::ActionResult withRunningSummary(Actions::ActionResult result) {
        result.text += " " + Actions::runningActivitiesText();
        return result;
    }
} // namespace

void registerActivityTools(GuiToolRegistry& registry) {
    Api::defineTool<NoArguments>(registry,
        {.name = "get_running_status",
            .description =
                "Reports what's currently running: classic tournament, SPRT test, EPD "
                "analysis checked/reported separately (run independently, see "
                "configure_sprt's/configure_epd's notes). Call whenever user asks "
                "broadly if \"something\"/\"a test\"/\"anything\" running, or "
                "specifically if TOURNAMENT running -- people often call SPRT test or "
                "EPD analysis \"tournament\" informally (all look same: engines running "
                "in background), so checking only get_status (type=\"tournament\") could "
                "wrongly say nothing happening while SPRT test or EPD analysis actually "
                "active. start/stop already return this same summary, so this is only needed "
                "for a pure check that changes nothing. Distinguishes a graceful stop "
                "still in progress from plain running/not running.",
            .invoke = [](const NoArguments&) {
                return Actions::succeeded(Actions::runningActivitiesText());
            }});

    Api::defineTool<ActivityRequest>(registry,
        {.name = "start",
            .description =
                "Starts a tournament, SPRT test, or EPD analysis (pick via \"type\") "
                "using whatever engines/settings were configured via select_engines/"
                "configure_tournament, select_sprt_engines/configure_sprt, or "
                "select_epd_engines/configure_epd respectively. Requires that type's "
                "own preconditions already met (engines selected, openings/EPD file "
                "configured) -- result states exactly which is missing if it can't "
                "start. EPD-specific: auto-resumes from a previous incomplete run "
                "instead of restarting if its engines/file/timing are unchanged since; "
                "if that previous run already completed, or its settings changed after "
                "it stopped, starting fails until clear_result (type=\"epd\") is called "
                "first. Response always reports what's running across all three "
                "afterward, so no separate get_running_status call is normally needed.",
            .params = {typeParam()},
            .invoke = [](const ActivityRequest& request) {
                return withRunningSummary(Actions::startActivity(*request.type));
            },
            // Engine processes need to launch and initialize; a handful of engines can
            // legitimately take longer than the default 30s.
            .timeout = std::chrono::seconds(60)});

    Api::defineTool<ActivityRequest>(registry,
        {.name = "stop",
            .description =
                "Stops a running tournament, SPRT test, or EPD analysis (pick via "
                "\"type\"). Optional \"mode\": graceful (default) or abrupt. Fails if "
                "that type isn't currently running. For EPD, progress is kept (not "
                "cleared) -- starting again resumes from here. Response always reports "
                "what's running across all three afterward, so no separate "
                "get_running_status call is normally needed.",
            .params = {typeParam(), stopModeParam()},
            .invoke = [](const ActivityRequest& request) {
                return withRunningSummary(
                    Actions::stopActivity(*request.type, request.mode.value_or(StopMode::Graceful)));
            }});

    Api::defineTool<ActivityRequest>(registry,
        {.name = "get_status",
            .description =
                "Reports the full current config/state of a tournament, SPRT test, or "
                "EPD analysis (pick via \"type\"): engines, all its settings, and "
                "whether/how it's running -- everything configure_tournament/"
                "configure_sprt/configure_epd also already return after any change they "
                "make, so this is only needed for a pure status check that changes "
                "nothing (e.g. \"what's the SPRT test set to right now\"), or before "
                "select_engines/select_sprt_engines/select_epd_engines. For \"is "
                "anything running\" across all three at once, use get_running_status "
                "instead.",
            .params = {typeParam()},
            .invoke = [](const ActivityRequest& request) {
                return Actions::activityStatus(*request.type);
            }});

    Api::defineTool<ActivityRequest>(registry,
        {.name = "clear_result",
            .description =
                "Discards the current results of a tournament, SPRT test, or EPD "
                "analysis (pick via \"type\"), stopping it first if still running. Use "
                "when the user wants to discard what's been played/analyzed so far, "
                "e.g. before reconfiguring and starting fresh with the same engines. "
                "For EPD specifically, this is also the required fix when start fails "
                "because a previous run already completed or its settings changed after "
                "it stopped.",
            .params = {typeParam()},
            .invoke = [](const ActivityRequest& request) {
                return Actions::clearActivityResult(*request.type);
            }});

    Api::defineTool<ActivityRequest>(registry,
        {.name = "show_result",
            .description =
                "Displays the current results of a tournament, SPRT test, or EPD "
                "analysis (pick via \"type\") as a table in the chat -- ranked-by-Elo "
                "standings for tournament, SPRT decision + duel score for sprt, "
                "per-position solved/not-solved for epd. Renders a real table control "
                "in the chat UI -- not for you to read the data and describe it in your "
                "own words; just call it and briefly confirm you're showing results, "
                "don't restate numbers. Works while running (partial results) or after "
                "finish; reports none available if nothing played/analyzed yet. ONLY way "
                "you ever learn any actual score/standing/Elo/decision -- no other "
                "source. Never state/type/guess a result yourself instead of calling "
                "this -- that's fabrication, not a real result.",
            .params = {typeParam()},
            .invoke = [](const ActivityRequest& request) {
                return Actions::showActivityResult(*request.type);
            }});
}

} // namespace QaplaLlm
