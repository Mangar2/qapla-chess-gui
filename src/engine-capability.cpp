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

#include "engine-capability.h"

#include "qapla-tester/engine-option.h"
#include "qapla-tester/string-helper.h"

#include <sstream>
#include <stdexcept>

 /**
  * @brief Converts an EngineOption to a JSON-Line formatted string.
  * @param option The EngineOption to convert.
  * @return A JSON-Line formatted string representing the EngineOption.
  */
static std::string to_string(const EngineOption& option) {
    std::string json = "{";

    // Add name
    json += "\"name\": \"" + option.name + "\", ";

    // Add type
    json += "\"type\": \"" + EngineOption::to_string(option.type) + "\", ";

    // Add defaultValue
    if (!option.defaultValue.empty()) {
        json += "\"defaultValue\": \"" + option.defaultValue + "\", ";
    }

    // Add min (if present)
    if (option.min.has_value()) {
        json += "\"min\": " + std::to_string(option.min.value()) + ", ";
    }

    // Add max (if present)
    if (option.max.has_value()) {
        json += "\"max\": " + std::to_string(option.max.value()) + ", ";
    }

    // Add vars (if not empty)
    if (!option.vars.empty()) {
        json += "\"vars\": [";
        for (size_t i = 0; i < option.vars.size(); ++i) {
            json += "\"" + option.vars[i] + "\"";
            if (i < option.vars.size() - 1) {
                json += ", ";
            }
        }
        json += "]";
    }

    json += "}";
    return json;
}

/**
 * @brief Parses a JSON-Line formatted string to create an EngineOption object.
 * @param json The JSON-Line formatted string.
 * @return An EngineOption object created from the string.
 * @throws std::invalid_argument if the string is not properly formatted or contains invalid data.
 */
static EngineOption parseEngineOption(const std::string& json) {
    EngineOption option;
    std::istringstream stream(json);
    std::string token;


    // Ensure the string starts and ends with curly braces
    if (json.front() != '{' || json.back() != '}') {
        throw std::invalid_argument("Invalid JSON format: Missing opening or closing braces.");
    }

    // Remove the outer braces
    std::string content = json.substr(1, json.size() - 2);

    // Parse key-value pairs
    std::istringstream contentStream(content);
    while (std::getline(contentStream, token, ',')) {
        size_t colonPos = token.find(':');
        if (colonPos == std::string::npos) {
            throw std::invalid_argument("Invalid JSON format: Missing colon in key-value pair.");
        }

        std::string key = trim(token.substr(0, colonPos));
        std::string value = trim(token.substr(colonPos + 1));

        if (key == "name") {
            option.name = value;
        }
        else if (key == "type") {
            option.type = EngineOption::parseType(value);
        }
        else if (key == "defaultValue") {
            option.defaultValue = value;
        }
        else if (key == "min") {
            option.min = std::stoi(value);
        }
        else if (key == "max") {
            option.max = std::stoi(value);
        }
        else if (key == "vars") {
            if (value.front() != '[' || value.back() != ']') {
                throw std::invalid_argument("Invalid JSON format: 'vars' must be an array.");
            }
            std::string varsContent = value.substr(1, value.size() - 2);
            std::istringstream varsStream(varsContent);
            std::string var;
            while (std::getline(varsStream, var, ',')) {
                option.vars.push_back(trim(var));
            }
        }
        else {
            throw std::invalid_argument("Unknown key in JSON: " + key);
        }
    }

    return option;
}

/**
 * @brief Saves the engine capability data to a stream in INI format.
 * @param out The output stream to write the data to.
 */
void EngineCapability::save(std::ostream& out) const {
    // Write the section header
    out << "[enginecapability]" << std::endl;

    // Write the path
    out << "path=" << path_ << std::endl;

    // Write the protocol
    out << "protocol=" << to_string(protocol_) << std::endl;

    // Write the name
    out << "name=" << name_ << std::endl;

    // Write the author
    out << "author=" << author_ << std::endl;

    // Write the supported options
    for (const auto& option : supportedOptions_) {
        out << "option=" << to_string(option) << std::endl;
    }
	// Add a blank line at the end for separation
    out << "\n";
}

EngineCapability EngineCapability::createFromKeyValueMap(const std::unordered_map<std::string, std::string>& keyValueMap) {
    EngineCapability capability;

    for (const auto& [key, value] : keyValueMap) {
        if (key == "path") {
            if (value.empty()) {
                throw std::invalid_argument("The 'path' value cannot be empty.");
            }
            capability.setPath(value);
        }
        else if (key == "protocol") {
            try {
                capability.setProtocol(parseEngineProtocol(value));
            }
            catch (const std::exception&) {
                throw std::invalid_argument("Invalid 'protocol' value: " + value);
            }
        }
        else if (key == "name") {
            capability.setName(value);
        }
        else if (key == "author") {
            capability.setAuthor(value);
        }
        else if (key == "option") {
            try {
                capability.supportedOptions_.push_back(parseEngineOption(value));
            }
            catch (const std::exception&) {
                // Ignore invalid options
                continue;
            }
        }
        else {
            // Ignore unknown keys
            continue;
        }
    }

    // Ensure required fields are set
    if (capability.getPath().empty()) {
        throw std::invalid_argument("Missing required 'path'.");
    }
    if (capability.getProtocol() == EngineProtocol::Unknown) {
        throw std::invalid_argument("Missing required 'protocol'.");
    }

    return capability;
}
