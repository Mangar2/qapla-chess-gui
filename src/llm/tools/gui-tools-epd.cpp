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
#include "../actions/gui-action-epd.h"

namespace QaplaLlm {

namespace {
    using Actions::EpdSettings;

    /**
     * @brief Arguments of configure_epd: the settings patch plus the engine selection.
     *
     * Engines used to be a tool of their own (select_epd_engines) -- see
     * ConfigureTournamentRequest in gui-tools-tournament.cpp for why they are a field here now.
     */
    struct ConfigureEpdRequest : EpdSettings {
        std::vector<std::string> engines;
    };

    std::vector<Api::Param<ConfigureEpdRequest>> configureParams() {
        using Request = ConfigureEpdRequest;
        std::vector<Api::Param<Request>> params;
        params.push_back(Api::stringListParam<Request>("engines", &Request::engines,
            "Engine display names to test against the positions, e.g. [\"Stockfish\",\"Qapla\"]. "
            "Replaces the previous selection. All are tested against the SAME position set, side "
            "by side (one result column each) -- unlike SPRT there is no champion/challenger "
            "role, and any number of engines (including one) is fine. Matched case-insensitively "
            "against the installed engine catalog, and a shortened name (e.g. \"spike\") is "
            "matched to the one engine it can mean; if it could mean more than one, nothing at "
            "all is changed and the result lists the candidates: ask the user, never guess."));
        params.push_back(Api::stringParam<Request>("epd_file", &EpdSettings::epdFile,
            "Path to existing EPD (or RAW position) file on disk."));
        auto epdFileDialog = Api::flagParam<Request>("epd_file_dialog", &EpdSettings::pickEpdFile,
            "Set true to open a native file picker instead of passing epd_file.");
        epdFileDialog.localOnly = true; // see Tools::openingsFileDialogParam()
        params.push_back(std::move(epdFileDialog));
        params.push_back(Api::integerParam<Request>("max_time_seconds",
            &EpdSettings::maxTimeInSeconds,
            "Max seconds engine may search each position. NOT tournament/SPRT time control -- "
            "EPD analysis has no clock string, just plain per-position time budget. Default 10."));
        params.push_back(Api::integerParam<Request>("min_time_seconds",
            &EpdSettings::minTimeInSeconds,
            "Min seconds engine must keep searching each position even after finding apparent "
            "right move (guards vs solving by luck on shallow search). Default 1."));
        params.push_back(Api::integerParam<Request>("seen_plies", &EpdSettings::seenPlies,
            "Consecutive plies engine's PV must keep showing correct best move before position "
            "counted solved, analysis moves on early -- saves time on easy positions. Default 3."));
        params.push_back(Api::integerParam<Request>("concurrency", &EpdSettings::concurrency,
            "Positions to analyze in parallel."));
        return params;
    }
} // namespace

void registerEpdTools(GuiToolRegistry& registry) {
    Api::defineTool<ConfigureEpdRequest>(registry,
        {.name = "configure_epd",
            .description =
                "Sets everything about the EPD analysis: engines, epd_file, "
                "max_time_seconds, min_time_seconds, seen_plies, concurrency. Each field "
                "independent, optional -- pass ONLY what user asked to change, and pass "
                "them together in one call when they asked for several; don't require/ask "
                "for others first. Unset fields keep prior value (this session or "
                "earlier). Response reports the full resulting EPD config, so no separate "
                "get_status call is needed to confirm it. While an analysis runs or stops, "
                "every field except concurrency is rejected and nothing changes; stop it "
                "first. concurrency applies immediately. If \"engines\" can't be resolved, "
                "nothing at all is applied, not even the other fields. "
                "IMPORTANT: completely separate from configure_tournament/configure_sprt "
                "-- EPD has its own engines and no shared time_control string (just plain "
                "per-position second counts), no adjudication concept, despite sounding "
                "like another engine-testing mode. If request could mean tournament, SPRT, "
                "or EPD and unclear which, ask, don't guess. engines and epd_file must "
                "both be set (here or earlier session) before start (type=\"epd\") "
                "succeeds. Never type or guess a path yourself: ask the user for it, or -- if "
                "epd_file_dialog is listed among the parameters above -- set it to true to open "
                "a native file picker.",
            .params = configureParams(),
            .invoke = [](const ConfigureEpdRequest& request) {
                // Engines first, and all-or-nothing -- see configure_tournament for why.
                if (!request.engines.empty()) {
                    auto selected = Actions::selectEpdEngines(request.engines);
                    if (!selected.ok) {
                        return selected;
                    }
                    auto configured = Actions::configureEpd(request);
                    if (!selected.text.empty()) {
                        configured.text = selected.text + " " + configured.text;
                    }
                    return configured;
                }
                return Actions::configureEpd(request);
            },
            .timeout = Tools::FILE_DIALOG_TIMEOUT});
}

} // namespace QaplaLlm
