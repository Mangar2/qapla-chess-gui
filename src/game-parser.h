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
#include <optional>
#include <functional>
#include <vector>

#include "game-record.h"

namespace QaplaUtils {

/**
 * @brief Type alias for a parser function
 * 
 * A parser function takes a string input and optionally returns a GameRecord
 */
using ParserFunction = std::function<std::optional<QaplaTester::GameRecord>(const std::string&)>;

/**
 * @brief Main game parser that manages parser functions
 * 
 * This class coordinates different chess format parser functions and attempts to parse
 * input strings using the registered parsers in order.
 */
class GameParser {
public:
    /**
     * @brief Default constructor that registers standard parsers
     */
    GameParser();

    /**
     * @brief Registers a new chess format parser function
     * 
     * @param name Name of the parser (for debugging/feedback)
     * @param parser Parser function that takes string and returns optional GameRecord
     */
    void addParser(const std::string& name, const ParserFunction& parser);

    /**
     * @brief Attempts to parse input string using all registered parsers
     * 
     * Tries parsers in registration order until one succeeds or all fail.
     * 
     * @param input The input string to parse
     * @return std::optional<GameRecord> A GameRecord if any parser succeeded, std::nullopt otherwise
     */
    std::optional<QaplaTester::GameRecord> parse(const std::string& input);

    /**
     * @brief Gets the name of the parser that last successfully parsed input
     * 
     * @return std::string Name of the last successful parser, empty if none succeeded
     */
    std::string getLastSuccessfulParser() const { return lastSuccessfulParser_; }

private:
    std::vector<std::pair<std::string, ParserFunction>> parsers_;
    std::string lastSuccessfulParser_;
};

/**
 * @brief FEN parser function
 * 
 * Attempts to find and parse FEN strings from input text using a sliding window approach.
 * 
 * @param input The input string to parse
 * @return std::optional<GameRecord> A GameRecord if FEN was found and parsed successfully
 */
std::optional<QaplaTester::GameRecord> parseFen(const std::string& input);

} // namespace QaplaUtils