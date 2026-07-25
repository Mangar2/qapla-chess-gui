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
 * start_tournament) from docs/llm-chatbot-plan.md Step 4.
 */
void registerTournamentTools(GuiToolRegistry& registry);

/**
 * @brief Outcome of resolving chat-provided engine names against the global engine catalog.
 */
struct ResolveEnginesOutcome {
    std::vector<QaplaTester::EngineConfig> resolved; ///< Catalog copies, marked selected (not gauntlet).
    std::vector<std::string> notFound;                ///< Names with no matching catalog entry.
};

/**
 * @brief Resolves engine names against the global engine catalog (case-insensitive,
 * see EngineConfigManager::getConfig()), marking each match selected for a tournament.
 *
 * Pure/UI-independent (only touches EngineWorkerFactory's config manager, not
 * TournamentData) so it can be unit-tested directly; the "select_engines" tool
 * handler is a thin wrapper that hands the result to
 * TournamentData::instance().getEngineSelect().
 */
[[nodiscard]] ResolveEnginesOutcome resolveEngines(const std::vector<std::string>& names);

} // namespace QaplaLlm
