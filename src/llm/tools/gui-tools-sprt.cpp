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
#include "../actions/gui-action-sprt.h"

namespace QaplaLlm {

namespace {
    using Actions::SprtSettings;

    /**
     * @brief Arguments of configure_sprt: the settings patch plus the two engines.
     *
     * The pair used to be a tool of its own (select_sprt_engines). It is a configuration field
     * like any other -- frozen for exactly as long, refused in the same words, and patched the
     * same way, one role at a time -- and a caller that wants "A against B at 1+0" had to know
     * it needed two calls in the right order.
     */
    struct ConfigureSprtRequest : SprtSettings {
        std::optional<std::string> champion;
        std::optional<std::string> challenger;
    };

    std::vector<Api::Param<ConfigureSprtRequest>> configureParams() {
        using Request = ConfigureSprtRequest;
        std::vector<Api::Param<Request>> params;
        params.push_back(Api::stringParam<Request>("champion", &Request::champion,
            "The comparison baseline: the trusted engine the challenger is measured against. "
            "Optional like every other field -- pass it only to change it; leaving it out keeps "
            "the champion already configured. Matched case-insensitively against the installed "
            "engine catalog, and a shortened name (e.g. \"spike\") is matched to the one engine "
            "it can mean; if it could mean more than one, nothing is changed and the result "
            "lists the candidates: ask the user, never guess."));
        params.push_back(Api::stringParam<Request>("challenger", &Request::challenger,
            "The engine under test. Optional in the same way: pass it to change it, leave it out "
            "to keep the one already configured. Must end up being a different engine than the "
            "champion."));
        params.push_back(Tools::timeControlParam<Request>());
        params.push_back(Api::numberParam<Request>("elo0", &SprtSettings::eloH0,
            "H0 (null hypothesis) Elo bound -- challenger NOT stronger at/below this Elo diff. "
            "Usually 0. Default 0."));
        params.push_back(Api::numberParam<Request>("elo1", &SprtSettings::eloH1,
            "H1 (alt hypothesis) Elo bound -- challenger stronger at/above this Elo diff. Must "
            "end up > elo0 (checked at test start, not when this field alone set). Default 3."));
        params.push_back(Api::numberParam<Request>("alpha", &SprtSettings::alpha,
            "Type I error rate: prob. of accepting challenger as stronger when it isn't. 0-1 "
            "exclusive. Default 0.05."));
        params.push_back(Api::numberParam<Request>("beta", &SprtSettings::beta,
            "Type II error rate: prob. of rejecting challenger when it is stronger. 0-1 "
            "exclusive. Default 0.05."));
        params.push_back(Api::integerParam<Request>("max_games", &SprtSettings::maxGames,
            "Max games before stopping inconclusive if no decision sooner. Default 100000."));
        params.push_back(Api::stringParam<Request>("model", &SprtSettings::model,
            "Statistical model for SPRT calc: \"normalized\" (default), \"logistic\", "
            "\"bayesian\"."));
        params.push_back(Api::boolParam<Request>("pentanomial", &SprtSettings::pentanomial,
            "True: pentanomial (paired-game) stats instead of trinomial -- more power, needs "
            "games in same-opening pairs. Default false."));
        params.push_back(Tools::openingsFileParam<Request>(
            "Path to existing EPD/PGN opening book file on disk."));
        params.push_back(Tools::openingsFileDialogParam<Request>());
        params.push_back(Tools::pgnFileParam<Request>());
        params.push_back(Tools::pgnFileDialogParam<Request>());
        params.push_back(Api::integerParam<Request>("concurrency", &SprtSettings::concurrency,
            "Games to run in parallel."));
        Tools::appendAdjudicationParams(params);
        return params;
    }
} // namespace

void registerSprtTools(GuiToolRegistry& registry) {
    Api::defineTool<ConfigureSprtRequest>(registry,
        {.name = "configure_sprt",
            .description =
                "Sets everything about the SPRT test: champion/challenger (its two "
                "engines), time_control, elo0/elo1 (H0/H1 Elo bounds), alpha, beta, "
                "max_games, model, pentanomial, openings_file, pgn_file, concurrency, "
                "draw_mode/draw_min_full_moves/draw_required_consecutive_moves/"
                "draw_centipawn_threshold, resign_mode/"
                "resign_required_consecutive_moves/resign_centipawn_threshold/"
                "resign_two_sided. Every field independent/optional -- pass ONLY what "
                "user asked to change, and pass them together in one call when they asked "
                "for several; don't require/ask for others first. Anything not passed "
                "keeps prior value (this session or earlier). Response reports the full "
                "resulting SPRT config, so no separate get_status call is needed to "
                "confirm it. While a test runs or stops, every field except concurrency "
                "is rejected and nothing changes; stop it first. concurrency applies "
                "immediately. champion and challenger are ordinary optional fields: name one to "
                "replace just that role and keep the other, or both to set the pair -- never ask "
                "the user for a role they didn't mention, it is already configured. If a named "
                "engine can't be resolved, nothing at all is applied, not even the other fields. "
                "IMPORTANT: fully separate config from configure_tournament -- classic "
                "tournament and SPRT each have own engines, time control, openings file, "
                "concurrency, adjudication etc, despite same field names. If user's "
                "message doesn't make clear tournament vs SPRT (e.g. bare \"set time "
                "control to 1 min/game\"), infer from conversation so far (multi-engine "
                "tournament or champion-vs-challenger SPRT?); if still unclear from "
                "context, ask which they mean, never guess -- wrong guess silently "
                "configures other feature. Both engines and openings_file must be set "
                "(here or earlier session) before start (type=\"sprt\") succeeds. Never type "
                "or guess a path yourself: ask the user for it, or -- if "
                "openings_file_dialog/pgn_file_dialog are listed among the parameters above -- "
                "set one to true to open a native file picker. "
                "draw_mode/resign_mode \"off\"/\"test\"/\"active\"; both disabled "
                "(\"off\") by default.",
            .params = configureParams(),
            .invoke = [](const ConfigureSprtRequest& request) {
                // Either role on its own is a complete request: the one not named keeps its
                // current engine, exactly as an unpassed time_control does.
                if (request.champion || request.challenger) {
                    auto selected =
                        Actions::selectSprtEngines(request.champion, request.challenger);
                    if (!selected.ok) {
                        return selected;
                    }
                }
                return Actions::configureSprt(request);
            },
            .timeout = Tools::FILE_DIALOG_TIMEOUT});
}

} // namespace QaplaLlm
