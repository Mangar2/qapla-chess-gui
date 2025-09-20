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

#include "game-parser.h"
#include "qapla-engine/fenscanner.h"
#include "qapla-engine/movegenerator.h"
#include "qapla-tester/game-state.h"
#include "qapla-tester/pgn-io.h"

#include <algorithm>

namespace QaplaUtils {

// ================================================================================================
// FEN Parser Function
// ================================================================================================

std::optional<GameRecord> parseFen(const std::string& input) {
    if (input.empty()) {
        return std::nullopt;
    }

    // Simple sliding window: try FenScanner::setBoard starting from each position
    const size_t maxSearchLength = 1000;
    size_t searchLength = std::min(input.length(), maxSearchLength);
    
    for (size_t startPos = 0; startPos < searchLength; ++startPos) {
        std::string candidateString = input.substr(startPos);
        
        QaplaInterface::FenScanner scanner;
        QaplaMoveGenerator::MoveGenerator position;
        
        if (scanner.setBoard(candidateString, position)) {
            // FenScanner succeeded! Create GameRecord from this position
            try {
                GameState gameState;
                gameState.setFen(false, candidateString);
                
                GameRecord gameRecord;
                gameRecord.setStartPosition(
                    false,                              // Not standard start position
                    candidateString,                    // FEN string
                    gameState.isWhiteToMove(),          // Who to move
                    gameState.getStartHalfmoves()       // Half-move clock
                );
                
                return gameRecord;
            } catch (...) {
                // Continue searching if GameState creation fails
                continue;
            }
        }
    }

    return std::nullopt;
}

// ================================================================================================
// PGN Parser Function
// ================================================================================================

std::optional<GameRecord> parsePGN(const std::string& input) {
    if (input.empty()) {
        return std::nullopt;
    }

    // Parse the PGN string using PgnIO
    GameRecord record = PgnIO::parseGame(input);

    // Create a clean copy using GameState
    GameState gameState;
    GameRecord cleanRecord = gameState.setFromGameRecordAndCopy(record);

    // Check if either FEN is set or at least one move is present
    bool hasFen = !cleanRecord.getStartFen().empty() && cleanRecord.getStartFen() != "startpos";
    bool hasMoves = !cleanRecord.history().empty();

    if (!hasFen && !hasMoves) {
        return std::nullopt;
    }

    return cleanRecord;
}

// ================================================================================================
// GameParser Implementation
// ================================================================================================

GameParser::GameParser() {
    // Register default parsers
    addParser("PGN", parsePGN);
    addParser("FEN", parseFen);
    
    // Future parsers can be added here:
    // addParser("UCI", parseUci);
}

void GameParser::addParser(const std::string& name, ParserFunction parser) {
    if (parser) {
        parsers_.emplace_back(name, parser);
    }
}

std::optional<GameRecord> GameParser::parse(const std::string& input) {
    lastSuccessfulParser_.clear();
    
    if (input.empty()) {
        return std::nullopt;
    }

    // Try each parser in registration order
    for (const auto& [name, parser] : parsers_) {
        auto result = parser(input);
        if (result.has_value()) {
            lastSuccessfulParser_ = name;
            return result;
        }
    }

    return std::nullopt;
}

} // namespace QaplaUtils