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
#include "../actions/gui-action-analysis.h"

namespace QaplaLlm {

namespace {
    using Actions::AnalysisSettings;

    /** @brief Arguments of configure_backward_analysis: the settings patch plus the engines. */
    struct ConfigureAnalysisRequest : AnalysisSettings {
        std::vector<std::string> engines;
    };

    std::vector<Api::Param<ConfigureAnalysisRequest>> configureParams() {
        using Request = ConfigureAnalysisRequest;
        std::vector<Api::Param<Request>> params;
        params.push_back(Api::stringListParam<Request>("engines", &Request::engines,
            "Engine display names that analyse the games, e.g. [\"Stockfish\",\"Qapla\"]. "
            "Replaces the previous selection. Every engine analyses every game and writes its "
            "own copy of it, told apart by the PGN Annotator tag -- there are no sides and no "
            "opponents here, so any number (including one) is fine. Matched case-insensitively "
            "against the installed engine catalog; a shortened name that could mean more than "
            "one engine changes nothing at all and the result lists the candidates: ask the "
            "user, never guess."));
        params.push_back(Api::stringParam<Request>("pgn_file", &AnalysisSettings::pgnFile,
            "Path to the existing PGN file holding the games to analyse. Every game in it is "
            "analysed; there is no filter on this route."));
        params.push_back(Api::integerParam<Request>("move_time_ms", &AnalysisSettings::moveTimeMs,
            "Milliseconds the engine may think about EACH position. NOT a clock: every position "
            "of every game gets exactly this, which is what makes the evaluations comparable. A "
            "game of 80 half moves therefore takes 80 times this per engine. Default 1000."));
        params.push_back(Api::stringParam<Request>("output_file", &AnalysisSettings::outputFile,
            "Path of the PGN file the analysed games are written to, as they finish. Must not be "
            "the file being read. Its directory has to exist; the file itself is created."));
        params.push_back(Api::boolParam<Request>("append_output", &AnalysisSettings::appendOutput,
            "True to append to the output file, false to overwrite it when the run starts."));
        params.push_back(Api::integerParam<Request>("concurrency", &AnalysisSettings::concurrency,
            "Games analysed at the same time, each on an engine of its own. Applies immediately, "
            "including while a run is going on."));
        return params;
    }
} // namespace

void registerAnalysisTools(GuiToolRegistry& registry) {
    Api::defineTool<ConfigureAnalysisRequest>(registry,
        {.name = "configure_backward_analysis",
            .description =
                "Sets everything about the backward analysis: engines, pgn_file, move_time_ms, "
                "output_file, append_output, concurrency. Each field independent and optional -- "
                "pass ONLY what the user asked to change, and pass them together in one call "
                "when they asked for several. Unset fields keep their previous value (this "
                "session or an earlier one). The response reports the full resulting "
                "configuration, so no separate get_status call is needed to confirm it. "
                "What the backward analysis does: it takes the games of a PGN file and has an "
                "engine recompute every position of each game, walking BACKWARDS from the last "
                "move to the first, and writes the games out again carrying the evaluations, "
                "principal variations, depths and times it found. The players of each game stay "
                "as they were -- the analysing engine is not one of them. Walking backwards is "
                "the point: the engine keeps what it learned about the later positions and meets "
                "each earlier one already knowing how the game continued. "
                "IMPORTANT: completely separate from configure_tournament/configure_sprt/"
                "configure_epd. No games are played, there is no clock and no opponent, and the "
                "only search limit is move_time_ms. If a request could mean a tournament, an "
                "SPRT test, an EPD run or this, ask, don't guess. engines, pgn_file, "
                "output_file and move_time_ms must all be set (here or in an earlier session) "
                "before start (type=\"analysis\") succeeds. While a run is going on, every field "
                "except concurrency is rejected and nothing changes; stop it first. Never type "
                "or guess a path yourself: ask the user for it.",
            .params = configureParams(),
            .invoke = [](const ConfigureAnalysisRequest& request) {
                // Engines first, and all-or-nothing -- see configure_tournament for why.
                if (!request.engines.empty()) {
                    auto selected = Actions::selectAnalysisEngines(request.engines);
                    if (!selected.ok) {
                        return selected;
                    }
                    auto configured = Actions::configureAnalysis(request);
                    if (!selected.text.empty()) {
                        configured.text = selected.text + " " + configured.text;
                    }
                    return configured;
                }
                return Actions::configureAnalysis(request);
            }});
}

} // namespace QaplaLlm
