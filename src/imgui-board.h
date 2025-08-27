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
#pragma once

#include "qapla-engine/types.h"

#include <optional>
#include <memory>
#include <utility>
#include <imgui.h>

namespace QaplaMoveGenerator {
    class MoveGenerator;
}

namespace QaplaBasics {
    class Move;
}

namespace QaplaWindows {

    struct MoveInput {
        std::optional<QaplaBasics::Square> from;
        std::optional<QaplaBasics::Square> to;
        std::optional<QaplaBasics::Piece> promotion;
    };

    class ImguiBoard {
    public:
        explicit ImguiBoard();

        /**
         * Draw the chessboard and pieces.
         */
        MoveInput draw();

        /**
         * Set the board orientation.
         * @param inverted Whether the board is inverted.
         */
        void setInverted(bool inverted) { boardInverted_ = inverted; }

        /**
         * Set whether to allow move input.
         * @param moveInput Whether to allow move input.
         */
        void setAllowMoveInput(bool moveInput) { allowMoveInput_ = moveInput; }

        /**
         * Set whether a promotion is pending.
         * @param promotionPending Whether a promotion is pending.
         */
        void setPromotionPending(bool promotionPending) { promotionPending_ = promotionPending; }

        /**
         * Set the position.
         * @param position The position to set.
         */
        void setPosition(std::unique_ptr<QaplaMoveGenerator::MoveGenerator> position) { 
            position_ = std::move(position); 
        }

        /**
         * Does a move.
         * @param move The move to apply.
         */
        void doMove(QaplaBasics::Move move);

        /**
         * @brief Resets the current move input.
         */
        void resetMoveInput() {
            moveInput_ = {};
        }

    private:

        void drawPromotionPopup(float cellSize);
        bool promotionPending_ = false;
  
        std::pair<ImVec2, ImVec2> computeCellCoordinates(const ImVec2& boardPos, float cellSize, 
            QaplaBasics::File file, QaplaBasics::Rank rank);
        
        void drawBoardSquare(ImDrawList* drawList, const ImVec2& boardPos, float cellSize,
            QaplaBasics::File file, QaplaBasics::Rank rank, bool isWhite);
        void drawBoardSquares(ImDrawList* drawList, const ImVec2& boardPos, float cellSize);
        
        void drawBoardPieces(ImDrawList* drawList, const ImVec2& boardPos, float cellSize, ImFont* font);

        void drawBoardCoordinates(ImDrawList* drawList, const ImVec2& boardPos, float cellSize, float boardSize, ImFont* font, float maxSize);

        bool boardInverted_ = false;
        bool allowMoveInput_ = false;
            
        MoveInput moveInput_;
        std::unique_ptr<QaplaMoveGenerator::MoveGenerator> position_;

    };


}
