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
                                     float cellSize, QaplaBasics::File file, QaplaBasics::Rank rank, bool isWhite, bool popupIsHovered)
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

        if (allowPieceInput_ && ImGui::IsItemHovered() && !popupIsHovered)
        {
            // Store hovered square for later rendering (after board pieces)
            hoveredSquareForPopup_ = square;
            hoveredSquareCellMin_ = cellMin;
            hoveredSquareCellMax_ = cellMax;
            hoveredSquareCellSize_ = cellSize;
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

    void ImGuiBoard::drawBoardSquares(ImDrawList *drawList, const ImVec2 &boardPos, float cellSize, bool popupIsHovered) 
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
                drawBoardSquare(drawList, boardPos, cellSize, file, rank, isWhite, popupIsHovered);
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
        if (!moveInput_.to) {
            // Autocomplete is disabled if only the starting position is provided,
            // as this could confuse users.
            return std::nullopt;
        }
        const auto [move, valid, promotion] = gameState_->resolveMove(
            std::nullopt, moveInput_.from, moveInput_.to, moveInput_.promotion);

        promotionPending_ = promotion;
        if (!valid) {
            moveInput_ = {};
        } else if (!move.isEmpty()) {
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

    bool ImGuiBoard::handlePieceSelectionPopup()
    {
        if (!hoveredSquareForPopup_) {
            return false;
        }
        
        // Calculate popup dimensions using same logic as drawPieceSelectionPopup
        constexpr int COL_COUNT = 4;
        constexpr float GRID_CELL_SIZE_RATIO = 0.4F;
        const float gridCellSize = hoveredSquareCellSize_ * GRID_CELL_SIZE_RATIO;
        const float popupWidth = gridCellSize * COL_COUNT;
        const float popupHeight = gridCellSize * COL_COUNT;
        const ImVec2 popupMin = {
            hoveredSquareCellMin_.x + (hoveredSquareCellSize_ - popupWidth) * 0.5F,
            hoveredSquareCellMin_.y + (hoveredSquareCellSize_ - popupHeight) * 0.5F
        };
        const ImVec2 popupMax = {popupMin.x + popupWidth, popupMin.y + popupHeight};
        
        // Check if mouse is over popup area (manual check, not using ImGui button)
        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool isMouseOverPopup = 
            mousePos.x >= popupMin.x && mousePos.x <= popupMax.x &&
            mousePos.y >= popupMin.y && mousePos.y <= popupMax.y;
        
        return isMouseOverPopup;
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
        
        // Check if popup is hovered BEFORE drawing board squares
        bool popupIsHovered = handlePieceSelectionPopup();

        if (!popupIsHovered) {
            // Reset hovered square before drawing
            hoveredSquareForPopup_ = std::nullopt;
        }

        drawBoardSquares(drawList, topLeft, cellSize, popupIsHovered);
        drawBoardPieces(drawList, topLeft, cellSize, font);
        drawBoardCoordinates(drawList, topLeft, cellSize, boardSize, font, maxBorderTextSize);

        // Draw piece selection popup AFTER everything else (on top) if there's a hovered square
        if (hoveredSquareForPopup_)
        {
            auto selectedPiece = drawPieceSelectionPopup(
                hoveredSquareCellMin_, hoveredSquareCellMax_, hoveredSquareCellSize_);
            
            if (selectedPiece) {
                if (*selectedPiece != QaplaBasics::Piece::NO_PIECE) {
                    gameState_->position().setPiece(*hoveredSquareForPopup_, *selectedPiece);
                } else {
                    gameState_->position().clearPiece(*hoveredSquareForPopup_);
                }
            }
        }

        const float coordTextHeight = std::min(cellSize * 0.5F, maxBorderTextSize);
        ImGui::Dummy(ImVec2(boardSize, boardSize + coordTextHeight));

        ImGui::PopFont();

        const auto moveRecord = checkMove();
        return moveRecord;
    }

    std::pair<ImVec2, ImVec2> ImGuiBoard::getPopupCellBounds(
        const ImVec2& popupMin, float cellSize, int col, int row)
    {
        // Grid is 4x4 with all cells having the same size
        // Simple calculation: position = popupMin + (col/row * cellSize)
        const float x = popupMin.x + col * cellSize;
        const float y = popupMin.y + row * cellSize;
        
        return {{x, y}, {x + cellSize, y + cellSize}};
    }

    void ImGuiBoard::drawPopupRect(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 bgColor)
    {
        drawList->AddRectFilled(min, max, bgColor);
        drawList->AddRect(min, max, IM_COL32(0, 0, 0, 255));
    }

    void ImGuiBoard::drawPopupPiece(ImDrawList* drawList, ImFont* font, 
                                     QaplaBasics::Piece piece, const ImVec2& min, float size)
    {
        font::drawPiece(drawList, piece, min, size, font);
    }

    bool ImGuiBoard::isRectClicked(const ImVec2& min, const ImVec2& max)
    {
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            return false;
        }
        const ImVec2 mousePos = ImGui::GetMousePos();
        return mousePos.x >= min.x && mousePos.x <= max.x && 
               mousePos.y >= min.y && mousePos.y <= max.y;
    }

    std::optional<QaplaBasics::Piece> ImGuiBoard::drawPieceSelectionPopup(
        const ImVec2& cellMin, const ImVec2& cellMax, float cellSize)
    {
        using QaplaBasics::Piece;
        
        // Grid configuration - can easily change to 3x3 by changing these constants
        constexpr int ROW_COUNT = 4;
        constexpr int COL_COUNT = 4;
        constexpr float GRID_CELL_SIZE_RATIO = 0.4F; // Each grid cell is 40% of board cell
        
        const float gridCellSize = cellSize * GRID_CELL_SIZE_RATIO;
        const float popupWidth = gridCellSize * COL_COUNT;
        const float popupHeight = gridCellSize * ROW_COUNT;
        
        const ImVec2 popupMin = {
            cellMin.x + (cellSize - popupWidth) * 0.5F,
            cellMin.y + (cellSize - popupHeight) * 0.5F
        };
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();
        ImGui::PushFont(font::chessFont);
        
        // Background
        const ImVec2 popupMax = {popupMin.x + popupWidth, popupMin.y + popupHeight};
        drawList->AddRectFilled(popupMin, popupMax, IM_COL32(50, 50, 50, 230));
        drawList->AddRect(popupMin, popupMax, IM_COL32(255, 255, 255, 255), 0.0F, 0, 2.0F);
        
        // Draw center separately (spans 4 grid cells: [1,1] to [2,2])
        const ImVec2 centerMin = {popupMin.x + gridCellSize, popupMin.y + gridCellSize};
        const ImVec2 centerMax = {centerMin.x + 2 * gridCellSize, centerMin.y + 2 * gridCellSize};
        drawList->AddRectFilled(centerMin, centerMax, IM_COL32(255, 255, 0, 255));
        drawList->AddRect(centerMin, centerMax, IM_COL32(0, 0, 0, 255), 0.0F, 0, 2.0F);
        
        if (lastSelectedPiece_ == Piece::NO_PIECE) {
            const float padding = gridCellSize * 0.4F;
            drawList->AddLine({centerMin.x + padding, centerMin.y + padding}, 
                           {centerMax.x - padding, centerMax.y - padding}, 
                           IM_COL32(255, 0, 0, 255), 3.0F);
            drawList->AddLine({centerMin.x + padding, centerMax.y - padding}, 
                           {centerMax.x - padding, centerMin.y + padding}, 
                           IM_COL32(255, 0, 0, 255), 3.0F);
        } else {
            drawPopupPiece(drawList, font, lastSelectedPiece_, centerMin, 2 * gridCellSize);
        }
        
        // Draw all configured cells
        for (const auto& cell : cells) {
            if (cell.type == PopupCellType::EMPTY || cell.type == PopupCellType::CENTER) {
                continue; // Skip empty cells and center (already drawn)
            }
            
            const auto [cellMin, cellMax] = getPopupCellBounds(popupMin, gridCellSize, cell.col, cell.row);
            
            
            if (cell.type == PopupCellType::PIECE) {
                const Piece coloredPiece = cell.basePiece + pieceColor_;
                drawPopupRect(drawList, cellMin, cellMax, IM_COL32(240, 217, 181, 255));
                drawPopupPiece(drawList, font, coloredPiece, cellMin, gridCellSize);
            } else if (cell.type == PopupCellType::COLOR_SWITCH) {
                drawPopupRect(drawList, cellMin, cellMax, IM_COL32(200, 200, 200, 255));
                const ImVec2 center = {(cellMin.x + cellMax.x) * 0.5F, (cellMin.y + cellMax.y) * 0.5F};
                const float radius = gridCellSize * 0.3F;
                ImU32 circleColor = (pieceColor_ == QaplaBasics::Piece::WHITE) ? 
                    IM_COL32(0, 0, 0, 255) : IM_COL32(255, 255, 255, 255);

                drawList->AddCircleFilled(center, radius, circleColor);
            } else if (cell.type == PopupCellType::CLEAR) {
                drawPopupRect(drawList, cellMin, cellMax, IM_COL32(240, 217, 181, 255));
                const float padding = gridCellSize * 0.2F;
                drawList->AddLine({cellMin.x + padding, cellMin.y + padding}, 
                               {cellMax.x - padding, cellMax.y - padding}, 
                               IM_COL32(255, 0, 0, 255), 2.0F);
                drawList->AddLine({cellMin.x + padding, cellMax.y - padding}, 
                               {cellMax.x - padding, cellMin.y + padding}, 
                               IM_COL32(255, 0, 0, 255), 2.0F);
            }
        }
        
        ImGui::PopFont();
        
        return handlePieceSelectionClick(popupMin, gridCellSize);
    }

    std::optional<QaplaBasics::Piece> ImGuiBoard::handlePieceSelectionClick(
        const ImVec2& popupMin, float gridCellSize)
    {
        using QaplaBasics::Piece;
        
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            return std::nullopt;
        }
        
        for (const auto& cell : cells) {
            const auto [cellMin, cellMax] = getPopupCellBounds(popupMin, gridCellSize, cell.col, cell.row);
            
            if (isRectClicked(cellMin, cellMax)) {
                if (cell.type == PopupCellType::CENTER) {
                    return lastSelectedPiece_; // Place piece on board
                } 
                if (cell.type == PopupCellType::PIECE) {
                    lastSelectedPiece_ = cell.basePiece + pieceColor_;
                } else if (cell.type == PopupCellType::COLOR_SWITCH) {
                    pieceColor_ = QaplaBasics::switchColor(pieceColor_);
                } else if (cell.type == PopupCellType::CLEAR) {
                    lastSelectedPiece_ = Piece::NO_PIECE;
                }
                return std::nullopt; // No piece placed yet
            }
        }
        
        return std::nullopt;
    }

}
