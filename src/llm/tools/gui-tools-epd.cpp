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

    /** @brief Arguments of select_epd_engines. */
    struct SelectEpdEnginesRequest {
        std::vector<std::string> engines;
    };

    std::vector<Api::Param<EpdSettings>> configureParams() {
        std::vector<Api::Param<EpdSettings>> params;
        params.push_back(Api::stringParam<EpdSettings>("epd_file", &EpdSettings::epdFile,
            "Path to existing EPD (or RAW position) file on disk."));
        params.push_back(Api::flagParam<EpdSettings>("epd_file_dialog", &EpdSettings::pickEpdFile,
            "Set true to open a native file picker instead of passing epd_file."));
        params.push_back(Api::integerParam<EpdSettings>("max_time_seconds",
            &EpdSettings::maxTimeInSeconds,
            "Max seconds engine may search each position. NOT tournament/SPRT time control -- "
            "EPD analysis has no clock string, just plain per-position time budget. Default 10."));
        params.push_back(Api::integerParam<EpdSettings>("min_time_seconds",
            &EpdSettings::minTimeInSeconds,
            "Min seconds engine must keep searching each position even after finding apparent "
            "right move (guards vs solving by luck on shallow search). Default 1."));
        params.push_back(Api::integerParam<EpdSettings>("seen_plies", &EpdSettings::seenPlies,
            "Consecutive plies engine's PV must keep showing correct best move before position "
            "counted solved, analysis moves on early -- saves time on easy positions. Default 3."));
        params.push_back(Api::integerParam<EpdSettings>("concurrency", &EpdSettings::concurrency,
            "Positions to analyze in parallel."));
        return params;
    }
} // namespace

void registerEpdTools(GuiToolRegistry& registry) {
    Api::defineTool<SelectEpdEnginesRequest>(registry,
        {.name = "select_epd_engines",
            .description =
                "Selects configured chess engines tested vs EPD positions, replacing any "
                "previous selection. Names matched case-insensitively vs installed "
                "engine catalog; informal/shortened name (e.g. \"spike\") auto-matched "
                "to the one installed engine it can only mean (e.g. \"Spike 1.4.1\") -- "
                "pass name user actually said, no need to call list_installed_engines "
                "first just to look up full name. If name could mean more than one "
                "installed engine, result lists candidates -- ask user which one meant, "
                "never guess. Entirely separate from select_engines/select_sprt_engines "
                "(classic tournament/SPRT) -- selecting EPD engines never changes those, "
                "vice versa.",
            .params = {Api::stringListParam<SelectEpdEnginesRequest>("engines",
                &SelectEpdEnginesRequest::engines,
                "Engine display names to test vs EPD positions, e.g. [\"Stockfish\", "
                "\"Qapla\"]. All tested vs SAME position set, side by side (one result column "
                "each) -- unlike SPRT, no champion/challenger role; any number of engines "
                "(incl. one) fine.",
                true)},
            .invoke = [](const SelectEpdEnginesRequest& request) {
                return Actions::selectEpdEngines(request.engines);
            }});

    Api::defineTool<EpdSettings>(registry,
        {.name = "configure_epd",
            .description =
                "Sets EPD analysis options: epd_file, max_time_seconds, "
                "min_time_seconds, seen_plies, concurrency. Each field independent, "
                "optional -- pass ONLY what user asked to change, don't require/ask for "
                "others first. Unset fields keep prior value (this session or earlier) "
                "-- call get_status (type=\"epd\") first if unsure what's current. "
                "IMPORTANT: completely separate from configure_tournament/configure_sprt "
                "-- EPD has no shared time_control string (just plain per-position second "
                "counts), no adjudication concept, despite sounding like another "
                "engine-testing mode. If request could mean tournament, SPRT, or EPD "
                "and unclear which, ask, don't guess. epd_file must be set (here or "
                "earlier session) before start (type=\"epd\") succeeds. If missing, "
                "invalid, or user wants to browse, set epd_file_dialog=true instead of "
                "a typed path.",
            .params = configureParams(),
            .invoke = [](const EpdSettings& settings) { return Actions::configureEpd(settings); },
            .timeout = Tools::FILE_DIALOG_TIMEOUT});
}

} // namespace QaplaLlm
