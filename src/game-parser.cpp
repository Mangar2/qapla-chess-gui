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
#include "game-state.h"
#include "pgn-io.h"

using QaplaTester::GameRecord;
using QaplaTester::GameState;
using QaplaTester::MoveRecord;
using QaplaTester::PgnIO;

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
    GameRecord cleanRecord = gameState.setFromGameRecordAndCopy(record, std::nullopt, false);

    // Check if either FEN is set or at least one move is present
    bool hasFen = !cleanRecord.getStartFen().empty() && cleanRecord.getStartFen() != "startpos";
    bool hasMoves = !cleanRecord.history().empty();

    if (!hasFen && !hasMoves) {
        return std::nullopt;
    }

    return cleanRecord;
}

/**
 * @brief Parses a UCI (Universal Chess Interface) string into a GameRecord.
 *
 * position fen <FEN string> moves <move1> <move2> ...
 * <FEN string> moves ... or just moves ... is also valid
 * 
 * @param input The UCI string to parse.
 * @return std::optional<GameRecord> The parsed GameRecord, or std::nullopt if parsing fails.
 */
std::optional<GameRecord> parseUCI(const std::string& input) {
    // Check that it is not a PGN string
    if (input.find('[') != std::string::npos && input.find(']') != std::string::npos) {
        return std::nullopt;
    }
    // Note: it will also return std::nullopt, if the move string starts with a number with dot
    // as the moves scanned here are expected to be in LAN or SAN format without move numbers.

    auto fenGame = parseFen(input);
    auto movesPos = input.find("moves ");
    std::string movesString;
    if (movesPos != std::string::npos) {
        movesString = input.substr(movesPos + 6);
    } else {
        movesString = input;
    }
    GameState gameState;
    GameRecord result;
    if (fenGame) {
        gameState.setFen(false, fenGame->getStartFen());
        result = *fenGame;
    } else {
        gameState.setFen(true);
    }
    // iterate over moves
    std::istringstream moveStream(movesString);
    std::string moveStr;
    while (moveStream >> moveStr) {
        auto move = gameState.stringToMove(moveStr, false);
        if (move.isEmpty()) {
            break;
        }
        MoveRecord moveRecord;
        moveRecord.lan_ = move.getLAN();
        moveRecord.san_ = gameState.moveToSan(move);
        moveRecord.original = moveStr;
        gameState.doMove(move);
        result.addMove(moveRecord);
    }
    if (result.getStartFen().empty() && result.history().empty()) {
        return std::nullopt;
    }

    return result;
}

// ================================================================================================
// GameParser Implementation
// ================================================================================================

GameParser::GameParser() {
    // Register default parsers
    // The ordering is important as first successful parser will be used. E.g. then FEN 
    // parser is often also successful for UCI or PGN inputs.
    addParser("UCI", parseUCI);
    addParser("PGN", parsePGN);
    addParser("FEN", parseFen);
}

void GameParser::addParser(const std::string& name, const ParserFunction& parser) {
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