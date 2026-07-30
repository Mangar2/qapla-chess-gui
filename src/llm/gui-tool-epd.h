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
 * @brief Registers the "EPD" tool group: select_epd_engines, configure_epd,
 * get_epd_status, start_epd_analysis, stop_epd_analysis, clear_epd_result, show_epd_result.
 *
 * EPD analysis tests one or more engines against a fixed set of positions (from an EPD/RAW
 * position file), checking whether each engine finds the expected best move within a time
 * budget -- a different activity from both the multi-engine round-robin tournament and the
 * champion-vs-challenger SPRT test, with its own entirely separate settings (see
 * QaplaWindows::EpdData): own engine selection, own concurrency, no adjudication concept at
 * all, and critically no shared "time_control" string -- just plain per-position
 * max/min-time-in-seconds fields. Every tool name in this group says "epd" so a model (and the
 * system prompt guidance in ChatbotLlmChat) can always tell it apart from configure_tournament/
 * configure_sprt, exactly like the existing tournament-vs-SPRT split.
 *
 * No separate gui-tool-epd.cpp/pure-logic split: like SPRT, this group has no logic of its own
 * worth unit-testing in isolation -- engine name resolution reuses resolveEngines() from
 * gui-tool-tournament.h, and everything else is a thin wrapper around QaplaWindows::EpdData, so
 * only the qapla executable links this.
 */
void registerEpdTools(GuiToolRegistry& registry);

} // namespace QaplaLlm
