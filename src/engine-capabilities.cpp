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

std::vector<EngineConfig> EngineCapabilities::collectMissingCapabilities() const {
    std::vector<EngineConfig> configs;
    for (auto& config : EngineWorkerFactory::getConfigManager().getAllConfigs()) {
        if (!hasAnyCapability(config.getCmd(), config.getProtocol())) {
            configs.push_back(config);
        }
    }
    return configs;
}

std::vector<EngineConfig> EngineCapabilities::detectWithProtocol(
    std::vector<EngineConfig>& configs,
    EngineProtocol protocol) 
{
    for (auto& config : configs) {
        auto mutableConfig = EngineWorkerFactory::getConfigManagerMutable()
            .getConfigMutableByCmdAndProtocol(config.getCmd(), config.getProtocol());
        config.setProtocol(protocol);
        mutableConfig->setProtocol(protocol);
    }
    
    auto engines = EngineWorkerFactory::createEngines(configs);
    
    std::vector<EngineConfig> failedConfigs;
    for (const auto& config : configs) {
        auto matchingEngine = std::ranges::find_if(engines,
            [&config](const std::unique_ptr<EngineWorker>& engine) {
                return engine->getConfig().getCmd() == config.getCmd();
            });
        
        if (matchingEngine == engines.end()) {
            failedConfigs.push_back(config);
        }
    }
    
    storeCapabilities(engines);
    
    return failedConfigs;
}

void EngineCapabilities::storeCapabilities(const std::vector<std::unique_ptr<EngineWorker>>& engines) {
    for (auto& engine : engines) {
        auto& command = engine->getConfig().getCmd();
        auto protocol = engine->getConfig().getProtocol();
        
        // Update the config manager with engine name and author
        auto config = EngineWorkerFactory::getConfigManagerMutable()
            .getConfigMutableByCmdAndProtocol(command, protocol);
        if (config && !engine->getEngineName().empty()) {
            config->setName(engine->getEngineName());
            config->setAuthor(engine->getEngineAuthor());
        }
        
        // Create and store capability
        EngineCapability capability;
        capability.setPath(command);
        capability.setProtocol(protocol);
        capability.setName(engine->getEngineName());
        capability.setAuthor(engine->getEngineAuthor());
        capability.setSupportedOptions(engine->getSupportedOptions());
        addOrReplace(capability);
    }
}

void EngineCapabilities::markAsNotSupported(const std::vector<EngineConfig>& failedConfigs) {
    std::string message = "Auto autodetection completed. Not supported Engine(s):\n";
    for (const auto& config : failedConfigs) {
        auto mutableConfig = EngineWorkerFactory::getConfigManagerMutable()
            .getConfigMutableByCmdAndProtocol(config.getCmd(), config.getProtocol());
        if (mutableConfig) {
            mutableConfig->setProtocol(EngineProtocol::NotSupported);
        }
        message += " - " + config.getCmd() + "\n";
    }
    SnackbarManager::instance().showWarning(message);
}

void EngineCapabilities::autoDetect() {
    std::thread([this]() {
        auto configs = collectMissingCapabilities();
        if (configs.empty()) {
            SnackbarManager::instance().showNote("No new engines found.");
            return;
        }
        detecting_ = true;
        SnackbarManager::instance().showNote("Starting engine autodetection.\nThis may take a while...");

        // Detect using UCI protocol first then xboard as uci is more common
        for (const auto protocol : {EngineProtocol::Uci, EngineProtocol::XBoard}) {
            configs = detectWithProtocol(configs, protocol);
            if (configs.empty()) {
                break;
            }
        }
        if (!configs.empty()) {
            markAsNotSupported(configs);
        } else {
            SnackbarManager::instance().showSuccess("Engine autodetection completed.");
        }
        detecting_ = false;
    }).detach();
}

bool EngineCapabilities::areAllEnginesDetected() const {
    for (const auto& config : EngineWorkerFactory::getConfigManager().getAllConfigs()) {
        if (!hasAnyCapability(config.getCmd(), config.getProtocol())) {
            return false;
        }
    }
    return true;
}

