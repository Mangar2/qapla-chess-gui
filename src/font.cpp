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

#include "font.h"
#include "ui/qapla-ui.h"
#include <stdexcept>
#include <filesystem>

namespace FontManager {

    const char8_t* pieceSymbol(QaplaBasics::Piece piece) {
        using enum QaplaBasics::Piece;
        switch (piece) {
        case WHITE_PAWN:   return (u8"♙");
        case WHITE_KNIGHT: return (u8"♘");
        case WHITE_BISHOP: return (u8"♗");
        case WHITE_ROOK:   return (u8"♖");
        case WHITE_QUEEN:  return (u8"♕");
        case WHITE_KING:   return (u8"♔");
        case BLACK_PAWN:   return (u8"♟");
        case BLACK_KNIGHT: return (u8"♞");
        case BLACK_BISHOP: return (u8"♝");
        case BLACK_ROOK:   return (u8"♜");
        case BLACK_QUEEN:  return (u8"♛");
        case BLACK_KING:   return (u8"♚");
        case NO_PIECE:     return (u8" ");
        case PIECE_AMOUNT: return (u8" ");
        case BLACK: return (u8" ");
        default: return (u8"");
        }
    }

    const char8_t* pieceBackground(QaplaBasics::Piece piece) {
        using enum QaplaBasics::Piece;
        switch (piece) {
        case WHITE_PAWN:   return (u8"");
        case WHITE_KNIGHT:  return (u8"");
        case WHITE_BISHOP: return (u8"");
        case WHITE_ROOK:   return (u8"");
        case WHITE_QUEEN:  return (u8"");
        case WHITE_KING:   return (u8"");
        case BLACK_PAWN:   return (u8"");
        case BLACK_KNIGHT: return (u8"");
        case BLACK_BISHOP: return (u8"");
        case BLACK_ROOK:   return (u8"");
        case BLACK_QUEEN:  return (u8"");
        case BLACK_KING:   return (u8"");
        case NO_PIECE:     return (u8" ");
        case PIECE_AMOUNT: return (u8" ");
        case BLACK: return (u8" ");
        default: return (u8"");
        }
        return u8" ";
    }

    void drawPiece(QaplaUi::DrawList* drawList, QaplaBasics::Piece piece, QaplaUi::Font font)
    {
        if (piece == QaplaBasics::Piece::NO_PIECE) {
            return;
        }

        const float fontSize = QaplaUi::getFontSize();
        const char* text = reinterpret_cast<const char*>(FontManager::pieceSymbol(piece));
        const char* background = reinterpret_cast<const char*>(FontManager::pieceBackground(piece));
        const QaplaUi::Vec2 textPos = QaplaUi::getCursorScreenPos();

        drawList->addText(font, fontSize, textPos, QaplaUi::COL32_WHITE, background);
        drawList->addText(font, fontSize, textPos, QaplaUi::COL32_BLACK, text);
    }

    void drawPiece(QaplaUi::DrawList* drawList, QaplaBasics::Piece piece,
        const QaplaUi::Vec2& cellMin, float cellSize, QaplaUi::Font font)
    {
        if (piece == QaplaBasics::Piece::NO_PIECE) {
            return;
        }

        const float fontSize = cellSize * 0.9F;
        const char* text = reinterpret_cast<const char*>(FontManager::pieceSymbol(piece));
        const char* background = reinterpret_cast<const char*>(FontManager::pieceBackground(piece));
        const QaplaUi::Vec2 textSize = QaplaUi::calcTextSize(font, fontSize, text);
        const QaplaUi::Vec2 textPos = {
            cellMin.x + (cellSize - textSize.x) * 0.5F,
            cellMin.y + (cellSize - textSize.y) * 0.5F
        };

        drawList->addText(font, fontSize, textPos, QaplaUi::COL32_WHITE, background);
        drawList->addText(font, fontSize, textPos, QaplaUi::COL32_BLACK, text);
    }

}
