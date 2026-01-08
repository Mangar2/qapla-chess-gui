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

#include <qapla-engine/types.h>

#include <base-elements/change-tracker.h>
#include "imgui-board-setup.h"

#include <imgui.h>

#include <optional>
#include <memory>
#include <utility>
#include <vector>

namespace QaplaMoveGenerator
{
    class MoveGenerator;
}

namespace QaplaBasics
{
    class Move;
}

namespace QaplaTester
{
    class GameRecord;
    struct MoveRecord;
    class GameState;
}

namespace QaplaWindows
{

    struct MoveInput
    {
        std::optional<QaplaBasics::Square> from;
        std::optional<QaplaBasics::Square> to;
        std::optional<QaplaBasics::Piece> promotion;
    };

    class ImGuiBoard
    {
    public:
        ImGuiBoard();

        ImGuiBoard(ImGuiBoard&&);
        ImGuiBoard& operator=(ImGuiBoard&&);

        virtual ~ImGuiBoard();

        /**
         * Draw the chessboard and pieces.
         * @return The move record if a valid move was entered including LAN, SAN and 
         * internal Move, otherwise an empty optional.
         */
        std::optional<QaplaTester::MoveRecord> draw();

        /**
         * Set the board orientation.
         * @param inverted Whether the board is inverted.
         */
        void setInverted(bool inverted) { boardInverted_ = inverted; }

        /**
         * Get whether the board is inverted.
         * @return True if the board is inverted, false otherwise.
         */
        bool isInverted() const { return boardInverted_; }

        /**
         * Set whether to allow move input.
         * @param moveInput Whether to allow move input.
         */
        void setAllowMoveInput(bool moveInput) { allowMoveInput_ = moveInput; }

        /**
         * Set whether to allow piece input (for setup mode).
         * @param setupMode Whether to show the setup mode and allow piece input.
         */
        void setSetupMode(bool setupMode) { setupMode_ = setupMode; }

        /**
         * Sets the board position from the game record.
         * 
         * @param gameRecord Board to get the position from.
         * @param suppressChangeTracking If true, the change tracker is not updated from the game record
         * and the board is set no matter if the record has changed.
         */
        void setFromGameRecord(const QaplaTester::GameRecord &gameRecord, bool suppressChangeTracking = false);

        /**
         * Does a move.
         * @param move The move to apply.
         */
        void doMove(QaplaBasics::Move move);

        /**
         * @brief Set the From Fen object
         * 
         * @param startPos Whether it's the standard starting position.
         * @param fen The FEN string representing the position.
         */
        void setFromFen(bool startPos, const std::string &fen);

        /**
         * @brief Get the Fen object
         * 
         * @return The FEN string representing the current position.
         */
        std::string getFen() const;

        /**
         * @brief Check if the current position is valid.
         * 
         */
        bool isValidPosition() const;

    protected:
        QaplaTester::ChangeTracker gameRecordTracker_;
        std::unique_ptr<QaplaTester::GameState> gameState_;

    private:
        void drawPromotionPopup(float cellSize);
        bool promotionPending_ = false;

        /**
         * @brief Check if the current move input is valid and return the corresponding move.
         *
         * Checks if the current move input is valid. Resets the move input if it is invalid
         * or if the move is already complete. Sets promotion pending, if a promotion is detected.
         *
         * @return The corresponding move if valid, with LAN, SAN and internal Move.
         */
        std::optional<QaplaTester::MoveRecord> checkMove();

        std::pair<ImVec2, ImVec2> computeCellCoordinates(const ImVec2 &boardPos, float cellSize,
            QaplaBasics::File file, QaplaBasics::Rank rank) const;

        void drawBoardSquare(ImDrawList *drawList, const ImVec2 &boardPos, float cellSize,
                             QaplaBasics::File file, QaplaBasics::Rank rank, bool isWhite, bool popupIsHovered);
        void drawBoardSquares(ImDrawList *drawList, const ImVec2 &boardPos, float cellSize, bool popupIsHovered);

        void drawBoardPieces(ImDrawList *drawList, const ImVec2 &boardPos, float cellSize, ImFont *font);

        void drawBoardCoordinates(ImDrawList *drawList, const ImVec2 &boardPos, 
            float cellSize, float boardSize, ImFont *font, float maxSize) const;

        static void drawIndicatorIcon(ImDrawList *drawList, const ImVec2 &cellMin, float cellSize, bool left,
            QaplaBasics::Piece piece, ImU32 bgColor, ImFont *font);
        void drawEPIndicator(ImDrawList *drawList, const ImVec2 &boardPos, float cellSize, ImFont *font);
        void drawWhiteKingCastlingIndicators(ImDrawList *drawList, const ImVec2 &cellMin, float cellSize, 
            bool isWhite, ImFont *font);
        void drawBlackKingCastlingIndicators(ImDrawList *drawList, const ImVec2 &cellMin, float cellSize, 
            bool isWhite, ImFont *font);

        // Popup configuration and helpers
        enum class PopupCellType { EMPTY, PIECE, COLOR_SWITCH, CLEAR, CENTER };
        
        struct PopupCell {
            int col;
            int row;
            PopupCellType type;
            QaplaBasics::Piece basePiece = QaplaBasics::Piece::NO_PIECE; // Base piece type (without color)
        };

        struct SatelliteDrawParams {
            ImDrawList* drawList;
            ImFont* font;
            ImVec2 popupMin;
            float gridCellSize;
            int col;
            int row;
        };

        static std::pair<ImVec2, ImVec2> getPopupCellBounds(const ImVec2& popupMin, float cellSize, 
            int col, int row);
        static void drawPopupRect(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 bgColor);
        static void drawPopupPiece(const SatelliteDrawParams& params, QaplaBasics::Piece piece);
        void drawSwitchColorIcon(const SatelliteDrawParams& params) const;
        static void drawClearField(const SatelliteDrawParams& params);
        void drawPopupCenter(ImDrawList* drawList, ImFont* font, 
            const ImVec2& popupMin, float gridCellSize, QaplaBasics::Piece currentPieceOnSquare) const;
        static bool isRectClicked(const ImVec2& min, const ImVec2& max);
        
        // Returns selected piece when center is clicked
        // std::nullopt: no action (just cursor change)
        // NO_PIECE: remove piece from square
        // other piece: place that piece on square
        std::optional<QaplaBasics::Piece> drawPieceSelectionPopup(const ImVec2& cellMin, float cellSize, QaplaBasics::Piece currentPieceOnSquare);
        std::optional<QaplaBasics::Piece> handlePieceSelectionClick(const ImVec2& popupMin, float gridCellSize);
        
        // Handles piece selection popup logic - returns true if mouse is over popup
        bool handlePieceSelectionPopup();

        // Draws setup controls like side to move, castling rights, en passant, ...
        void drawSetupControls(const ImVec2 &screenPos, const float boardSize);

        bool boardInverted_ = false;
        bool allowMoveInput_ = false;
        bool setupMode_ = false;
        bool gameOver_ = false;

        QaplaBasics::Piece lastSelectedPiece_ = QaplaBasics::Piece::WHITE_PAWN;
        QaplaBasics::Piece pieceColor_ = QaplaBasics::Piece::WHITE;
        
        // For rendering piece selection popup after board (to avoid z-order issues)
        std::optional<QaplaBasics::Square> hoveredSquareForPopup_;
        ImVec2 hoveredSquareCellMin_;
        ImVec2 hoveredSquareCellMax_;
        float hoveredSquareCellSize_;

        MoveInput moveInput_;

        static const std::vector<PopupCell> cells;
    };

}
