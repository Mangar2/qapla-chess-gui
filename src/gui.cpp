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
#include "gui.h"
#include "board.h"
#include "font.h"

#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "gui.h"
#include "board.h"

#include <imgui.h>
#include <array>

namespace gui {

    namespace {

        const char8_t* pieceSymbol(board::Piece p) {
            using enum board::Piece;
            switch (p) {
            case WhitePawn:   return (u8"♙");
            case WhiteKnight: return (u8"♘");
            case WhiteBishop: return (u8"♗");
            case WhiteRook:   return (u8"♖");
            case WhiteQueen:  return (u8"♕");
            case WhiteKing:   return (u8"♔");
            case BlackPawn:   return (u8"♟");
            case BlackKnight: return (u8"♞");
            case BlackBishop: return (u8"♝");
            case BlackRook:   return (u8"♜");
            case BlackQueen:  return (u8"♛");
            case BlackKing:   return (u8"♚");
            }
            return u8" ";
        }

        const char8_t* pieceBackground(board::Piece p) {
            using enum board::Piece;
            switch (p) {
            case WhitePawn:   return (u8"");
            case WhiteKnight: return (u8"");
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
            }
            return u8" ";
        }

    }

    void drawChessBoard(int width, int height) {
        constexpr auto MAX_BORDER_TEXT_SIZE = 30.0f;

        ImGui::SetNextWindowSizeConstraints(
            ImVec2(150, 150),
            ImVec2(static_cast<float>(width), static_cast<float>(height))
        );
        ImGui::Begin("Chess Board", nullptr);

        const auto region = ImGui::GetContentRegionAvail();
        if (region.x <= 0.0f || region.y <= 0.0f) {
			ImGui::End();
            return;
        }
        ImGui::PushFont(font::chessFont);

        const auto board = board::createTestBoard();

        const int gridSize = 8;
        auto* drawList = ImGui::GetWindowDrawList();
        const auto boardPos = ImGui::GetCursorScreenPos();


        const float cellSize = std::floor(std::min(region.x, region.y) / gridSize) * 0.95f;
        const float boardSize = cellSize * gridSize;
        const ImVec2 boardEnd = { boardPos.x + boardSize, boardPos.y + boardSize };

        auto* font = ImGui::GetFont();

        bool isWhite = true;
        for (int row = 0; row < gridSize; ++row) {
            for (int col = 0; col < gridSize; ++col) {
                const ImVec2 cellMin = {
                    boardPos.x + col * cellSize,
                    boardPos.y + row * cellSize
                };
                const ImVec2 cellMax = {
                    cellMin.x + cellSize,
                    cellMin.y + cellSize
                };

                const ImU32 color = isWhite ? IM_COL32(240, 217, 181, 255) : IM_COL32(181, 136, 99, 255);
                drawList->AddRectFilled(cellMin, cellMax, color);

                if (const auto& piece = board[row][col]; piece.has_value()) {
                    const char* text = reinterpret_cast<const char*>(pieceSymbol(*piece));
					const char* backgroundText = reinterpret_cast<const char*>(pieceBackground(*piece));
                    const float fontSize = cellSize * 0.9f;
                    const ImVec2 textSize = font->CalcTextSizeA(fontSize, cellSize, -1.0f, text);

                    const ImVec2 textPos = {
                        cellMin.x + (cellSize - textSize.x) * 0.5f,
                        cellMin.y + (cellSize - textSize.y) * 0.5f
                    };

                    drawList->AddText(font, fontSize, textPos, IM_COL32_WHITE, backgroundText);

                    drawList->AddText(font, fontSize, textPos, IM_COL32_BLACK, text);
                }

                isWhite = !isWhite;
            }
            isWhite = !isWhite;
        }

        // File A-H below
        for (int col = 0; col < gridSize; ++col) {
            auto label = std::string(1, 'a' + col);
            const ImVec2 pos = {
                boardPos.x + col * cellSize + cellSize * 0.5f - ImGui::CalcTextSize(label.c_str()).x * 0.5f,
                boardPos.y + boardSize + cellSize * 0
            };
            drawList->AddText(
                font, std::min(cellSize * 0.5f, MAX_BORDER_TEXT_SIZE),
                pos,
                IM_COL32_WHITE,
                label.c_str()
            );
        }

        // Rank 1-8 right
        for (int row = 0; row < gridSize; ++row) {
            auto label = std::string(1, '8' - row);
            const ImVec2 pos = {
                boardPos.x + boardSize + cellSize * 0,
                boardPos.y + row * cellSize + cellSize * 0.3f
            };
            drawList->AddText(
                font, std::min(cellSize * 0.5f, MAX_BORDER_TEXT_SIZE),
                pos,
                IM_COL32_WHITE,
                label.c_str()
            );
        }

        ImGui::Dummy(ImVec2(boardSize, boardSize));
        ImGui::PopFont();

        ImGui::End();
    }

}
