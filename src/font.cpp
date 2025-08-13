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
#include "chess-font.h"
#include "inter-variable.h"
#include <imgui.h>
#include <stdexcept>
#include <filesystem>

namespace font {

    const char8_t* pieceSymbol(QaplaBasics::Piece p) {
        using enum QaplaBasics::Piece;
        switch (p) {
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

    const char8_t* pieceBackground(QaplaBasics::Piece p) {
        using enum QaplaBasics::Piece;
        switch (p) {
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

    void drawPiece(ImDrawList* drawList, QaplaBasics::Piece piece,
        const ImVec2& cellMin, float cellSize, ImFont* font)
    {
        if (piece == QaplaBasics::Piece::NO_PIECE)
            return;

        const float fontSize = cellSize * 0.9f;
        const char* text = reinterpret_cast<const char*>(font::pieceSymbol(piece));
        const char* background = reinterpret_cast<const char*>(font::pieceBackground(piece));
        const ImVec2 textSize = font->CalcTextSizeA(fontSize, cellSize, -1.0f, text);
        const ImVec2 textPos = {
            cellMin.x + (cellSize - textSize.x) * 0.5f,
            cellMin.y + (cellSize - textSize.y) * 0.5f
        };

        drawList->AddText(font, fontSize, textPos, IM_COL32_WHITE, background);
        drawList->AddText(font, fontSize, textPos, IM_COL32_BLACK, text);
    }

    void loadFonts() {
        auto& io = ImGui::GetIO();

        io.Fonts->AddFontDefault(); // Default UI font
        //chessFont = io.Fonts->AddFontFromFileTTF(path.c_str(), size);
        ImFontConfig fontCfg;
        fontCfg.FontDataOwnedByAtlas = false;

        chessFont = io.Fonts->AddFontFromMemoryTTF(
            reinterpret_cast<void*>(const_cast<uint32_t*>(chessFontData)),
            static_cast<int>(chessFontSize),
            32,
            &fontCfg
        );

        interVariable = io.Fonts->AddFontFromMemoryTTF(
            reinterpret_cast<void*>(const_cast<uint32_t*>(interVariableData)),
            static_cast<int>(interVariableSize),
            16,
            &fontCfg
        );

        io.FontDefault = io.Fonts->Fonts[2]; 
    }

}

