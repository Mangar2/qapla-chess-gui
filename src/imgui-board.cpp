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
    using QaplaBasics::Piece;
    using QaplaBasics::Square;
    using QaplaBasics::File;
    using QaplaBasics::Rank;
    
    // 4x4 version
    constexpr int GRID_ROW_COUNT = 4;
    constexpr int GRID_COL_COUNT = 4;
    constexpr float CENTER_EXTENT = 2.0F;
    constexpr float GRID_CELL_SIZE_RATIO = 0.35F; 
    constexpr bool ROUND_POPUP_FIELD = false;
    constexpr float ENLARGE_CENTER = 1.0F;
    constexpr float REDUCE_SATELLITES = 1.0F;

    const std::vector<ImGuiBoard::PopupCell> ImGuiBoard::cells = {
        // Top row
        { .col = 2, .row = 0, .type = PopupCellType::COLOR_SWITCH, .basePiece = Piece::NO_PIECE},
        { .col = 3, .row = 0, .type = PopupCellType::CLEAR, .basePiece = Piece::NO_PIECE},
        // Left column
        // Right column
        { .col = 3, .row = 1, .type = PopupCellType::PIECE, .basePiece = Piece::KING },
        { .col = 3, .row = 2, .type = PopupCellType::PIECE, .basePiece = Piece::QUEEN },
        // Bottom row
        { .col = 0, .row = 3, .type = PopupCellType::PIECE, .basePiece = Piece::PAWN },
        { .col = 1, .row = 3, .type = PopupCellType::PIECE, .basePiece = Piece::KNIGHT },
        { .col = 2, .row = 3, .type = PopupCellType::PIECE, .basePiece = Piece::BISHOP },
        { .col = 3, .row = 3, .type = PopupCellType::PIECE, .basePiece = Piece::ROOK },
        // Center (4 cells for click detection)
        { .col = 1, .row = 1, .type = PopupCellType::CENTER, .basePiece = Piece::NO_PIECE},
        { .col = 2, .row = 1, .type = PopupCellType::CENTER, .basePiece = Piece::NO_PIECE},
        { .col = 1, .row = 2, .type = PopupCellType::CENTER, .basePiece = Piece::NO_PIECE},
        { .col = 2, .row = 2, .type = PopupCellType::CENTER, .basePiece = Piece::NO_PIECE}
    };
    
    /*
    // 3x3 version
    constexpr bool ROUND_POPUP_FIELD = true;
    constexpr float ENLARGE_CENTER = 1.4F;
    constexpr float REDUCE_SATELLITES = 0.7F;

    constexpr int GRID_ROW_COUNT = 3;
    constexpr int GRID_COL_COUNT = 3;
    constexpr float CENTER_EXTENT = 1.0F;
    constexpr float GRID_CELL_SIZE_RATIO = 0.5F; // Each grid cell is 40% of board cell

    const std::vector<ImGuiBoard::PopupCell> ImGuiBoard::cells = {
        // Top row
        { .col = 0, .row = 0, .type = PopupCellType::CLEAR, .basePiece = Piece::NO_PIECE},
        { .col = 1, .row = 0, .type = PopupCellType::PIECE, .basePiece = Piece::KING },
        // Left column
        { .col = 0, .row = 1, .type = PopupCellType::COLOR_SWITCH, .basePiece = Piece::NO_PIECE},
        // Right column
        { .col = 2, .row = 0, .type = PopupCellType::PIECE, .basePiece = Piece::QUEEN },
        { .col = 2, .row = 1, .type = PopupCellType::PIECE, .basePiece = Piece::ROOK },
        // Bottom row
        { .col = 0, .row = 2, .type = PopupCellType::PIECE, .basePiece = Piece::PAWN },
        { .col = 1, .row = 2, .type = PopupCellType::PIECE, .basePiece = Piece::KNIGHT },
        { .col = 2, .row = 2, .type = PopupCellType::PIECE, .basePiece = Piece::BISHOP },
        // Center (4 cells for click detection)
        { .col = 1, .row = 1, .type = PopupCellType::CENTER, .basePiece = Piece::NO_PIECE},
    };
    */

    constexpr auto POPUP_CENTER_COLOR = IM_COL32(255, 255, 128, 255);
    constexpr auto POPUP_PIECE_BACKGROUND = IM_COL32(240, 217, 181, 255);
    constexpr auto POPUP_SWITCH_BACKGROUND = IM_COL32(200, 200, 200, 255);
    constexpr auto WHITE_COLOR = IM_COL32(255, 255, 255, 255);
    constexpr auto BLACK_COLOR = IM_COL32(0, 0, 0, 255);

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
        File file, Rank rank) const
    {
        float cellX;
        float cellY;
        
        if (boardInverted_) {
            // When inverted: a1 should be at top-right, h8 at bottom-left
            cellX = static_cast<float>(File::H - file) * cellSize;
            cellY = static_cast<float>(rank) * cellSize;
        } else {
            // Normal: a1 should be at bottom-left, h8 at top-right  
            cellX = static_cast<float>(file) * cellSize;
            cellY = static_cast<float>(Rank::R8 - rank) * cellSize;
        }

        const ImVec2 cellMin = {
            boardPos.x + cellX,
            boardPos.y + cellY};
        const ImVec2 cellMax = {cellMin.x + cellSize, cellMin.y + cellSize};
        return {cellMin, cellMax};
    }

    void ImGuiBoard::drawPromotionPopup(float cellSize)
    {
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
                                     float cellSize, File file, Rank rank, bool isWhite, bool popupIsHovered)
    {
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
        const auto &position = gameState_->position();
        
        // Draw EP indicator outside the piece loop
        for (Rank rank = Rank::R1; rank <= Rank::R8; ++rank)
        {
            for (File file = File::A; file <= File::H; ++file)
            {
                Square sq = computeSquare(file, rank);
                Piece piece = position[sq];
                const auto [cellMin, _] = computeCellCoordinates(boardPos, cellSize, file, rank);
                
                // Draw the piece
                font::drawPiece(drawList, piece, cellMin, cellSize, font);
                
                // Draw castling indicators on top of king pieces
                bool isWhite = (static_cast<int>(file) + static_cast<int>(rank)) % 2 != 0;
                if (piece == Piece::WHITE_KING) {
                    drawWhiteKingCastlingIndicators(drawList, cellMin, cellSize, isWhite,font);
                } else if (piece == Piece::BLACK_KING) {
                    drawBlackKingCastlingIndicators(drawList, cellMin, cellSize, isWhite, font);
                }
            }
        }
        drawEPIndicator(drawList, boardPos, cellSize, font);
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

    void ImGuiBoard::drawIndicatorIcon(ImDrawList *drawList, const ImVec2 &cellMin, 
        float cellSize, bool left, Piece piece, ImU32 bgColor, ImFont *font)
    {
        constexpr float INDICATOR_SIZE = 0.30F;
        constexpr float INDICATOR_PADDING = 0.25F;
        
        const float indicatorSize = cellSize * INDICATOR_SIZE;
        const float radius = indicatorSize * 0.6F;
        float posX = indicatorSize * INDICATOR_PADDING;
        if (!left) {
            posX = cellSize - posX - indicatorSize;
        }
        const ImVec2 indicatorMin = { 
            cellMin.x + posX, 
            cellMin.y + indicatorSize * INDICATOR_PADDING 
        };
        const ImVec2 center = { 
            indicatorMin.x + indicatorSize * 0.5F, 
            indicatorMin.y + indicatorSize * 0.5F 
        };
        
        drawList->AddCircleFilled(center, radius, bgColor);
        
        // Draw piece icon
        font::drawPiece(drawList, piece, indicatorMin, indicatorSize, font);
    }

    void ImGuiBoard::drawEPIndicator(ImDrawList *drawList, const ImVec2 &boardPos, 
        float cellSize, ImFont *font)
    {
        const auto &position = gameState_->position();
        const auto epSquare = position.getEP();

        if (!allowPieceInput_ || epSquare == Square::NO_SQUARE) {
            return;
        }
        
        const auto epFile = getFile(epSquare);
        const auto epRank = getRank(epSquare);
        const auto [cellMin, _] = computeCellCoordinates(boardPos, cellSize, epFile, epRank);
        
        // Draw opponent's pawn (the one that can be captured)
        const Piece epPawn = position.isWhiteToMove() ? Piece::WHITE_PAWN : Piece::BLACK_PAWN;
        drawIndicatorIcon(drawList, cellMin, cellSize, false, epPawn, IM_COL32(255, 255, 0, 255), font);
    }

    void ImGuiBoard::drawWhiteKingCastlingIndicators(ImDrawList *drawList, 
        const ImVec2 &cellMin, float cellSize, bool isWhite, ImFont *font)
    {
        if (!allowPieceInput_) {
            return;
        }
        const auto &position = gameState_->position();
        const auto bgColor = getSquareColor(false, !isWhite);
        
        if (position.isKingSideCastleAllowed<Piece::WHITE>()) {
            drawIndicatorIcon(drawList, cellMin, cellSize, false, Piece::WHITE_ROOK, bgColor, font);
        }
        
        if (position.isQueenSideCastleAllowed<Piece::WHITE>()) {
            drawIndicatorIcon(drawList, cellMin, cellSize, true, Piece::WHITE_ROOK, bgColor, font);
        }
    }

    void ImGuiBoard::drawBlackKingCastlingIndicators(ImDrawList *drawList, 
        const ImVec2 &cellMin, float cellSize, bool isWhite, ImFont *font)
    {
        if (!allowPieceInput_) {
            return;
        }
        const auto &position = gameState_->position();
        const auto bgColor = getSquareColor(false, !isWhite);
        
        if (position.isKingSideCastleAllowed<Piece::BLACK>()) {
            drawIndicatorIcon(drawList, cellMin, cellSize, false, Piece::BLACK_ROOK, bgColor, font);
        }
        
        if (position.isQueenSideCastleAllowed<Piece::BLACK>()) {
            drawIndicatorIcon(drawList, cellMin, cellSize, true, Piece::BLACK_ROOK, bgColor, font);
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
            const Piece currentPieceOnSquare = gameState_->position()[*hoveredSquareForPopup_];
            auto selectedPiece = drawPieceSelectionPopup(
                hoveredSquareCellMin_, hoveredSquareCellMax_, hoveredSquareCellSize_, currentPieceOnSquare);
            
            if (selectedPiece) {
                if (*selectedPiece != Piece::NO_PIECE) {
                    gameState_->position().setupAddPiece(*hoveredSquareForPopup_, *selectedPiece);
                } else {
                    gameState_->position().setupRemovePiece(*hoveredSquareForPopup_);
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
        if (ROUND_POPUP_FIELD) {
            const float rounding = (max.x - min.x) * 0.5F; // Fully round
            drawList->AddRectFilled(min, max, bgColor, rounding);
            drawList->AddRect(min, max, IM_COL32(0, 0, 0, 255), rounding);
        } else {
            drawList->AddRectFilled(min, max, bgColor);
            drawList->AddRect(min, max, IM_COL32(0, 0, 0, 255));
        }
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

    // Helper function to adjust satellite position for round popup
    static std::pair<ImVec2, ImVec2> adjustSatellitePosition(
        const ImVec2& originalMin, const ImVec2& originalMax, 
        const ImVec2& popupMin, float gridCellSize)
    {
        if (!ROUND_POPUP_FIELD) {
            return {originalMin, originalMax};
        }
        
        // Calculate center of popup (grid center)
        const ImVec2 popupCenter = {
            popupMin.x + (GRID_COL_COUNT * gridCellSize) * 0.5F,
            popupMin.y + (GRID_ROW_COUNT * gridCellSize) * 0.5F
        };
        
        // Original cell center
        const ImVec2 originalCenter = {
            (originalMin.x + originalMax.x) * 0.5F, 
            (originalMin.y + originalMax.y) * 0.5F
        };
        
        // Calculate angle from popup center to satellite
        const float dx = originalCenter.x - popupCenter.x;
        const float dy = originalCenter.y - popupCenter.y;
        const float angle = std::atan2(dy, dx);
        
        // Reduce size
        const float originalSize = originalMax.x - originalMin.x;
        const float newSize = originalSize * REDUCE_SATELLITES;
        
        // Calculate radius for circular arrangement
        // Use the grid cell size as the base radius (distance from center to satellite)
        const float baseRadius = gridCellSize * (GRID_COL_COUNT * 0.5F);
        // Add half the enlarged center size and half the satellite size
        const float enlargedCenterRadius = (gridCellSize * CENTER_EXTENT * ENLARGE_CENTER) * 0.5F;
        const float satelliteRadius = newSize * 0.5F;
        const float circularRadius = enlargedCenterRadius + satelliteRadius;
        
        // New center position on circle
        const ImVec2 newCenter = {
            popupCenter.x + std::cos(angle) * circularRadius,
            popupCenter.y + std::sin(angle) * circularRadius
        };
        
        return {
            {newCenter.x - newSize * 0.5F, newCenter.y - newSize * 0.5F},
            {newCenter.x + newSize * 0.5F, newCenter.y + newSize * 0.5F}
        };
    }

    void ImGuiBoard::drawPopupPiece(const SatelliteDrawParams& params, Piece piece)
    {
        const auto [originalMin, originalMax] = getPopupCellBounds(
            params.popupMin, params.gridCellSize, params.col, params.row);
        
        const auto [min, max] = adjustSatellitePosition(
            originalMin, originalMax, params.popupMin, params.gridCellSize);
        
        const float size = max.x - min.x;
        
        drawPopupRect(params.drawList, min, max, POPUP_PIECE_BACKGROUND);
        font::drawPiece(params.drawList, piece, min, size, params.font);
    }

    static void drawClearIcon(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float paddingRatio)
    {
        const float padding = (max.x - min.x) * paddingRatio;
        drawList->AddLine({min.x + padding, min.y + padding}, 
                       {max.x - padding, max.y - padding}, 
                       IM_COL32(255, 0, 0, 255), 2.0F);
        drawList->AddLine({min.x + padding, max.y - padding}, 
                       {max.x - padding, min.y + padding}, 
                       IM_COL32(255, 0, 0, 255), 2.0F);
    }

    void ImGuiBoard::drawSwitchColorIcon(const SatelliteDrawParams& params) const
    {
        const auto [originalMin, originalMax] = getPopupCellBounds(
            params.popupMin, params.gridCellSize, params.col, params.row);
        
        const auto [min, max] = adjustSatellitePosition(
            originalMin, originalMax, params.popupMin, params.gridCellSize);
        
        const float cellSize = max.x - min.x;
        
        drawPopupRect(params.drawList, min, max, POPUP_SWITCH_BACKGROUND);
        const ImVec2 center = {(min.x + max.x) * 0.5F, (min.y + max.y) * 0.5F};
        const float radius = cellSize * 0.3F;
        ImU32 circleColor = (pieceColor_ == Piece::WHITE) ? BLACK_COLOR : WHITE_COLOR;
        params.drawList->AddCircleFilled(center, radius, circleColor);
    }

    void ImGuiBoard::drawClearField(const SatelliteDrawParams& params)
    {
        const auto [originalMin, originalMax] = getPopupCellBounds(
            params.popupMin, params.gridCellSize, params.col, params.row);
        
        const auto [min, max] = adjustSatellitePosition(
            originalMin, originalMax, params.popupMin, params.gridCellSize);
        
        drawPopupRect(params.drawList, min, max, POPUP_PIECE_BACKGROUND);
        drawClearIcon(params.drawList, min, max, 0.2F);
    }

    void ImGuiBoard::drawPopupCenter(ImDrawList* drawList, ImFont* font, 
        const ImVec2& popupMin, float gridCellSize, Piece currentPieceOnSquare) const
    {
        ImVec2 min = {popupMin.x + gridCellSize, popupMin.y + gridCellSize};
        ImVec2 max = {min.x + CENTER_EXTENT * gridCellSize, min.y + CENTER_EXTENT * gridCellSize};
        float size = max.x - min.x;
        
        if (ROUND_POPUP_FIELD) {
            // Enlarge center
            const ImVec2 originalCenter = {(min.x + max.x) * 0.5F, (min.y + max.y) * 0.5F};
            size *= ENLARGE_CENTER;
            min = {originalCenter.x - size * 0.5F, originalCenter.y - size * 0.5F};
            max = {originalCenter.x + size * 0.5F, originalCenter.y + size * 0.5F};
        }
        
        // Check if the piece on the square is already the selected piece
        const bool isAlreadyThere = (currentPieceOnSquare == lastSelectedPiece_);
        const ImU32 centerColor = isAlreadyThere ? IM_COL32(150, 150, 150, 255) : POPUP_CENTER_COLOR;
        
        if (ROUND_POPUP_FIELD) {
            // Draw as circle
            const ImVec2 center = {(min.x + max.x) * 0.5F, (min.y + max.y) * 0.5F};
            const float radius = size * 0.5F;
            drawList->AddCircleFilled(center, radius, centerColor);
            drawList->AddCircle(center, radius, BLACK_COLOR, 0, 2.0F);
        } else {
            drawList->AddRectFilled(min, max, centerColor);
            drawList->AddRect(min, max, BLACK_COLOR, 0.0F, 0, 2.0F);
        }
        
        if (lastSelectedPiece_ == Piece::NO_PIECE) {
            drawClearIcon(drawList, min, max, 0.3F);
        } else {
            font::drawPiece(drawList, lastSelectedPiece_, min, size, font);
        }
    }

    std::optional<Piece> ImGuiBoard::drawPieceSelectionPopup(
        const ImVec2& cellMin, const ImVec2& cellMax, float cellSize, Piece currentPieceOnSquare)
    {
        const float gridCellSize = cellSize * GRID_CELL_SIZE_RATIO;
        const float popupWidth = gridCellSize * GRID_COL_COUNT;
        const float popupHeight = gridCellSize * GRID_ROW_COUNT;
        
        const ImVec2 popupMin = {
            cellMin.x + (cellSize - popupWidth) * 0.5F,
            cellMin.y + (cellSize - popupHeight) * 0.5F
        };
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImFont* font = ImGui::GetFont();
        ImGui::PushFont(font::chessFont);
        
        // Background
        const ImVec2 popupMax = {popupMin.x + popupWidth, popupMin.y + popupHeight};
        //drawList->AddRectFilled(popupMin, popupMax, IM_COL32(50, 50, 50, 50));
        //drawList->AddRect(popupMin, popupMax, IM_COL32(255, 255, 255, 255), 0.0F, 0, 2.0F);
        
        drawPopupCenter(drawList, font, popupMin, gridCellSize, currentPieceOnSquare);
        
        // Draw all configured cells
        for (const auto& cell : cells) {
            if (cell.type == PopupCellType::EMPTY || cell.type == PopupCellType::CENTER) {
                continue; // Center is drawn separately
            }
            
            const SatelliteDrawParams params = {
                .drawList = drawList,
                .font = font,
                .popupMin = popupMin,
                .gridCellSize = gridCellSize,
                .col = cell.col,
                .row = cell.row
            };
            
            if (cell.type == PopupCellType::PIECE) {
                const Piece coloredPiece = cell.basePiece + pieceColor_;
                drawPopupPiece(params, coloredPiece);
            } else if (cell.type == PopupCellType::COLOR_SWITCH) {
                drawSwitchColorIcon(params);
            } else if (cell.type == PopupCellType::CLEAR) {
                drawClearField(params);
            }
        }
        
        ImGui::PopFont();
        
        return handlePieceSelectionClick(popupMin, gridCellSize);
    }

    std::optional<Piece> ImGuiBoard::handlePieceSelectionClick(
        const ImVec2& popupMin, float gridCellSize)
    {
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
                    pieceColor_ = switchColor(pieceColor_);
                    lastSelectedPiece_ = getPieceType(lastSelectedPiece_);
                    if (lastSelectedPiece_ != Piece::NO_PIECE) {
                        lastSelectedPiece_ += pieceColor_;
                    }
                } else if (cell.type == PopupCellType::CLEAR) {
                    lastSelectedPiece_ = Piece::NO_PIECE;
                }
                return std::nullopt; 
            }
        }
        
        return std::nullopt;
    }

}
