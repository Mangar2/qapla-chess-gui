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

    /**
     * @brief Arguments shared by start/stop/get_status/clear_result.
     *
     * `type` is required on all but get_status; `mode` is only declared on stop. Both are
     * optionals purely because that is how the mapper reads a parameter -- a required one is
     * guaranteed present by the time invoke() runs.
     */
    struct ActivityRequest {
        std::optional<Activity> type;
        std::optional<StopMode> mode;
    };

    Api::Param<ActivityRequest> typeParam(bool required, std::string extraSentence = {}) {
        std::string description =
            "Which of the three independent activities to act on. \"tournament\": classic "
            "multi-engine round robin. \"sprt\": champion-vs-challenger SPRT test. \"epd\": "
            "move-finding analysis vs a fixed position set. If unclear which the user means, "
            "ask, don't guess -- wrong guess silently acts on the wrong one.";
        if (!extraSentence.empty()) {
            description += " " + extraSentence;
        }
        return Api::enumParam<ActivityRequest>("type", &ActivityRequest::type,
            std::move(description),
            {{"tournament", Activity::Tournament}, {"sprt", Activity::Sprt},
                {"epd", Activity::Epd}},
            required);
    }

    Api::Param<ActivityRequest> stopModeParam() {
        return Api::enumParam<ActivityRequest>("mode", &ActivityRequest::mode,
            "\"graceful\" (default): finish whatever's already in progress, then stop, nothing "
            "new starts. \"abrupt\": abort in-progress work immediately. If user just says "
            "\"stop\" unqualified, use \"graceful\" -- ask only if they've previously shown "
            "they care about the distinction.",
            {{"graceful", StopMode::Graceful}, {"abrupt", StopMode::Abrupt}});
    }

    // Every tool that changes a run's state ends with the cross-activity summary, so a caller
    // never needs a second round-trip to learn the state its own call produced -- and so a change
    // the user made by hand in the meantime shows up rather than being silently assumed away.
    // That is an external packaging choice about round-trip cost, not something the actions know.
    Actions::ActionResult withRunningSummary(Actions::ActionResult result) {
        result.text += " " + Actions::runningActivitiesText();
        return result;
    }
} // namespace

void registerActivityTools(GuiToolRegistry& registry) {
    Api::defineTool<ActivityRequest>(registry,
        {.name = "start",
            .description =
                "Starts a tournament, SPRT test, or EPD analysis (pick via \"type\") "
                "using whatever engines/settings were configured via "
                "configure_tournament, configure_sprt or configure_epd respectively. "
                "Requires that type's own preconditions already met (engines selected, "
                "openings/EPD file configured) -- and they usually already are, carried "
                "over from an earlier session, so \"start it\" normally needs no setup at "
                "all, just this call. Never configure anything, list engines or ask the "
                "user for a value first unless the user asked for a change: if something "
                "really is missing, this result names exactly what. EPD-specific: "
                "auto-resumes from a previous incomplete "
                "run instead of restarting if its engines/file/timing are unchanged "
                "since; if that previous run already completed, or its settings changed "
                "after it stopped, starting fails until clear_result (type=\"epd\") is "
                "called first. Response always reports what's running across all three "
                "afterward, so no separate status call is needed to confirm it.",
            .params = {typeParam(true)},
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
                "cleared) -- starting again resumes from here. An abrupt stop only "
                "returns once the run is really gone, so you may act immediately "
                "afterward; a graceful one returns while its games are still finishing "
                "and says so. Response always reports what's running across all three "
                "afterward, so no separate status call is needed to confirm it.",
            .params = {typeParam(true), stopModeParam()},
            .invoke = [](const ActivityRequest& request) {
                return withRunningSummary(
                    Actions::stopActivity(*request.type, request.mode.value_or(StopMode::Graceful)));
            }});

    // The one reporting tool. It used to be three (get_status / get_running_status /
    // show_result), whose descriptions consisted largely of telling each other apart -- and a
    // wrong pick there is a real failure, while getting more than was needed is not. So: omit
    // "type" for the overview, pass it for everything known about one activity, results included.
    Api::defineTool<ActivityRequest>(registry,
        {.name = "get_status",
            .description =
                "Reports what is going on, and changes nothing. Without \"type\": what is "
                "running across all three activities, plus which idle ones could be "
                "started exactly as they stand -- use this for any \"is something/"
                "anything/a tournament running\" question, since people call an SPRT test "
                "or EPD analysis a \"tournament\" informally and they all look alike from "
                "outside. With "
                "\"type\": everything about that one activity -- engines, every setting, "
                "whether and how it is running, what may be changed right now, and "
                "whether it is ready to start as it is -- plus its "
                "current results rendered as a real table in the chat (standings for "
                "tournament, decision + duel score for sprt, per-position solved/not for "
                "epd), partial while running, or a note that none exist yet. That table is "
                "the ONLY place any score, standing, Elo or decision ever comes from: "
                "never state, type or guess one yourself, and once it is shown don't "
                "restate its numbers -- the user already sees them. configure_* and "
                "start/stop already report this same state after every change, so call "
                "this only when nothing was changed.",
            .params = {typeParam(false,
                "Omit it entirely for the cross-activity overview; that is the right call when "
                "the user asks broadly whether anything is running.")},
            .invoke = [](const ActivityRequest& request) {
                if (!request.type) {
                    return Actions::succeeded(Actions::runningActivitiesText());
                }
                // Status and results in one answer: the split between "what is it set to" and
                // "what has it produced" was ours, never the caller's, and it cost a round-trip
                // on every question that spanned both.
                auto status = Actions::activityStatus(*request.type);
                auto results = Actions::showActivityResult(*request.type);
                status.text += " " + results.text;
                status.widget = results.widget;
                return status;
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
                "it stopped. Response always reports what's running across all three "
                "afterward.",
            .params = {typeParam(true)},
            .invoke = [](const ActivityRequest& request) {
                return withRunningSummary(Actions::clearActivityResult(*request.type));
            }});
}

} // namespace QaplaLlm
