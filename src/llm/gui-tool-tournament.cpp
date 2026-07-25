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

// Deliberately kept free of any GUI/ImGui dependency (no tournament-data.h,
// no configuration.h): this is the pure, UI-independent logic that the
// unit-tests target links directly. The actual tool registration -- reading
// and writing TournamentData -- is in gui-tool-tournament-register.cpp,
// which only the qapla executable links (tournament-data.h transitively
// pulls in the whole GUI stack).

#include "gui-tool-tournament.h"

#include <engine-handling/engine-worker-factory.h>

namespace QaplaLlm {

ResolveEnginesOutcome resolveEngines(const std::vector<std::string>& names) {
    ResolveEnginesOutcome outcome;
    const auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();

    for (const auto& name : names) {
        const auto* found = configManager.getConfig(name);
        if (found == nullptr) {
            outcome.notFound.push_back(name);
            continue;
        }
        auto engine = *found;
        engine.setSelected(true);
        engine.setGauntlet(false);
        outcome.resolved.push_back(std::move(engine));
    }
    return outcome;
}

} // namespace QaplaLlm
