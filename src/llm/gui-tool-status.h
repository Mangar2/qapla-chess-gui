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

#pragma once

#include "gui-tool-registry.h"

namespace QaplaLlm {

/**
 * @brief Registers the cross-feature tools: get_running_status, start, stop, get_status,
 * clear_result, show_result. Each of the latter five takes a "type" ("tournament"/"sprt"/"epd")
 * and dispatches into the matching feature file's exported handle* function (see
 * gui-tool-tournament.h / gui-tool-sprt.h / gui-tool-epd.h) -- there are no separate
 * per-feature start_tournament/get_tournament_status-style tools anymore.
 *
 * get_running_status exists on top of that because tournament/SPRT/EPD are separate,
 * independently-running activities, but users often call an SPRT test or EPD analysis a
 * "tournament" informally, since to a person running one it looks and feels the same (engines
 * playing games/analyzing positions in the background). Answering "is a tournament running?"
 * using only get_status (type="tournament") would incorrectly say "no" while an SPRT test or
 * EPD analysis is actively running. This tool exists so the model has one place to check all
 * three at once and report accurately.
 */
void registerStatusTools(GuiToolRegistry& registry);

} // namespace QaplaLlm
