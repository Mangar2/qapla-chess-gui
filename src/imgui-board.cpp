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

#include "imgui-board.h"
#include "font.h"
#include "imgui-button.h"

#include "qapla-tester/game-state.h"
#include "qapla-tester/move-record.h"
#include "qapla-tester/game-record.h"

#include "qapla-engine/types.h"
#include "qapla-engine/move.h"

#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace QaplaWindows
{

    ImGuiBoard::ImGuiBoard()
        : gameState_(std::make_unique<GameState>())
    {
    }

    ImGuiBoard::ImGuiBoard(ImGuiBoard&&) = default;
    ImGuiBoard& ImGuiBoard::operator=(ImGuiBoard&&) = default;

    // Must be defined in the .cpp file to ensure that the destructor of unique_ptr<GameState> is known here.
    ImGuiBoard::~ImGuiBoard() = default;

    void ImGuiBoard::setFromFen(bool startPos, const std::string &fen)
    {
        gameState_->setFen(startPos, fen);
        moveInput_ = {};
    }

    std::string ImGuiBoard::getFen() const
    {
        return gameState_->position().getFen();
    }

    bool ImGuiBoard::isValidPosition() const
    {
        return gameState_->position().isValidPosition();
    }

    std::pair<ImVec2, ImVec2> ImGuiBoard::computeCellCoordinates(const ImVec2 &boardPos, float cellSize,
        QaplaBasics::File file, QaplaBasics::Rank rank) const
    {
        float cellX;
        float cellY;
        
        if (boardInverted_) {
            // When inverted: a1 should be at top-right, h8 at bottom-left
            cellX = static_cast<float>(QaplaBasics::File::H - file) * cellSize;
            cellY = static_cast<float>(rank) * cellSize;
        } else {
            // Normal: a1 should be at bottom-left, h8 at top-right  
            cellX = static_cast<float>(file) * cellSize;
            cellY = static_cast<float>(QaplaBasics::Rank::R8 - rank) * cellSize;
        }

        const ImVec2 cellMin = {
            boardPos.x + cellX,
            boardPos.y + cellY};
        const ImVec2 cellMax = {cellMin.x + cellSize, cellMin.y + cellSize};
        return {cellMin, cellMax};
    }

    void ImGuiBoard::drawPromotionPopup(float cellSize)
    {
        using QaplaBasics::Piece;
        constexpr float SHRINK_CELL_SIZE = 0.8F;
        const auto &position = gameState_->position();

        if (!moveInput_.to) {
            return;
        }

        const bool whiteToMove = position.isWhiteToMove();
        const std::array<Piece, 4> pieces = {
            whiteToMove ? Piece::WHITE_QUEEN : Piece::BLACK_QUEEN,
            whiteToMove ? Piece::WHITE_ROOK : Piece::BLACK_ROOK,
            whiteToMove ? Piece::WHITE_BISHOP : Piece::BLACK_BISHOP,
            whiteToMove ? Piece::WHITE_KNIGHT : Piece::BLACK_KNIGHT};

        cellSize = std::max(30.0F, cellSize * SHRINK_CELL_SIZE);
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImFont *font = ImGui::GetFont();
        ImGui::PushFont(font::chessFont);

        const ImVec2 startPos = ImGui::GetCursorScreenPos();

        for (int i = 0; i < 4; ++i)
        {
            const ImVec2 cellMin = {
                startPos.x + i * cellSize,
                startPos.y};
            const ImVec2 cellMax = {
                cellMin.x + cellSize,
                cellMin.y + cellSize};

            ImGui::SetCursorScreenPos(cellMin);
            ImGui::InvisibleButton(("promo_" + std::to_string(i)).c_str(), ImVec2(cellSize, cellSize));

            if (ImGui::IsItemClicked())
            {
                moveInput_.promotion = pieces[i];
                ImGui::CloseCurrentPopup();
            }

            const ImU32 bgColor = IM_COL32(240, 217, 181, 255);
            drawList->AddRectFilled(cellMin, cellMax, bgColor);
            font::drawPiece(drawList, pieces[i], cellMin, cellSize, font);
        }

        ImGui::PopFont();
    }

    static ImU32 getSquareColor(bool isSelected, bool isWhite)
    {
        if (isSelected) {
            return IM_COL32(100, 149, 237, 255);
        }
        return isWhite ? IM_COL32(240, 217, 181, 255) : IM_COL32(181, 136, 99, 255);
    }

    void ImGuiBoard::drawBoardSquare(ImDrawList *drawList, const ImVec2 &boardPos,
                                     float cellSize, QaplaBasics::File file, QaplaBasics::Rank rank, bool isWhite)
    {
        using QaplaBasics::Piece;
        using QaplaBasics::Square;
        const auto &position = gameState_->position();

        const Square square = computeSquare(file, rank);
        const Piece piece = position[square];

        const auto [cellMin, cellMax] = computeCellCoordinates(boardPos, cellSize, file, rank);

        const bool isSelected = (moveInput_.from && *moveInput_.from == square) || (moveInput_.to && *moveInput_.to == square);
        const ImU32 bgColor = getSquareColor(isSelected, isWhite);

        drawList->AddRectFilled(cellMin, cellMax, bgColor);

        ImGui::SetCursorScreenPos(cellMin);
        ImGui::InvisibleButton(("cell_" + std::to_string(square)).c_str(), ImVec2(cellSize, cellSize));

        if (allowPieceInput_ && ImGui::IsItemHovered())
        {
            drawPieceSelectionPopup(cellMin, cellMax, cellSize);
        }
        else if (ImGui::IsItemClicked() && allowMoveInput_ && !gameOver_)
        {
            bool wtm = position.isWhiteToMove();
            if (piece != Piece::NO_PIECE && getPieceColor(piece) == (wtm ? Piece::WHITE : Piece::BLACK))
            {
                moveInput_.from = square;
            }
            else
            {
                moveInput_.to = square;
            }

            if (promotionPending_)
            {
                ImGui::OpenPopup("Promotion");
            }
        }
    }

    void ImGuiBoard::drawBoardSquares(ImDrawList *drawList, const ImVec2 &boardPos, float cellSize) 
    {
        using QaplaBasics::File;
        using QaplaBasics::Rank;

        for (Rank rank = Rank::R1; rank <= Rank::R8; ++rank)
        {
            for (File file = File::A; file <= File::H; ++file)
            {
                // Calculate if square should be white based on board coordinates
                // a1 (File::A=0, Rank::R1=0) should be black (false)
                // Standard chess: (file + rank) % 2 == 0 means black square
                bool isWhite = (static_cast<int>(file) + static_cast<int>(rank)) % 2 != 0;
                drawBoardSquare(drawList, boardPos, cellSize, file, rank, isWhite);
            }
        }
    }

    void ImGuiBoard::drawBoardPieces(ImDrawList *drawList, const ImVec2 &boardPos, float cellSize, ImFont *font) {
        using QaplaBasics::File;
        using QaplaBasics::Piece;
        using QaplaBasics::Rank;
        using QaplaBasics::Square;
        const auto &position = gameState_->position();
        for (Rank rank = Rank::R1; rank <= Rank::R8; ++rank)
        {
            for (File file = File::A; file <= File::H; ++file)
            {
                Square sq = computeSquare(file, rank);
                Piece piece = position[sq];
                const auto [cellMin, _] = computeCellCoordinates(boardPos, cellSize, file, rank);
                font::drawPiece(drawList, piece, cellMin, cellSize, font);
            }
        }
    }

    void ImGuiBoard::drawBoardCoordinates(ImDrawList *drawList,
        const ImVec2 &boardPos, float cellSize, float boardSize, ImFont *font, float maxSize) const
    {
        const int gridSize = 8;

        // Draw file labels (a-h)
        for (int col = 0; col < gridSize; ++col)
        {
            // When inverted: visual col 0 should show 'h', visual col 7 should show 'a'
            // When normal: visual col 0 should show 'a', visual col 7 should show 'h'
            char fileChar = boardInverted_ ? ('h' - col) : ('a' + col);
            std::string label(1, fileChar);
            ImVec2 pos = {
                boardPos.x + col * cellSize + cellSize * 0.5F - ImGui::CalcTextSize(label.c_str()).x * 0.5F,
                boardPos.y + boardSize};
            drawList->AddText(font, std::min(cellSize * 0.5F, maxSize), pos, IM_COL32_WHITE, label.c_str());
        }

        // Draw rank labels (1-8)
        for (int row = 0; row < gridSize; ++row)
        {
            // When inverted: visual row 0 should show '1', visual row 7 should show '8'
            // When normal: visual row 0 should show '8', visual row 7 should show '1'
            int rankNumber = boardInverted_ ? (row + 1) : (8 - row);
            std::string label = std::to_string(rankNumber);
            ImVec2 pos = {
                boardPos.x + boardSize,
                boardPos.y + row * cellSize + cellSize * 0.3F};
            drawList->AddText(font, std::min(cellSize * 0.5F, maxSize), pos, IM_COL32_WHITE, label.c_str());
        }
    }

    std::optional<MoveRecord> ImGuiBoard::checkMove() {
        const auto [move, valid, promotion] = gameState_->resolveMove(
            std::nullopt, moveInput_.from, moveInput_.to, moveInput_.promotion);

        promotionPending_ = promotion;
        if (!valid) {
            moveInput_ = {};
        }
        else if (!moveInput_.to) {
            // Autocomplete is disabled if only the starting position is provided,
            // as this could confuse users.
            return std::nullopt;
        }
        else if (!move.isEmpty()) {
            moveInput_ = {};
            MoveRecord moveRecord;
            moveRecord.lan = move.getLAN();
            moveRecord.san = gameState_->moveToSan(move);
            moveRecord.halfmoveNo_ = gameState_->getHalfmovePlayed() + 1;
            moveRecord.move = move;
            return moveRecord;
        }
        return std::nullopt;
    }

    void ImGuiBoard::setFromGameRecord(const GameRecord &gameRecord)
    {
        auto updated = gameRecordTracker_.checkModification(gameRecord.getChangeTracker()).second;
        gameRecordTracker_.updateFrom(gameRecord.getChangeTracker());
        if (updated) {
            gameState_->setFromGameRecord(gameRecord, gameRecord.nextMoveIndex());
            gameOver_ = gameRecord.isGameOver();
        }
    }

    std::optional<MoveRecord> ImGuiBoard::draw()
    {
        constexpr float maxBorderTextSize = 30.0F;
        constexpr int gridSize = 8;

        const auto screenPos = ImGui::GetCursorScreenPos();
        const auto region = ImGui::GetContentRegionAvail();
        const auto boardHeight = std::max(50.0F, region.y - 10.0F);
        const auto boardWidth = std::max(50.0F, region.x - 10.0F);

        if (region.x <= 0.0F || boardHeight <= 0.0F)
        {
            return {};
        }

        ImGui::PushFont(font::chessFont);

        const float cellSize = std::floor(std::min(boardWidth, boardHeight) / gridSize) * 0.95F;

        if (promotionPending_)
        {
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1.0F, 1.0F, 1.0F, 0.3F));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
            if (ImGui::BeginPopup("Promotion"))
            {
                drawPromotionPopup(cellSize);
                ImGui::EndPopup();
            }
            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(1);
        }

        const float boardSize = cellSize * gridSize;

        auto topLeft = ImVec2(screenPos.x + 3, screenPos.y + 3);

        const ImVec2 boardEnd = {screenPos.x + boardSize, screenPos.y + boardSize};
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImFont *font = ImGui::GetFont();

        drawBoardSquares(drawList, topLeft, cellSize);
        drawBoardPieces(drawList, topLeft, cellSize, font);
        drawBoardCoordinates(drawList, topLeft, cellSize, boardSize, font, maxBorderTextSize);

        const float coordTextHeight = std::min(cellSize * 0.5F, maxBorderTextSize);
        ImGui::Dummy(ImVec2(boardSize, boardSize + coordTextHeight));

        ImGui::PopFont();

        const auto moveRecord = checkMove();
        return moveRecord;
    }

    void ImGuiBoard::drawPieceSelectionPopup(const ImVec2& cellMin, const ImVec2& cellMax, float cellSize)
    {
        using QaplaBasics::Piece;
        
        // Calculate popup size and position
        const float popupCellSize = cellSize * 0.5F; // Smaller cells for popup
        const float popupWidth = popupCellSize * 5;  // 5 cells wide
        const float popupHeight = popupCellSize * 3; // 3 cells high
        
        // Center the popup on the cell
        const ImVec2 popupMin = {
            cellMin.x + (cellSize - popupWidth) * 0.5F,
            cellMin.y + (cellSize - popupHeight) * 0.5F
        };
        const ImVec2 popupMax = {popupMin.x + popupWidth, popupMin.y + popupHeight};
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();
        
        // Background
        drawList->AddRectFilled(popupMin, popupMax, IM_COL32(50, 50, 50, 200));
        drawList->AddRect(popupMin, popupMax, IM_COL32(255, 255, 255, 255));
        
        // Determine piece color based on last selected piece
        const bool isWhite = QaplaBasics::getPieceColor(lastSelectedPiece_) == Piece::WHITE;
        
        // Define pieces for selection (6 pieces of current color)
        const std::array<Piece, 6> pieces = isWhite ? 
            std::array<Piece, 6>{Piece::WHITE_PAWN, Piece::WHITE_ROOK, Piece::WHITE_KNIGHT, 
                                 Piece::WHITE_BISHOP, Piece::WHITE_QUEEN, Piece::WHITE_KING} :
            std::array<Piece, 6>{Piece::BLACK_PAWN, Piece::BLACK_ROOK, Piece::BLACK_KNIGHT,
                                 Piece::BLACK_BISHOP, Piece::BLACK_QUEEN, Piece::BLACK_KING};
        
        // Layout: 3x3 grid with center being the last selected piece (larger)
        // Row 0: pieces[0] pieces[1] pieces[2]
        // Row 1: pieces[3] [LAST PIECE - LARGER] pieces[4]  
        // Row 2: pieces[5] [COLOR SWITCH] [EMPTY]
        
        ImGui::PushFont(font::chessFont);
        
        // Row 0
        for (int i = 0; i < 3; ++i) {
            const ImVec2 itemMin = {popupMin.x + i * popupCellSize, popupMin.y};
            const ImVec2 itemMax = {itemMin.x + popupCellSize, itemMin.y + popupCellSize};
            drawList->AddRectFilled(itemMin, itemMax, IM_COL32(240, 217, 181, 255));
            drawList->AddRect(itemMin, itemMax, IM_COL32(0, 0, 0, 255));
            font::drawPiece(drawList, pieces[i], itemMin, popupCellSize, font);
        }
        
        // Row 1 - Left and Right
        for (int i = 0; i < 2; ++i) {
            const ImVec2 itemMin = {popupMin.x + i * 2 * popupCellSize, popupMin.y + popupCellSize};
            const ImVec2 itemMax = {itemMin.x + popupCellSize, itemMin.y + popupCellSize};
            drawList->AddRectFilled(itemMin, itemMax, IM_COL32(240, 217, 181, 255));
            drawList->AddRect(itemMin, itemMax, IM_COL32(0, 0, 0, 255));
            font::drawPiece(drawList, pieces[i + 3], itemMin, popupCellSize, font);
        }
        
        // Center - Last selected piece (larger)
        const ImVec2 centerMin = {popupMin.x + 2 * popupCellSize, popupMin.y + popupCellSize};
        const ImVec2 centerMax = {centerMin.x + popupCellSize, centerMin.y + popupCellSize};
        drawList->AddRectFilled(centerMin, centerMax, IM_COL32(255, 255, 0, 255)); // Yellow highlight
        drawList->AddRect(centerMin, centerMax, IM_COL32(0, 0, 0, 255));
        font::drawPiece(drawList, lastSelectedPiece_, centerMin, popupCellSize, font);
        
        // Row 2
        // King
        {
            const ImVec2 itemMin = {popupMin.x, popupMin.y + 2 * popupCellSize};
            const ImVec2 itemMax = {itemMin.x + popupCellSize, itemMin.y + popupCellSize};
            drawList->AddRectFilled(itemMin, itemMax, IM_COL32(240, 217, 181, 255));
            drawList->AddRect(itemMin, itemMax, IM_COL32(0, 0, 0, 255));
            font::drawPiece(drawList, pieces[5], itemMin, popupCellSize, font);
        }
        
        // Color switch (circle)
        {
            const ImVec2 itemMin = {popupMin.x + popupCellSize, popupMin.y + 2 * popupCellSize};
            const ImVec2 itemMax = {itemMin.x + popupCellSize, itemMin.y + popupCellSize};
            drawList->AddRectFilled(itemMin, itemMax, IM_COL32(200, 200, 200, 255));
            drawList->AddRect(itemMin, itemMax, IM_COL32(0, 0, 0, 255));
            // Draw a circle to represent color switch
            const ImVec2 center = {(itemMin.x + itemMax.x) * 0.5F, (itemMin.y + itemMax.y) * 0.5F};
            const float radius = popupCellSize * 0.3F;
            drawList->AddCircleFilled(center, radius, isWhite ? IM_COL32(0, 0, 0, 255) : IM_COL32(255, 255, 255, 255));
        }
        
        // Empty square (X)
        {
            const ImVec2 itemMin = {popupMin.x + 2 * popupCellSize, popupMin.y + 2 * popupCellSize};
            const ImVec2 itemMax = {itemMin.x + popupCellSize, itemMin.y + popupCellSize};
            drawList->AddRectFilled(itemMin, itemMax, IM_COL32(240, 217, 181, 255));
            drawList->AddRect(itemMin, itemMax, IM_COL32(0, 0, 0, 255));
            // Draw X for empty
            const float padding = popupCellSize * 0.2F;
            drawList->AddLine({itemMin.x + padding, itemMin.y + padding}, 
                           {itemMax.x - padding, itemMax.y - padding}, 
                           IM_COL32(255, 0, 0, 255), 2.0F);
            drawList->AddLine({itemMin.x + padding, itemMax.y - padding}, 
                           {itemMax.x - padding, itemMin.y + padding}, 
                           IM_COL32(255, 0, 0, 255), 2.0F);
        }
        
        ImGui::PopFont();
    }

}
