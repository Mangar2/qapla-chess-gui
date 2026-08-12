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

    /** @brief Arguments of select_sprt_engines. */
    struct SelectSprtEnginesRequest {
        std::string champion;
        std::string challenger;
    };

    std::vector<Api::Param<SprtSettings>> configureParams() {
        std::vector<Api::Param<SprtSettings>> params;
        params.push_back(Tools::timeControlParam<SprtSettings>());
        params.push_back(Api::numberParam<SprtSettings>("elo0", &SprtSettings::eloH0,
            "H0 (null hypothesis) Elo bound -- challenger NOT stronger at/below this Elo diff. "
            "Usually 0. Default 0."));
        params.push_back(Api::numberParam<SprtSettings>("elo1", &SprtSettings::eloH1,
            "H1 (alt hypothesis) Elo bound -- challenger stronger at/above this Elo diff. Must "
            "end up > elo0 (checked at test start, not when this field alone set). Default 3."));
        params.push_back(Api::numberParam<SprtSettings>("alpha", &SprtSettings::alpha,
            "Type I error rate: prob. of accepting challenger as stronger when it isn't. 0-1 "
            "exclusive. Default 0.05."));
        params.push_back(Api::numberParam<SprtSettings>("beta", &SprtSettings::beta,
            "Type II error rate: prob. of rejecting challenger when it is stronger. 0-1 "
            "exclusive. Default 0.05."));
        params.push_back(Api::integerParam<SprtSettings>("max_games", &SprtSettings::maxGames,
            "Max games before stopping inconclusive if no decision sooner. Default 100000."));
        params.push_back(Api::stringParam<SprtSettings>("model", &SprtSettings::model,
            "Statistical model for SPRT calc: \"normalized\" (default), \"logistic\", "
            "\"bayesian\"."));
        params.push_back(Api::boolParam<SprtSettings>("pentanomial", &SprtSettings::pentanomial,
            "True: pentanomial (paired-game) stats instead of trinomial -- more power, needs "
            "games in same-opening pairs. Default false."));
        params.push_back(Tools::openingsFileParam<SprtSettings>(
            "Path to existing EPD/PGN opening book file on disk."));
        params.push_back(Tools::openingsFileDialogParam<SprtSettings>());
        params.push_back(Tools::pgnFileParam<SprtSettings>());
        params.push_back(Tools::pgnFileDialogParam<SprtSettings>());
        params.push_back(Api::integerParam<SprtSettings>("concurrency", &SprtSettings::concurrency,
            "Games to run in parallel."));
        Tools::appendAdjudicationParams(params);
        return params;
    }
} // namespace

void registerSprtTools(GuiToolRegistry& registry) {
    Api::defineTool<SelectSprtEnginesRequest>(registry,
        {.name = "select_sprt_engines",
            .description =
                "Selects two engines for SPRT test: \"champion\" (comparison baseline), "
                "\"challenger\" (engine under test), replacing prior selection. Names "
                "matched case-insensitive vs installed engine catalog; informal/"
                "shortened name (e.g. \"spike\") auto-matched to the one installed "
                "engine it can only mean (e.g. \"Spike 1.4.1\") -- pass name user "
                "actually said, no need to call list_installed_engines first just to "
                "find exact full name. If name could mean >1 installed engine, result "
                "lists candidates -- ask user which they meant, never guess. Entirely "
                "separate from select_engines/configure_tournament (classic tournament "
                "mode) -- selecting SPRT engines never changes tournament's engine "
                "selection, and vice versa. Rejected while a test runs or stops; stop it first. "
                "open_add_engine_dialog is not affected.",
            .params = {Api::stringParam<SelectSprtEnginesRequest>("champion",
                           &SelectSprtEnginesRequest::champion,
                           "Comparison/baseline engine -- trusted one, challenger tested against "
                           "it. Matched case-insensitive vs installed engine catalog -- call "
                           "list_installed_engines first if unsure what's available.",
                           true),
                Api::stringParam<SelectSprtEnginesRequest>("challenger",
                    &SelectSprtEnginesRequest::challenger,
                    "Engine under test. Must differ from champion.", true)},
            .invoke = [](const SelectSprtEnginesRequest& request) {
                return Actions::selectSprtEngines(request.champion, request.challenger);
            }});

    Api::defineTool<SprtSettings>(registry,
        {.name = "configure_sprt",
            .description =
                "Sets SPRT test options: time_control, elo0/elo1 (H0/H1 Elo bounds), "
                "alpha, beta, max_games, model, pentanomial, openings_file, pgn_file, "
                "concurrency, draw_mode/draw_min_full_moves/"
                "draw_required_consecutive_moves/draw_centipawn_threshold, resign_mode/"
                "resign_required_consecutive_moves/resign_centipawn_threshold/"
                "resign_two_sided. Every field independent/optional -- pass ONLY what "
                "user asked to change, don't require/ask for others first. Anything not "
                "passed keeps prior value (this session or earlier). Response reports the "
                "full current SPRT config, so no separate get_status (type=\"sprt\") call "
                "is normally needed. While a test runs or stops, every field except concurrency "
                "is rejected; stop it first. concurrency applies immediately. "
                "IMPORTANT: fully separate config from configure_tournament -- classic "
                "tournament and SPRT each have own time control, openings file, "
                "concurrency, adjudication etc, despite same field names. If user's "
                "message doesn't make clear tournament vs SPRT (e.g. bare \"set time "
                "control to 1 min/game\"), infer from conversation so far (multi-engine "
                "tournament or champion-vs-challenger SPRT?); if still unclear from "
                "context, ask which they mean, never guess -- wrong guess silently "
                "configures other feature. openings_file must be set (here or earlier "
                "session) before start (type=\"sprt\") succeeds. If missing/invalid or "
                "user wants to browse, set openings_file_dialog/pgn_file_dialog to true "
                "instead of a typed path. "
                "draw_mode/resign_mode \"off\"/\"test\"/\"active\"; both disabled "
                "(\"off\") by default.",
            .params = configureParams(),
            .invoke = [](const SprtSettings& settings) { return Actions::configureSprt(settings); },
            .timeout = Tools::FILE_DIALOG_TIMEOUT});
}

} // namespace QaplaLlm
