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

#include "qapla-engine/types.h"
#include "board-data.h"
#include "embedded-window.h"

#include <optional>
#include <unordered_set>
#include <utility>
#include <string>
#include <imgui.h>

namespace QaplaWindows {

    class ChessBoardWindow : public EmbeddedWindow {
    public:
        explicit ChessBoardWindow(std::shared_ptr<BoardData> BoardData) : BoardData_(std::move(BoardData)) {}

        void draw() override;

    private:
        void drawMaximized();

        void drawPromotionPopup(float cellSize);
        bool promotionPending_ = false;
        std::string id_ = "#0";

        std::pair<ImVec2, ImVec2> computeCellCoordinates(const ImVec2& boardPos, float cellSize, 
            QaplaBasics::File file, QaplaBasics::Rank rank);
        
        void drawBoardSquare(ImDrawList* drawList, const ImVec2& boardPos, float cellSize,
            QaplaBasics::File file, QaplaBasics::Rank rank, bool isWhite);
        void drawBoardSquares(ImDrawList* drawList, const ImVec2& boardPos, float cellSize);
        
        void drawPiece(ImDrawList* drawList, QaplaBasics::Piece piece,
            const ImVec2& cellMin, float cellSize, ImFont* font);
        void drawBoardPieces(ImDrawList* drawList, const ImVec2& boardPos, float cellSize, ImFont* font);

        void drawBoardCoordinates(ImDrawList* drawList, const ImVec2& boardPos, float cellSize, float boardSize, ImFont* font, float maxSize);

        bool boardInverted_ = false;
        bool maximized_ = false;
        std::shared_ptr<BoardData> BoardData_;

        std::optional<QaplaBasics::Square> selectedFrom_;
        std::optional<QaplaBasics::Square> selectedTo_;

    };


}
