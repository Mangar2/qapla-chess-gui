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

#include <array>
#include <optional>

namespace board {

    /**
     * @brief Figurentypen auf dem Schachbrett.
     */
    enum class Piece {
        WhitePawn, WhiteKnight, WhiteBishop, WhiteRook, WhiteQueen, WhiteKing,
        BlackPawn, BlackKnight, BlackBishop, BlackRook, BlackQueen, BlackKing
    };

    /**
     * @brief 8x8 Schachbrett mit optionalen Figuren.
     */
    using Board = std::array<std::array<std::optional<Piece>, 8>, 8>;

    /**
     * @brief Gibt das aktuelle Brett zurück (Testaufstellung).
     */
    Board createTestBoard();

}
