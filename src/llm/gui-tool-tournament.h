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

#include <engine-handling/engine-config.h>

#include <string>
#include <vector>

namespace QaplaLlm {

/**
 * @brief Registers the "Turnier" tool group (select_engines, configure_tournament,
 * clear/show_tournament_result) from docs/llm-chatbot-plan.md Step 4. start/stop are handled
 * by the unified start/stop tool in gui-tool-status-register.cpp instead -- see
 * handleStartTournament/handleStopTournament below.
 */
void registerTournamentTools(GuiToolRegistry& registry);

/**
 * @brief Starts the tournament (see TournamentData::startTournament()), reporting exactly which
 * precondition is missing (no engines selected, no openings file, etc.) if it can't.
 *
 * Exported rather than kept file-local so the unified "start" tool (gui-tool-status-register.cpp)
 * can dispatch into it by type="tournament" -- there is no separate model-visible
 * "start_tournament" tool anymore.
 */
[[nodiscard]] GuiToolResult handleStartTournament(const QaplaTester::Json::JsonValue& arguments);

/**
 * @brief Stops the running tournament. `arguments["mode"]` = "graceful" (default) or "abrupt".
 *
 * Exported for the same reason as handleStartTournament() -- dispatched into by the unified
 * "stop" tool.
 */
[[nodiscard]] GuiToolResult handleStopTournament(const QaplaTester::Json::JsonValue& arguments);

/**
 * @brief One chat-provided name that matched more than one installed engine.
 */
struct AmbiguousEngineName {
    std::string given;                     ///< The name as given (e.g. "spike").
    std::vector<std::string> matches;       ///< Installed engine names it could mean.
};

/**
 * @brief Outcome of resolving chat-provided engine names against the global engine catalog.
 */
struct ResolveEnginesOutcome {
    std::vector<QaplaTester::EngineConfig> resolved; ///< Catalog copies, marked selected (not gauntlet).
    std::vector<std::string> notFound;                ///< Names with no matching catalog entry, not even by substring.
    std::vector<AmbiguousEngineName> ambiguous;       ///< Names that substring-matched more than one installed engine.
};

/**
 * @brief Resolves engine names against the global engine catalog, marking each match
 * selected for a tournament.
 *
 * First tries an exact, case-insensitive match (EngineConfigManager::getConfig()). If that
 * fails, falls back to a case-insensitive substring match against installed engine names --
 * e.g. the model/user says "spike" and the catalog has "Spike 1.4.1": there's no exact match,
 * but exactly one installed name contains "spike", so that one is used. If more than one
 * installed name contains the given text, the name is reported as ambiguous instead of
 * guessing one -- see AmbiguousEngineName -- so the caller can ask which one was meant.
 *
 * Pure/UI-independent (only touches EngineWorkerFactory's config manager, not
 * TournamentData) so it can be unit-tested directly; the "select_engines" tool
 * handler is a thin wrapper that hands the result to
 * TournamentData::instance().getEngineSelect().
 */
[[nodiscard]] ResolveEnginesOutcome resolveEngines(const std::vector<std::string>& names);

/**
 * @brief Formats resolveEngines()'s ambiguous names as one sentence per name, e.g.
 * "\"spike\" could mean: Spike 1.4.1, Spike Classic." -- for a tool result asking the user
 * (via the model) which one they meant. Empty string if `ambiguous` is empty.
 */
[[nodiscard]] std::string formatAmbiguousEngineNames(const std::vector<AmbiguousEngineName>& ambiguous);

} // namespace QaplaLlm
