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

#include <engine-handling/engine-config.h>

#include <string>
#include <vector>

/**
 * @file
 * @brief Engine-name resolution, shared by every feature that lets a chat name engines.
 *
 * Deliberately outside both tool layers: it is neither model-facing (src/llm/tools/) nor a GUI
 * action (src/llm/actions/), just pure logic over the engine catalog. Keeping it free of GUI
 * includes is what lets the unit-tests target link and test it directly.
 */

namespace QaplaLlm {

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
 * TournamentData) so it can be unit-tested directly; the engine selection actions are thin
 * wrappers that hand the result to getEngineSelect() on the matching activity's data singleton.
 */
[[nodiscard]] ResolveEnginesOutcome resolveEngines(const std::vector<std::string>& names);

/**
 * @brief Formats resolveEngines()'s ambiguous names as one sentence per name, e.g.
 * "\"spike\" could mean: Spike 1.4.1, Spike Classic." -- for a tool result asking the user
 * (via the model) which one they meant. Empty string if `ambiguous` is empty.
 */
[[nodiscard]] std::string formatAmbiguousEngineNames(const std::vector<AmbiguousEngineName>& ambiguous);

} // namespace QaplaLlm
