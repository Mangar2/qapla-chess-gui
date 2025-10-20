/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "configuration.h"
#include "engine-capabilities.h"
#include "snackbar.h"

#include "qapla-tester/engine-config.h"
#include "qapla-tester/engine-config-manager.h"
#include "qapla-tester/engine-worker-factory.h"

#include <thread>

using QaplaTester::EngineConfig;
using QaplaTester::EngineWorkerFactory;
using QaplaTester::EngineWorker;
using QaplaTester::EngineProtocol;

using namespace QaplaConfiguration;

void EngineCapabilities::autoDetect() {
    std::thread([this]() {
		detecting_ = true;
        std::vector<EngineConfig> configs;
        for (auto& config : EngineWorkerFactory::getConfigManager().getAllConfigs()) {
            if (!hasAnyCapability(config.getCmd())) {
                configs.push_back(config);
			}
        }
        auto engines = EngineWorkerFactory::createEngines(configs);
        for (auto& config : configs) {
			// search matching eninge from the created engines
            auto matchingEngine = std::find_if(engines.begin(), engines.end(),
                [&config](const std::unique_ptr<EngineWorker>& engine) {
                    return engine->getConfig().getCmd() == config.getCmd() &&
                           engine->getConfig().getProtocol() == config.getProtocol();
                });
            if (matchingEngine == engines.end()) {
                SnackbarManager::instance().showError(
					"Could not start engine: " + config.getCmd() + " using protocol " +
					to_string(config.getProtocol()));
			}
		}

        for (auto& engine : engines) {
            auto& command = engine->getConfig().getCmd();
            auto config = EngineWorkerFactory::getConfigManagerMutable()
                .getConfigMutableByCmdAndProtocol(command, EngineProtocol::Uci);
            if (config) {
                config->setName(engine->getEngineName());
                config->setAuthor(engine->getEngineAuthor());
            }
			EngineCapability capability;
            capability.setPath(command);
            capability.setProtocol(EngineProtocol::Uci);
            capability.setName(engine->getEngineName());
            capability.setAuthor(engine->getEngineAuthor());
			capability.setSupportedOptions(engine->getSupportedOptions());
			addOrReplace(capability);

        }
		detecting_ = false;
        }).detach();
}

bool EngineCapabilities::areAllEnginesDetected() const {
    for (const auto& config : EngineWorkerFactory::getConfigManager().getAllConfigs()) {
        if (!hasAnyCapability(config.getCmd())) {
            return false;
        }
    }
    return true;
}

