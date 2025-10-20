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

#pragma once
#include "engine-capability.h"

#include "qapla-tester/ini-file.h"

#include <unordered_map>
#include <string>
#include <ostream>
#include <optional>
#include <atomic>

namespace QaplaConfiguration {
    /**
     * @class EngineCapabilities
     * @brief Manages a collection of EngineCapability objects, providing methods to save, add, and retrieve capabilities.
     */
    class EngineCapabilities {
    public:
        /**
         * @brief Saves all engine capabilities to a stream in INI format.
         * @param out The output stream to write the data to.
         */
        void save(std::ostream& out) const {
            for (const auto& [key, capability] : capabilities_) {
                capability.save(out);
            }
        }

        /**
         * @brief Adds or replaces an EngineCapability in the collection.
         * @param capability The EngineCapability to add or replace.
         */
        void addOrReplace(const EngineCapability& capability) {
            auto key = makeKey(capability.getPath(), capability.getProtocol());
            capabilities_[key] = capability;
        }

        /**
         * @brief Adds or replaces an EngineCapability using a key-value map.
         * @param keyValueMap A map containing key-value pairs to initialize the EngineCapability.
         * @throws std::invalid_argument if the map is invalid or missing required keys.
         */
        void addOrReplace(const QaplaHelpers::IniFile::Section& section) {
            EngineCapability capability = EngineCapability::createFromSection(section);
            addOrReplace(capability);
        }

        /**
         * @brief Deletes an EngineCapability based on its path and protocol.
         * @param path The path to the engine executable.
         * @param protocol The protocol used by the engine.
         */
        void deleteCapability(const std::string& path, QaplaTester::EngineProtocol protocol) {
            auto key = makeKey(path, protocol);
            capabilities_.erase(key);
        }

        /**
         * @brief Retrieves an EngineCapability based on its path and protocol.
         * @param path The path to the engine executable.
         * @param protocol The protocol used by the engine.
         * @return An optional EngineCapability if found.
         */
        const std::optional<EngineCapability> getCapability(const std::string& path, QaplaTester::EngineProtocol protocol) const {
            auto key = makeKey(path, protocol);
            auto it = capabilities_.find(key);
            if (it != capabilities_.end()) {
                return it->second;
            } 
            return std::nullopt;
        }

        /**
         * @brief Checks if any capability exists for a given path.
         * @param path The path to check for capabilities.
         * @return True if any capability exists for the given path, false otherwise.
         */
        const bool hasAnyCapability(const std::string& path) const {
            for (const auto& [key, capability] : capabilities_) {
                if (capability.getPath() == path) {
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Detects missing engine configurations.
         *
         * This function attempts to start engines using the file paths and reads their parameters
         * (e.g., supported options, protocol, and other metadata) as well as their names for
         * configurations where the engine name is not set.
         *
         */
        void autoDetect();

        /**
         * @brief Checks if detection is currently in progress.
         * @return True if detection is ongoing, false otherwise.
         */
        bool isDetecting() const {
            return detecting_.load();
        }

        /**
         * @brief Checks if all configured engines have been detected.
         * @return True if all engines have capabilities, false otherwise.
         */
        bool areAllEnginesDetected() const;

    private:
        /**
         * @brief Creates a unique key for the unordered_map based on path and protocol.
         * @param path The path to the engine executable.
         * @param protocol The protocol used by the engine.
         * @return A string key combining path and protocol.
         */
        static std::string makeKey(const std::string& path, QaplaTester::EngineProtocol protocol) {
            return path + "|" + to_string(protocol);
        }

        std::unordered_map<std::string, EngineCapability> capabilities_; ///< Stores EngineCapability objects indexed by a unique key.
        std::atomic<bool> detecting_; ///< True, if detection is currently in progress.
    };

}