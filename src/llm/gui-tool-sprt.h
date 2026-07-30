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
 * @brief Registers the "SPRT" tool group: select_sprt_engines, configure_sprt,
 * configure_sprt_draw_adjudication, configure_sprt_resign_adjudication, get_sprt_status,
 * start_sprt_tournament, stop_sprt_tournament, clear_sprt_result, show_sprt_result.
 *
 * SPRT (Sequential Probability Ratio Test) compares exactly two engines -- a "champion"
 * (comparison baseline) and a "challenger" (engine under test) -- unlike tournament mode's
 * many-engines round robin/gauntlet. It has its own, entirely separate copy of every setting
 * that looks the same as a tournament setting (engine selection, time control, concurrency,
 * openings file, PGN output, adjudication -- see QaplaWindows::SprtTournamentData): configuring
 * one never affects the other. Every tool name in this group says "sprt" precisely so a model
 * (and the system prompt guidance in ChatbotLlmChat) can always tell, unambiguously, which of
 * the two independent configurations a call targets.
 *
 * No separate gui-tool-sprt.cpp/pure-logic split: this group has no logic of its own worth
 * unit-testing in isolation -- engine name resolution reuses resolveEngines() from
 * gui-tool-tournament.h (already pure/tested there), and everything else is a thin wrapper
 * around QaplaWindows::SprtTournamentData, so only the qapla executable links this.
 */
void registerSprtTools(GuiToolRegistry& registry);

} // namespace QaplaLlm
