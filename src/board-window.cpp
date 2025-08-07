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
#include "board-window.h"
#include "qapla-engine/types.h"
#include "qapla-tester/game-state.h"
#include "qapla-tester/game-record.h"
#include "font.h"

#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace QaplaWindows {

    static const char8_t* pieceSymbol(QaplaBasics::Piece p) {
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

    static const char8_t* pieceBackground(QaplaBasics::Piece p) {
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
            /*
        case WhiteBishop: return (u8"");
        case WhiteRook:   return (u8"");
        case WhiteQueen:  return (u8"");
        case WhiteKing:   return (u8"");
        case BlackPawn:   return (u8"");
        case BlackKnight: return (u8"");
        case BlackBishop: return (u8"");
        case BlackRook:   return (u8"");
        case BlackQueen:  return (u8"");
        case BlackKing:   return (u8"");
        */
        }
        return u8" ";
    }

    std::pair<ImVec2, ImVec2> BoardWindow::computeCellCoordinates(const ImVec2& boardPos, float cellSize, 
        QaplaBasics::File file, QaplaBasics::Rank rank) {
        float cellY = boardInverted_ ?
            static_cast<float>(rank) : static_cast<float>(QaplaBasics::Rank::R8 - rank);

        const ImVec2 cellMin = {
            boardPos.x + static_cast<float>(file) * cellSize,
            boardPos.y + cellY * cellSize
        };
        const ImVec2 cellMax = { cellMin.x + cellSize, cellMin.y + cellSize };
        return { cellMin, cellMax };
    }

    void BoardWindow::drawPromotionPopup(float cellSize)
    {
        using QaplaBasics::Piece;
        constexpr float SHRINK_CELL_SIZE = 0.8f;
		auto& gameState = boardData_->gameState();

        if (!selectedTo_)
            return;

        const bool whiteToMove = gameState.position().isWhiteToMove();
        const Piece pieces[4] = {
            whiteToMove ? Piece::WHITE_QUEEN : Piece::BLACK_QUEEN,
            whiteToMove ? Piece::WHITE_ROOK : Piece::BLACK_ROOK,
            whiteToMove ? Piece::WHITE_BISHOP : Piece::BLACK_BISHOP,
            whiteToMove ? Piece::WHITE_KNIGHT : Piece::BLACK_KNIGHT
        };

        cellSize = std::max(30.0f, cellSize * SHRINK_CELL_SIZE);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();
        ImGui::PushFont(font::chessFont);

        const ImVec2 startPos = ImGui::GetCursorScreenPos();

        for (int i = 0; i < 4; ++i) {
            const ImVec2 cellMin = {
                startPos.x + i * cellSize,
                startPos.y
            };
            const ImVec2 cellMax = {
                cellMin.x + cellSize,
                cellMin.y + cellSize
            };

            ImGui::SetCursorScreenPos(cellMin);
            ImGui::InvisibleButton(("promo_" + std::to_string(i)).c_str(), ImVec2(cellSize, cellSize));

            if (ImGui::IsItemClicked()) {
                boardData_->addMove(selectedFrom_, selectedTo_, pieces[i]);

                selectedFrom_.reset();
                selectedTo_.reset();
                promotionPending_ = false;
                ImGui::CloseCurrentPopup();
            }

            const ImU32 bgColor = IM_COL32(240, 217, 181, 255);
            drawList->AddRectFilled(cellMin, cellMax, bgColor);
            drawPiece(drawList, pieces[i], cellMin, cellSize, font);
        }

        ImGui::PopFont();
    }


    void BoardWindow::drawBoardSquare(ImDrawList* drawList, const ImVec2& boardPos, 
        float cellSize, QaplaBasics::File file, QaplaBasics::Rank rank, bool isWhite)
    {
        using QaplaBasics::Square;
        using QaplaBasics::Piece;
        auto& gameState = boardData_->gameState();

        const Square square = computeSquare(file, rank);
        const Piece piece = gameState.position()[square];

		const auto [cellMin, cellMax] = computeCellCoordinates(boardPos, cellSize, file, rank);

        const bool isSelected = (selectedFrom_ && *selectedFrom_ == square) || (selectedTo_ && *selectedTo_ == square);
        const ImU32 bgColor = isSelected
            ? IM_COL32(100, 149, 237, 255)
            : (isWhite ? IM_COL32(240, 217, 181, 255) : IM_COL32(181, 136, 99, 255));

        drawList->AddRectFilled(cellMin, cellMax, bgColor);

        ImGui::SetCursorScreenPos(cellMin);
        ImGui::InvisibleButton(("cell_" + std::to_string(square)).c_str(), ImVec2(cellSize, cellSize));

        if (ImGui::IsItemClicked() && !boardData_->isGameOver()) {
			bool wtm = gameState.isWhiteToMove();
            if (piece != Piece::NO_PIECE && getPieceColor(piece) == (wtm ? Piece::WHITE : Piece::BLACK)) {
                selectedFrom_ = square;
            }
            else {
                selectedTo_ = square;
            }

            if (selectedTo_) {
                const auto [partial, promotion] = boardData_->addMove(selectedFrom_, selectedTo_, QaplaBasics::Piece::NO_PIECE);
                if (promotion) {
                    promotionPending_ = true;
                    ImGui::OpenPopup((id_ + "Promotion").c_str());
                } else if (!partial) {
                    selectedFrom_.reset();
                    selectedTo_.reset();
                }
            }
        }
    }

    void BoardWindow::drawBoardSquares(ImDrawList* drawList, const ImVec2& boardPos, float cellSize)
    {
        using QaplaBasics::Rank;
        using QaplaBasics::File;

        bool isWhite = true;
        for (Rank rank = Rank::R1; rank <= Rank::R8; ++rank) {
            for (File file = File::A; file <= File::H; ++file) {
                drawBoardSquare(drawList, boardPos, cellSize, file, rank, isWhite);
                isWhite = !isWhite;
            }
            isWhite = !isWhite;
        }
    }

    void BoardWindow::drawPiece(ImDrawList* drawList, QaplaBasics::Piece piece, 
        const ImVec2& cellMin, float cellSize, ImFont* font)
    {
        if (piece == QaplaBasics::Piece::NO_PIECE)
            return;

        const float fontSize = cellSize * 0.9f;
        const char* text = reinterpret_cast<const char*>(pieceSymbol(piece));
        const char* background = reinterpret_cast<const char*>(pieceBackground(piece));
        const ImVec2 textSize = font->CalcTextSizeA(fontSize, cellSize, -1.0f, text);
        const ImVec2 textPos = {
            cellMin.x + (cellSize - textSize.x) * 0.5f,
            cellMin.y + (cellSize - textSize.y) * 0.5f
        };

        drawList->AddText(font, fontSize, textPos, IM_COL32_WHITE, background);
        drawList->AddText(font, fontSize, textPos, IM_COL32_BLACK, text);
    }

    void BoardWindow::drawBoardPieces(ImDrawList* drawList, const ImVec2& boardPos, float cellSize, ImFont* font)
    {
        using QaplaBasics::Rank;
        using QaplaBasics::File;
        using QaplaBasics::Square;
        using QaplaBasics::Piece;
        auto& gameState = boardData_->gameState();

        for (Rank rank = Rank::R1; rank <= Rank::R8; ++rank) {
            for (File file = File::A; file <= File::H; ++file) {
                Square sq = computeSquare(file, rank);
                Piece piece = gameState.position()[sq];
                const auto [cellMin, _] = computeCellCoordinates(boardPos, cellSize, file, rank);
                drawPiece(drawList, piece, cellMin, cellSize, font);
            }
        }
    }

    void BoardWindow::drawBoardCoordinates(ImDrawList* drawList,
        const ImVec2& boardPos, float cellSize, float boardSize, ImFont* font, float maxSize)
    {
        const int gridSize = 8;

        for (int col = 0; col < gridSize; ++col) {
            std::string label(1, 'a' + col);
            ImVec2 pos = {
                boardPos.x + col * cellSize + cellSize * 0.5f - ImGui::CalcTextSize(label.c_str()).x * 0.5f,
                boardPos.y + boardSize
            };
            drawList->AddText(font, std::min(cellSize * 0.5f, maxSize), pos, IM_COL32_WHITE, label.c_str());
        }

        for (int row = 0; row < gridSize; ++row) {
            std::string label(1, '8' - row);
            ImVec2 pos = {
                boardPos.x + boardSize,
                boardPos.y + row * cellSize + cellSize * 0.3f
            };
            drawList->AddText(font, std::min(cellSize * 0.5f, maxSize), pos, IM_COL32_WHITE, label.c_str());
        }
    }

    void BoardWindow::draw()
    {
        constexpr float maxBorderTextSize = 30.0f;
        constexpr int gridSize = 8;

        //ImGui::SetNextWindowSizeConstraints(ImVec2(150, 150), ImVec2(static_cast<float>(width), static_cast<float>(height)));

        const ImVec2 region = ImGui::GetContentRegionAvail();
        if (region.x <= 0.0f || region.y <= 0.0f) {
            return;
        }

        ImGui::PushFont(font::chessFont);

        const float cellSize = std::floor(std::min(region.x, region.y) / gridSize) * 0.95f;

        if (promotionPending_) {
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1.0f, 1.0f, 1.0f, 0.3f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
            if (ImGui::BeginPopup((id_ + "Promotion").c_str())) {
                drawPromotionPopup(cellSize);
                ImGui::EndPopup();
            }
            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(1);
        }

        const float boardSize = cellSize * gridSize;
        ImVec2 boardPos = ImGui::GetCursorScreenPos();
        boardPos.x += 3;
        boardPos.y += 3;
        const ImVec2 boardEnd = { boardPos.x + boardSize, boardPos.y + boardSize };
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();

        drawBoardSquares(drawList, boardPos, cellSize);
        drawBoardPieces(drawList, boardPos, cellSize, font);
        drawBoardCoordinates(drawList, boardPos, cellSize, boardSize, font, maxBorderTextSize);

        const float coordTextHeight = std::min(cellSize * 0.5f, maxBorderTextSize);
        ImGui::Dummy(ImVec2(boardSize, boardSize + coordTextHeight));

        ImGui::PopFont();
    }
   
}
