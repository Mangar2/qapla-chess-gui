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
 * @author Volker B�hm
 * @copyright Copyright (c) 2025 Volker B�hm
 */

#include "imgui-board.h"
#include "imgui.h"

#include "qapla-engine/types.h"
#include "qapla-engine/movegenerator.h"
#include "qapla-engine/move.h"
#include "font.h"
#include "imgui-button.h"

#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace QaplaWindows {

    ImGuiBoard::ImGuiBoard() 
        : position_(std::make_unique<QaplaMoveGenerator::MoveGenerator>())
    {
    }

    std::pair<ImVec2, ImVec2> ImGuiBoard::computeCellCoordinates(const ImVec2& boardPos, float cellSize, 
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

    void ImGuiBoard::drawPromotionPopup(float cellSize)
    {
        using QaplaBasics::Piece;
        constexpr float SHRINK_CELL_SIZE = 0.8f;
		auto& position = *position_;

        if (!moveInput_.to) return;

        MoveInput moveInput;

        const bool whiteToMove = position.isWhiteToMove();
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
                moveInput.promotion = pieces[i];

                moveInput.from.reset();
                moveInput.to.reset();
                promotionPending_ = false;
                ImGui::CloseCurrentPopup();
            }

            const ImU32 bgColor = IM_COL32(240, 217, 181, 255);
            drawList->AddRectFilled(cellMin, cellMax, bgColor);
            font::drawPiece(drawList, pieces[i], cellMin, cellSize, font);
        }

        ImGui::PopFont();
    }


    void ImGuiBoard::drawBoardSquare(ImDrawList* drawList, const ImVec2& boardPos, 
        float cellSize, QaplaBasics::File file, QaplaBasics::Rank rank, bool isWhite)
    {
        using QaplaBasics::Square;
        using QaplaBasics::Piece;
		auto& position = *position_;

        const Square square = computeSquare(file, rank);
        const Piece piece = position[square];

		const auto [cellMin, cellMax] = computeCellCoordinates(boardPos, cellSize, file, rank);

        const bool isSelected = (moveInput_.from && *moveInput_.from == square) 
            || (moveInput_.to && *moveInput_.to == square);
        const ImU32 bgColor = isSelected
            ? IM_COL32(100, 149, 237, 255)
            : (isWhite ? IM_COL32(240, 217, 181, 255) : IM_COL32(181, 136, 99, 255));

        drawList->AddRectFilled(cellMin, cellMax, bgColor);

        ImGui::SetCursorScreenPos(cellMin);
        ImGui::InvisibleButton(("cell_" + std::to_string(square)).c_str(), ImVec2(cellSize, cellSize));

        if (ImGui::IsItemClicked() && allowMoveInput_) {
			bool wtm = position.isWhiteToMove();
            if (piece != Piece::NO_PIECE && getPieceColor(piece) == (wtm ? Piece::WHITE : Piece::BLACK)) {
                moveInput_.from = square;
            }
            else {
                moveInput_.to = square;
            }

            if (promotionPending_) {
                ImGui::OpenPopup("Promotion");
            }
        }
    }

    void ImGuiBoard::drawBoardSquares(ImDrawList* drawList, const ImVec2& boardPos, float cellSize)
    {
        using QaplaBasics::Rank;
        using QaplaBasics::File;

        bool isWhite = false;
        for (Rank rank = Rank::R1; rank <= Rank::R8; ++rank) {
            for (File file = File::A; file <= File::H; ++file) {
                drawBoardSquare(drawList, boardPos, cellSize, file, rank, isWhite);
                isWhite = !isWhite;
            }
            isWhite = !isWhite;
        }
    }

    void ImGuiBoard::drawBoardPieces(ImDrawList* drawList, const ImVec2& boardPos, float cellSize, ImFont* font)
    {
        using QaplaBasics::Rank;
        using QaplaBasics::File;
        using QaplaBasics::Square;
        using QaplaBasics::Piece;
        auto& position = *position_;
        for (Rank rank = Rank::R1; rank <= Rank::R8; ++rank) {
            for (File file = File::A; file <= File::H; ++file) {
                Square sq = computeSquare(file, rank);
                Piece piece = position[sq];
                const auto [cellMin, _] = computeCellCoordinates(boardPos, cellSize, file, rank);
                font::drawPiece(drawList, piece, cellMin, cellSize, font);
            }
        }
    }

    void ImGuiBoard::drawBoardCoordinates(ImDrawList* drawList,
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

    MoveInput ImGuiBoard::draw()
    {
        constexpr float maxBorderTextSize = 30.0f;
        constexpr int gridSize = 8;

        const auto screenPos = ImGui::GetCursorScreenPos();
        const auto region = ImGui::GetContentRegionAvail();
        const auto boardHeight = std::max(50.0f, region.y - 10.0f);
		const auto boardWidth = std::max(50.0f, region.x - 10.0f);

        if (region.x <= 0.0f || boardHeight <= 0.0f) {
            return moveInput_;
        }

        ImGui::PushFont(font::chessFont);

        const float cellSize = std::floor(std::min(boardWidth, boardHeight) / gridSize) * 0.95f;

        if (promotionPending_) {
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1.0f, 1.0f, 1.0f, 0.3f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
            if (ImGui::BeginPopup("Promotion")) {
                drawPromotionPopup(cellSize);
                ImGui::EndPopup();
            }
            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(1);
        }

        const float boardSize = cellSize * gridSize;

		auto topLeft = ImVec2(screenPos.x + 3, screenPos.y + 3);

        const ImVec2 boardEnd = { screenPos.x + boardSize, screenPos.y + boardSize };
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();

        drawBoardSquares(drawList, topLeft, cellSize);
        drawBoardPieces(drawList, topLeft, cellSize, font);
        drawBoardCoordinates(drawList, topLeft, cellSize, boardSize, font, maxBorderTextSize);

        const float coordTextHeight = std::min(cellSize * 0.5f, maxBorderTextSize);
        ImGui::Dummy(ImVec2(boardSize, boardSize + coordTextHeight));

        ImGui::PopFont();

        return moveInput_;

    }

}
