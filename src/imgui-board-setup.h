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

namespace QaplaWindows
{
    /**
     * @brief Setup data structure for board position configuration.
     * 
     * Contains all FEN-related setup parameters that are not directly
     * represented by piece placement on the board.
     */
    struct BoardSetupData
    {
        bool whiteToMove = true;
        bool whiteKingsideCastle = true;
        bool whiteQueensideCastle = true;
        bool blackKingsideCastle = true;
        bool blackQueensideCastle = true;
        std::string enPassantSquare = "-";
        int fullmoveNumber = 1;
        int halfmoveClock = 0;
    };

    /**
     * @brief Input controls for board setup mode.
     * 
     * Provides UI controls for configuring additional setup parameters
     * beyond piece placement. Displays side to move, castling rights,
     * en passant square, and move counters.
     */
    class ImGuiBoardSetup
    {
    public:
        /**
         * Draw the setup controls.
         * @param data The setup data to display and modify.
         * @return True if any value was modified, false otherwise.
         */
        static bool draw(BoardSetupData& data);
    };

} // namespace QaplaWindows
