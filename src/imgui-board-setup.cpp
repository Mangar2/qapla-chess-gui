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

#include "imgui-board-setup.h"
#include "imgui-controls.h"

#include <imgui.h>
#include <algorithm>
#include <cctype>

namespace QaplaWindows
{
    namespace
    {
        /**
         * Validates if a string represents a valid en passant square.
         * @param square The square string to validate (e.g., "e3", "-").
         * @return True if valid, false otherwise.
         */
        bool isValidEnPassantSquare(const std::string& square)
        {
            if (square == "-") {
                return true;
            }
            if (square.length() != 2) {
                return false;
            }
            const char file = square[0];
            const char rank = square[1];
            return (file >= 'a' && file <= 'h') && (rank == '3' || rank == '6');
        }

        /**
         * Normalizes en passant square input.
         * @param square The square string to normalize.
         * @return Normalized square string.
         */
        std::string normalizeEnPassantSquare(std::string square)
        {
            if (square.empty()) {
                return "-";
            }
            
            // Convert to lowercase
            for (char& c : square) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            
            if (!isValidEnPassantSquare(square)) {
                return "-";
            }
            
            return square;
        }

        /**
         * Input callback for en passant square validation.
         */
        int enPassantInputCallback(ImGuiInputTextCallbackData* data)
        {
            if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
                const char c = static_cast<char>(data->EventChar);
                // Allow only a-h, A-H, 3, 6, and '-'
                if ((c >= 'a' && c <= 'h') || (c >= 'A' && c <= 'H') || 
                    c == '3' || c == '6' || c == '-') {
                    return 0; // Accept character
                }
                return 1; // Reject character
            }
            return 0;
        }

    } // anonymous namespace

    namespace
    {
        void drawSideToMove(BoardSetupData& data, bool& modified)
        {
            ImGui::Text("Side to Move");
            
            bool whiteToMove = data.whiteToMove;
            if (ImGui::RadioButton("White", whiteToMove)) {
                data.whiteToMove = true;
                modified = true;
            }
            
            if (ImGui::RadioButton("Black", !whiteToMove)) {
                data.whiteToMove = false;
                modified = true;
            }
        }

        void drawSeparator()
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }

        void drawCastlingRights(BoardSetupData& data, bool& modified)
        {
            ImGui::Text("Castling Rights");
            
            if (ImGui::Checkbox("White Kingside", &data.whiteKingsideCastle)) {
                modified = true;
            }
            
            if (ImGui::Checkbox("White Queenside", &data.whiteQueensideCastle)) {
                modified = true;
            }
            
            if (ImGui::Checkbox("Black Kingside", &data.blackKingsideCastle)) {
                modified = true;
            }
            
            if (ImGui::Checkbox("Black Queenside", &data.blackQueensideCastle)) {
                modified = true;
            }
        }

        void drawEnPassantSquare(BoardSetupData& data, bool& modified)
        {
            ImGui::Text("En Passant Square");
            
            std::string tempSquare = data.enPassantSquare;
            if (ImGuiControls::inputText(
                "##enpassant", 
                tempSquare, 
                ImGuiInputTextFlags_CallbackCharFilter,
                enPassantInputCallback
            )) {
                std::string normalized = normalizeEnPassantSquare(tempSquare);
                if (normalized != data.enPassantSquare) {
                    data.enPassantSquare = normalized;
                    modified = true;
                }
            }
            
            ImGui::SameLine();
            ImGui::TextDisabled("(e.g., e3, -)");
        }

        void drawFullmoveNumber(BoardSetupData& data, bool& modified)
        {
            ImGui::Text("Fullmove Number");
            
            if (ImGuiControls::inputInt("##fullmove", data.fullmoveNumber, 1, 9999)) {
                modified = true;
            }
        }

        void drawHalfmoveClock(BoardSetupData& data, bool& modified)
        {
            ImGui::Text("Halfmove Clock");
            
            if (ImGuiControls::inputInt("##halfmove", data.halfmoveClock, 0, 100)) {
                modified = true;
            }
            
            ImGui::SameLine();
            ImGui::TextDisabled("(50-move rule)");
        }

    } // anonymous namespace

    bool ImGuiBoardSetup::draw(BoardSetupData& data)
    {
        bool modified = false;

        drawSideToMove(data, modified);
        drawSeparator();
        
        drawCastlingRights(data, modified);
        drawSeparator();
        
        drawEnPassantSquare(data, modified);
        drawSeparator();
        
        drawFullmoveNumber(data, modified);
        drawSeparator();
        
        drawHalfmoveClock(data, modified);

        return modified;
    }

} // namespace QaplaWindows
