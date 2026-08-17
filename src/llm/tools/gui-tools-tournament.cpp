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

    /**
     * @brief Arguments of configure_tournament: the settings patch plus the engine selection.
     *
     * Engines used to be a tool of their own (select_engines). They are a configuration field
     * like any other -- frozen for exactly as long, refused in the same words -- and a caller
     * that wants "Stockfish vs Qapla at 1+0" had to know it needed two calls in the right order.
     * Deriving keeps the settings struct itself free of the external shape, so the actions layer
     * still never hears about this.
     */
    struct ConfigureTournamentRequest : TournamentSettings {
        std::vector<std::string> engines;
    };

    std::vector<Api::Param<ConfigureTournamentRequest>> configureParams() {
        using Request = ConfigureTournamentRequest;
        std::vector<Api::Param<Request>> params;
        params.push_back(Api::stringListParam<Request>("engines", &Request::engines,
            "Engine display names for the tournament, e.g. [\"Stockfish\",\"Qapla\"]. Replaces "
            "the previous selection; sets up a round robin (every engine plays every other). "
            "Matched case-insensitively against the installed engine catalog, and an informal or "
            "shortened name (e.g. \"spike\") is matched to the one engine it can mean (e.g. "
            "\"Spike 1.4.1\") -- pass the name the user said, no need to list the catalog first. "
            "If a name could mean more than one installed engine, nothing at all is changed and "
            "the result lists the candidates: ask the user which, never guess."));
        params.push_back(Tools::timeControlParam<Request>());
        params.push_back(Api::integerParam<Request>("games",
            &TournamentSettings::gamesPerPairing,
            "Games per engine pairing PER ROUND, not tournament total. Total per pairing = "
            "games*rounds, applies to every pairing. User gives one number, no rounds mention "
            "-> set games to that, leave rounds at default 1, so total=what they said. User "
            "gives BOTH total games and rounds (e.g. \"100 games total, 10 rounds\") -> compute "
            "games=total/rounds yourself (100/10 -> games=10, rounds=10), never put total as-is "
            "into games. If unclear whether given count is total or per-round (e.g. \"100 games "
            "and 10 rounds\", not specified which), ask user, never guess."));
        params.push_back(Api::integerParam<Request>("rounds", &TournamentSettings::rounds,
            "Times full pairing set repeats. Default 1. See \"games\" for how this multiplies "
            "into tournament total."));
        params.push_back(Api::stringParam<Request>("event", &TournamentSettings::event,
            "Tournament/event name."));
        params.push_back(Tools::openingsFileParam<Request>(
            "Path to EPD/PGN opening book file."));
        params.push_back(Tools::openingsFileDialogParam<Request>());
        params.push_back(Tools::pgnFileParam<Request>());
        params.push_back(Tools::pgnFileDialogParam<Request>());
        params.push_back(Api::integerParam<Request>("concurrency",
            &TournamentSettings::concurrency, "Games run in parallel."));
        Tools::appendAdjudicationParams(params);
        return params;
    }
} // namespace

void registerTournamentTools(GuiToolRegistry& registry) {
    Api::defineTool<ConfigureTournamentRequest>(registry,
        {.name = "configure_tournament",
            .description =
                "Sets everything about the classic tournament: engines, time_control, "
                "games (per pairing), rounds, event (name), openings_file, pgn_file, "
                "concurrency, draw_mode/draw_min_full_moves/"
                "draw_required_consecutive_moves/draw_centipawn_threshold, resign_mode/"
                "resign_required_consecutive_moves/resign_centipawn_threshold/"
                "resign_two_sided. Each field independent/optional -- pass ONLY what user "
                "asked to change (e.g. just \"games\"), and pass them together in one "
                "call when they asked for several; don't require/ask other fields first. "
                "Unpassed fields keep prior value (this session or earlier), don't assume "
                "unset. Response reports the full resulting tournament config, so no "
                "separate get_status call is needed to confirm it. While a tournament "
                "runs or stops, every field except concurrency is rejected and nothing "
                "changes; stop it first. concurrency applies immediately. If \"engines\" "
                "can't be resolved, nothing at all is applied, not even the other fields. "
                "engines and openings_file must both be set (here or in an earlier "
                "session) before start (type=\"tournament\") succeeds, no safe default. "
                "Never type or guess a path yourself: ask the user for it, or -- if "
                "openings_file_dialog/pgn_file_dialog are listed among the parameters above "
                "-- set one to true to open a native file picker. draw_mode/resign_mode "
                "\"off\"/\"test\"/\"active\"; both disabled (\"off\") by default.",
            .params = configureParams(),
            .invoke = [](const ConfigureTournamentRequest& request) {
                // Engines first, and all-or-nothing: applying the rest of a patch whose engine
                // list turned out to be ambiguous would leave a configuration that matches
                // neither what was asked for nor what was there before.
                if (!request.engines.empty()) {
                    auto selected = Actions::selectTournamentEngines(request.engines);
                    if (!selected.ok) {
                        return selected;
                    }
                    auto configured = Actions::configureTournament(request);
                    if (!selected.text.empty()) {
                        configured.text = selected.text + " " + configured.text;
                    }
                    return configured;
                }
                return Actions::configureTournament(request);
            },
            .timeout = Tools::FILE_DIALOG_TIMEOUT});
}

} // namespace QaplaLlm
