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

// Deliberately kept free of any GUI/ImGui dependency (no os-dialogs.h,
// no configuration.h): this is the pure, UI-independent logic that the
// unit-tests target links directly. The actual tool registration --
// including the native file dialog and Configuration::setModified() -- is
// in gui-tool-engine-management-register.cpp, which only the qapla
// executable links (that file's dependencies drag in the whole GUI stack).

#include "gui-tool-engine-management.h"

#include <engine-handling/engine-config.h>
#include <engine-handling/engine-worker-factory.h>

#include <algorithm>

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;
}

std::string listInstalledEnginesJson() {
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();

    auto array = Json::JsonValue::array();
    for (const auto& config : configManager.getAllConfigs()) {
        auto entry = Json::JsonValue::object();
        entry["name"] = config.getName();
        entry["protocol"] = QaplaTester::to_string(config.getProtocol());
        array.push_back(entry);
    }
    return array.stringify();
}

AddEnginesOutcome addEnginesFromPaths(const std::vector<std::string>& paths) {
    AddEnginesOutcome outcome;

    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    auto existing = configManager.getAllConfigs();

    for (const auto& path : paths) {
        auto newConfig = QaplaTester::EngineConfig::createFromPath(path);

        bool isDuplicate = std::ranges::any_of(existing, [&](const auto& config) {
            return config.getCmd() == newConfig.getCmd() && config.getProtocol() == newConfig.getProtocol();
        });
        if (isDuplicate) {
            outcome.duplicateNames.push_back(newConfig.getName());
            continue;
        }

        configManager.addConfig(newConfig);
        existing.push_back(newConfig); // keep the dedup view in sync for the rest of this call
        outcome.addedNames.push_back(newConfig.getName());
    }

    return outcome;
}

} // namespace QaplaLlm
