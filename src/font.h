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
#include <string>
#include "ui/qapla-ui.h"

namespace FontManager {

	inline constexpr QaplaUi::Font chessFont = QaplaUi::Font::Chess;
	inline constexpr QaplaUi::Font interVariable = QaplaUi::Font::Inter;
	inline constexpr QaplaUi::Font ibmPlexMono = QaplaUi::Font::Mono;

	static constexpr int interVariableIndex = 2;
	static constexpr int ibmPlexMonoIndex = 3;

	/**
	 * @brief List of supported fonts for general use (excluding chess font at index 1).
	 * Each entry contains the font index and a descriptive name.
	 */
	struct SupportedFont {
		int index;
		const char* name;
	};

	inline constexpr SupportedFont supportedFonts[] = {
		{0, "System Font"},
		{interVariableIndex, "Inter Variable"},
		{ibmPlexMonoIndex, "IBM Plex Mono"}
	};


	/**
	 * Gets the piece symbol for the given piece type.
	 */
	const char8_t* pieceSymbol(QaplaBasics::Piece piece);

	/**
	 * Gets the piece background symbol for the given piece type.
	 */
	const char8_t* pieceBackground(QaplaBasics::Piece piece);

	/**
	 * Draws a chess piece at the specified cell position.
	 * @param drawList ImGui draw list to use for rendering.
	 * @param piece The piece type to draw.
	 * @param cellMin Minimum coordinates of the cell.
	 * @param cellSize Size of the cell in pixels.
	 * @param font Optional font to use for drawing the piece symbol.
	 */
	void drawPiece(QaplaUi::DrawList* drawList, QaplaBasics::Piece piece,
		const QaplaUi::Vec2& cellMin, float cellSize, QaplaUi::Font font = chessFont);
	void drawPiece(QaplaUi::DrawList* drawList, QaplaBasics::Piece piece, QaplaUi::Font font = chessFont);

}
