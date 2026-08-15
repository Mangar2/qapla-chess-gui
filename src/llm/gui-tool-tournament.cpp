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
// unit-tests target links directly. The actions built on it -- reading and
// writing TournamentData -- are in src/llm/actions/gui-action-tournament.cpp,
// which only the qapla executable links (tournament-data.h transitively
// pulls in the whole GUI stack).

#include "gui-tool-tournament.h"

#include <engine-handling/engine-worker-factory.h>
#include <base-elements/string-helper.h>

namespace QaplaLlm {

namespace {
    QaplaTester::EngineConfig markSelected(const QaplaTester::EngineConfig& found) {
        auto engine = found;
        engine.setSelected(true);
        engine.setGauntlet(false);
        return engine;
    }
}

ResolveEnginesOutcome resolveEngines(const std::vector<std::string>& names) {
    ResolveEnginesOutcome outcome;
    const auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();

    for (const auto& name : names) {
        const auto* found = configManager.getConfig(name);
        if (found != nullptr) {
            outcome.resolved.push_back(markSelected(*found));
            continue;
        }

        // No exact (case-insensitive) match -- fall back to a substring match against the
        // installed catalog, so an informal/shortened name (e.g. "spike") still resolves to
        // the one installed engine it can only mean (e.g. "Spike 1.4.1") instead of failing
        // outright with "not installed". Only resolved automatically if exactly one installed
        // name contains it; more than one is reported as ambiguous rather than guessed.
        std::vector<std::string> substringMatches;
        const auto lowerName = QaplaHelpers::to_lowercase(name);
        for (const auto& config : configManager.getAllConfigs()) {
            if (QaplaHelpers::to_lowercase(config.getName()).find(lowerName) != std::string::npos) {
                substringMatches.push_back(config.getName());
            }
        }

        if (substringMatches.size() == 1) {
            outcome.resolved.push_back(markSelected(*configManager.getConfig(substringMatches.front())));
        } else if (substringMatches.size() > 1) {
            outcome.ambiguous.push_back({.given = name, .matches = substringMatches});
        } else {
            outcome.notFound.push_back(name);
        }
    }
    return outcome;
}

std::string formatAmbiguousEngineNames(const std::vector<AmbiguousEngineName>& ambiguous) {
    std::string message;
    for (const auto& entry : ambiguous) {
        if (!message.empty()) {
            message += " ";
        }
        message += "\"" + entry.given + "\" could mean: ";
        for (std::size_t i = 0; i < entry.matches.size(); ++i) {
            if (i > 0) {
                message += ", ";
            }
            message += entry.matches[i];
        }
        message += ".";
    }
    return message;
}

} // namespace QaplaLlm
