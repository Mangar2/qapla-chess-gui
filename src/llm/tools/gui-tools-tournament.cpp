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
#include "../actions/gui-action-tournament.h"

namespace QaplaLlm {

namespace {
    using Actions::TournamentSettings;

    /** @brief Arguments of select_engines. */
    struct SelectEnginesRequest {
        std::vector<std::string> engines;
    };

    std::vector<Api::Param<TournamentSettings>> configureParams() {
        std::vector<Api::Param<TournamentSettings>> params;
        params.push_back(Tools::timeControlParam<TournamentSettings>());
        params.push_back(Api::integerParam<TournamentSettings>("games",
            &TournamentSettings::gamesPerPairing,
            "Games per engine pairing PER ROUND, not tournament total. Total per pairing = "
            "games*rounds, applies to every pairing. User gives one number, no rounds mention "
            "-> set games to that, leave rounds at default 1, so total=what they said. User "
            "gives BOTH total games and rounds (e.g. \"100 games total, 10 rounds\") -> compute "
            "games=total/rounds yourself (100/10 -> games=10, rounds=10), never put total as-is "
            "into games. If unclear whether given count is total or per-round (e.g. \"100 games "
            "and 10 rounds\", not specified which), ask user, never guess."));
        params.push_back(Api::integerParam<TournamentSettings>("rounds", &TournamentSettings::rounds,
            "Times full pairing set repeats. Default 1. See \"games\" for how this multiplies "
            "into tournament total."));
        params.push_back(Api::stringParam<TournamentSettings>("event", &TournamentSettings::event,
            "Tournament/event name."));
        params.push_back(Tools::openingsFileParam<TournamentSettings>(
            "Path to EPD/PGN opening book file."));
        params.push_back(Tools::openingsFileDialogParam<TournamentSettings>());
        params.push_back(Tools::pgnFileParam<TournamentSettings>());
        params.push_back(Tools::pgnFileDialogParam<TournamentSettings>());
        params.push_back(Api::integerParam<TournamentSettings>("concurrency",
            &TournamentSettings::concurrency, "Games run in parallel."));
        Tools::appendAdjudicationParams(params);
        return params;
    }
} // namespace

void registerTournamentTools(GuiToolRegistry& registry) {
    Api::defineTool<SelectEnginesRequest>(registry,
        {.name = "select_engines",
            .description =
                "Selects configured engines for next tournament, replaces previous "
                "selection. Names matched case-insensitively vs installed engine "
                "catalog; informal/short name (e.g. \"spike\") auto-matched to the one "
                "engine it can mean (e.g. \"Spike 1.4.1\") -- pass name user said, no "
                "need to call list_installed_engines first for exact name. If name "
                "matches multiple engines, result lists candidates -- ask user which, "
                "never guess. Sets up round-robin (every engine plays every other); "
                "gauntlet mode not supported via chat.",
            .params = {Api::stringListParam<SelectEnginesRequest>("engines",
                &SelectEnginesRequest::engines,
                "Engine display names, e.g. [\"Stockfish\",\"Qapla\"].", true)},
            .invoke = [](const SelectEnginesRequest& request) {
                return Actions::selectTournamentEngines(request.engines);
            }});

    Api::defineTool<TournamentSettings>(registry,
        {.name = "configure_tournament",
            .description =
                "Sets tournament options: time_control, games (per pairing), rounds, "
                "event (name), openings_file, pgn_file, concurrency, draw_mode/"
                "draw_min_full_moves/draw_required_consecutive_moves/"
                "draw_centipawn_threshold, resign_mode/resign_required_consecutive_moves/"
                "resign_centipawn_threshold/resign_two_sided. Each field independent/"
                "optional -- pass ONLY what user asked to change (e.g. just \"games\"); "
                "don't require/ask other fields first. Unpassed fields keep prior value "
                "(this session or earlier), don't assume unset. Response always reports "
                "the full current tournament config, so no separate get_status "
                "(type=\"tournament\") call is normally needed to confirm what changed. "
                "openings_file must be set (here or earlier session) before start "
                "(type=\"tournament\") succeeds, no safe default. For openings_file/"
                "pgn_file, set openings_file_dialog/pgn_file_dialog to true instead of a "
                "typed path to open a native file picker -- never type/guess a path "
                "yourself. draw_mode/resign_mode "
                "\"off\"/\"test\"/\"active\"; both disabled (\"off\") by default.",
            .params = configureParams(),
            .invoke = [](const TournamentSettings& settings) {
                return Actions::configureTournament(settings);
            },
            .timeout = Tools::FILE_DIALOG_TIMEOUT});
}

} // namespace QaplaLlm
