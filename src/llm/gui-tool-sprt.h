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
 * @brief Registers the "SPRT" tool group: select_sprt_engines, configure_sprt (also covers
 * draw/resign adjudication). start/stop/get_status/clear_result/show_result are all handled by
 * the unified tools in gui-tool-status-register.cpp instead -- see the exported handle*
 * functions below.
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

/**
 * @brief Starts the SPRT test (see SprtTournamentData::startTournament()), reporting exactly
 * which precondition is missing if it can't.
 *
 * Exported rather than kept file-local so the unified "start" tool (gui-tool-status-register.cpp)
 * can dispatch into it by type="sprt" -- there is no separate model-visible "start_sprt" tool
 * anymore.
 */
[[nodiscard]] GuiToolResult handleStartSprtTournament(const QaplaTester::Json::JsonValue& arguments);

/**
 * @brief Stops the running SPRT test. `arguments["mode"]` = "graceful" (default) or "abrupt".
 *
 * Exported for the same reason as handleStartSprtTournament() -- dispatched into by the unified
 * "stop" tool.
 */
[[nodiscard]] GuiToolResult handleStopSprtTournament(const QaplaTester::Json::JsonValue& arguments);

/**
 * @brief Reports the SPRT test's full current config/state as text.
 *
 * Exported so the unified "get_status" tool can dispatch into it by type="sprt" -- there is no
 * separate model-visible "get_sprt_status" tool anymore.
 */
[[nodiscard]] GuiToolResult handleGetSprtStatus(const QaplaTester::Json::JsonValue& arguments);

/**
 * @brief Discards current SPRT test results (stops it first if still running).
 *
 * Exported so the unified "clear_result" tool can dispatch into it by type="sprt" -- there is
 * no separate model-visible "clear_sprt_result" tool anymore.
 */
[[nodiscard]] GuiToolResult handleClearSprtResult(const QaplaTester::Json::JsonValue& arguments);

/**
 * @brief Renders the current SPRT test results as live tables in the chat (see
 * GuiToolResult::renderWidget).
 *
 * Exported so the unified "show_result" tool can dispatch into it by type="sprt" -- there is no
 * separate model-visible "show_sprt_result" tool anymore.
 */
[[nodiscard]] GuiToolResult handleShowSprtResult(const QaplaTester::Json::JsonValue& arguments);

} // namespace QaplaLlm
