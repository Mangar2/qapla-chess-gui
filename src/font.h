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
#include <string>
#include <imgui.h>

namespace font {

	inline ImFont* chessFont = nullptr;
	inline ImFont* interVariable = nullptr;

	/**
	 * @brief Loads a TrueType chess font into the ImGui font system.
	 * @param path Path to the .ttf font file.
	 * @param size Font size in pixels.
	 */
	void loadFonts();

	/**
	 * Gets the piece symbol for the given piece type.
	 */
	const char8_t* pieceSymbol(QaplaBasics::Piece p);

	/**
	 * Gets the piece background symbol for the given piece type.
	 */
	const char8_t* pieceBackground(QaplaBasics::Piece p);

	/**
	 * Draws a chess piece at the specified cell position.
	 * @param drawList ImGui draw list to use for rendering.
	 * @param piece The piece type to draw.
	 * @param cellMin Minimum coordinates of the cell.
	 * @param cellSize Size of the cell in pixels.
	 * @param font Optional font to use for drawing the piece symbol.
	 */
	void drawPiece(ImDrawList* drawList, QaplaBasics::Piece piece,
		const ImVec2& cellMin, float cellSize, ImFont* font = chessFont);
	void drawPiece(ImDrawList* drawList, QaplaBasics::Piece piece, ImFont* font = chessFont);

}
