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

#include "../gui-tool-registry.h"

/**
 * @file
 * @brief The EXTERNAL tool API: everything the model sees, and nothing else.
 *
 * Each file in src/llm/tools/ declares its tools as data -- name, description, parameters, and
 * one `invoke` lambda per tool that calls the internal actions (src/llm/actions/). Nothing here
 * touches a GUI singleton, and nothing in the actions layer knows a tool name, a parameter name,
 * or a JSON Schema.
 *
 * That split exists because these two APIs change for completely different reasons, and at
 * completely different rates. The model-facing side is tuned constantly -- renaming a field so a
 * small model stops confusing it with another, merging five tools into one that takes a "type",
 * splitting one back apart, rewording an enum, moving a parameter to a different tool. None of
 * that says anything about what the GUI should do, so none of it should reach the GUI code:
 * a reshape of the external API is a change to these files and (rarely) to the mapper in
 * src/llm/api/, and it stops there.
 *
 * The tool set is deliberately small (currently eleven), and the two shapes it settled on say why
 * one tool exists rather than another:
 *
 * - One tool per *topic* for configuration. configure_tournament / configure_sprt / configure_epd
 *   each own everything about one activity, engine selection included, so "run A against B at
 *   1+0" is one call rather than a two-call sequence in an order the caller had to know.
 * - One tool per *verb* for the lifecycle. start / stop / clear_result / get_status take the topic
 *   as a "type" argument. Keeping the verb in the name -- rather than folding all four into the
 *   three topic tools as an "action" argument -- keeps the destructive ones visible at a glance in
 *   a log, a permission prompt or a chat transcript, and keeps each tool's schema a real contract
 *   instead of the union of four actions' fields where nothing can be required.
 *
 * Reporting is one tool, not three. get_status / get_running_status / show_result used to be
 * separate, and their descriptions consisted largely of telling each other apart -- the surest
 * sign that the split was ours rather than the caller's. Picking the wrong one of three is a real
 * failure; being handed more than was asked for is not.
 *
 * To add a tool: declare it with Api::defineTool() in the file for its feature, and add its
 * register function to registerGuiTools(). To change one: edit its declaration -- the parameter
 * list is simultaneously the schema the model is shown and the code that reads its arguments, so
 * there is no second place that can fall out of sync.
 */

namespace QaplaLlm {

/** @brief Registers the tournament tool: configure_tournament (engines included). */
void registerTournamentTools(GuiToolRegistry& registry);

/** @brief Registers the SPRT tool: configure_sprt (champion/challenger included). */
void registerSprtTools(GuiToolRegistry& registry);

/** @brief Registers the EPD tool: configure_epd (engines included). */
void registerEpdTools(GuiToolRegistry& registry);

/** @brief Registers the CLOP tuning tool. Remote-only -- see GuiToolDefinition::remoteOnly. */
void registerClopTools(GuiToolRegistry& registry);

/**
 * @brief Registers the cross-feature tools: start, stop, get_status, clear_result.
 *
 * Each takes a "type" ("tournament"/"sprt"/"epd") rather than existing as twelve separate
 * per-feature tools -- purely an external packaging decision, made because a small model picks the
 * right one far more reliably from four tools plus an enum than from twelve names. get_status
 * takes it optionally: omitted, it reports across all three at once.
 */
void registerActivityTools(GuiToolRegistry& registry);

/** @brief Registers the app-level tools: close_application, open_pgn_file. */
void registerAppTools(GuiToolRegistry& registry);

/** @brief Registers the engine catalog tools: list_installed_engines, open_add_engine_dialog. */
void registerEngineTools(GuiToolRegistry& registry);

/**
 * @brief Registers every tool group. Registration is cheap and unconditional -- tools only ever
 * run when a chat actually calls them.
 */
void registerGuiTools(GuiToolRegistry& registry);

} // namespace QaplaLlm
