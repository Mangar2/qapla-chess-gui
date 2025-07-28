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

#include "board.h"

namespace board {

    Board createTestBoard() {
        Board b{};

        // Beispielaufstellung: nur Bauernreihe und Könige
        for (int col = 0; col < 8; ++col) {
            b[1][col] = Piece::WhitePawn;
            b[6][col] = Piece::BlackPawn;
        }

        b[0][0] = Piece::WhiteRook;
        b[0][1] = Piece::WhiteKnight;
		b[0][2] = Piece::WhiteBishop;
		b[0][3] = Piece::WhiteQueen;
        b[0][4] = Piece::WhiteKing;
		b[0][5] = Piece::WhiteBishop;
		b[0][6] = Piece::WhiteKnight;
		b[0][7] = Piece::WhiteRook;
		b[7][0] = Piece::BlackRook;
		b[7][1] = Piece::BlackKnight;
		b[7][2] = Piece::BlackBishop;
		b[7][3] = Piece::BlackQueen;
        b[7][4] = Piece::BlackKing;
		b[7][5] = Piece::BlackBishop;
		b[7][6] = Piece::BlackKnight;
		b[7][7] = Piece::BlackRook;

        return b;
    }

}

