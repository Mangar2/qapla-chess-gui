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

#include <string>
#include <unordered_map>
#include "qapla-tester/engine-option.h"

 /**
  * @class EngineCapability
  * @brief Represents the capabilities of a chess engine, including its path, protocol, name, author, and supported options.
  */
class EngineCapability {
public:
    /**
     * @brief Default constructor.
     */
    EngineCapability() = default;

    /**
     * @brief Gets the path to the engine executable.
     * @return The path as a string.
     */
    const std::string& getPath() const {
		return path_;
    }

    /**
     * @brief Sets the path to the engine executable.
     * @param path The path as a string.
     */
    void setPath(const std::string& path) {
        path_ = path;
	}

    /**
     * @brief Gets the protocol used by the engine.
     * @return The protocol as an EngineProtocol enum.
     */
    EngineProtocol getProtocol() const {
        return protocol_;
	}

    /**
     * @brief Sets the protocol used by the engine.
     * @param protocol The protocol as an EngineProtocol enum.
     */
    void setProtocol(EngineProtocol protocol) {
		protocol_ = protocol;
	}

    /**
     * @brief Gets the name of the engine.
     * @return The name as a string.
     */
    const std::string& getName() const {
		return name_;
    }

    /**
     * @brief Sets the name of the engine.
     * @param name The name as a string.
     */
    void setName(const std::string& name) {
		name_ = name;
	}

    /**
     * @brief Gets the author of the engine.
     * @return The author as a string.
     */
    const std::string& getAuthor() const {
		return author_;
    }

    /**
     * @brief Sets the author of the engine.
     * @param author The author as a string.
     */
    void setAuthor(const std::string& author) {
		author_ = author;
    }

    /**
     * @brief Gets the supported options of the engine.
     * @return A vector of EngineOption objects.
     */
    const EngineOptions& getSupportedOptions() const {
        return supportedOptions_;
	}

    /**
     * @brief Sets the supported options of the engine.
     * @param options A vector of EngineOption objects.
     */
    void setSupportedOptions(const EngineOptions& options) {
		supportedOptions_ = options;
	}

    /**
     * @brief Saves the engine capability data to a stream in INI format.
     * @param out The output stream to write the data to.
     */
    void save(std::ostream& out) const;

    /**
     * @brief Creates an EngineCapability instance from a key-value map.
     * @param keyValueMap An unordered map containing key-value pairs to initialize the properties.
     * @return An EngineCapability instance initialized with the provided key-value pairs.
     * @throws std::invalid_argument if the command line (path) or protocol is missing or invalid.
     */
    static EngineCapability createFromKeyValueMap (const std::unordered_map<std::string, std::string>& keyValueMap);

private:
    std::string path_; ///< Path to the engine executable.
    EngineProtocol protocol_ = EngineProtocol::Unknown; ///< Protocol used by the engine.
    std::string name_;  ///< The name reported by the engine itself.
	std::string author_; ///< The author reported by the engine itself.
    EngineOptions supportedOptions_; ///< Supported options of the engine.
};

