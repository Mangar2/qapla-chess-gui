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

#include "engine-window.h"
#include "qapla-tester/move-record.h"
#include "qapla-tester/game-record.h"

#include "imgui.h"

#include <sstream>

using namespace QaplaWindows;

static void alignRight(const std::string& content) {
    float colWidth = ImGui::GetColumnWidth();
    float textWidth = ImGui::CalcTextSize(content.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + colWidth - textWidth - 10);
}

void EngineWindow::draw() {
    if (!BoardData_) {
        ImGui::TextUnformatted("Internal Error");
        return;
    }
	auto& gameRecord = BoardData_->gameRecord();
    ImGui::BeginChild("MoveListScroll", ImVec2(0, 0), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::Columns(5, nullptr, false);
    ImGui::SetColumnWidth(0, 80);   
    ImGui::SetColumnWidth(1, 50);   
    ImGui::SetColumnWidth(2, 50);   
    ImGui::SetColumnWidth(3, 50);   
    // Column 5 (PV) uses remaining width

    ImGui::TextUnformatted("Move"); 
    ImGui::NextColumn();
    alignRight("Depth");
    ImGui::TextUnformatted("Depth"); 
    ImGui::NextColumn();
    alignRight("Time");
    ImGui::TextUnformatted("Time"); 
    ImGui::NextColumn();
    alignRight("Eval");
    ImGui::TextUnformatted("Eval"); 
    ImGui::NextColumn();
    ImGui::TextUnformatted("PV"); 
    ImGui::NextColumn();
    ImGui::Separator();

    int moveNumber = 1;
    const auto& moves = gameRecord.history();

    for (size_t i = 0; i < moves.size(); i += 2) {
        renderMoveLine(std::to_string(moveNumber) + ".", moves[i]);
        if (i + 1 < moves.size()) {
            renderMoveLine("...", moves[i + 1]);
        }
        moveNumber++;
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}



void EngineWindow::renderMoveLine(const std::string& label, const MoveRecord& move) {
    std::ostringstream stream;
    std::string moveLabel = label + move.san;

    if (label.at(0) == '.') {
        alignRight(moveLabel);
    }
    ImGui::TextUnformatted(moveLabel.c_str()); 
    ImGui::NextColumn();
    
    std::string depthLabel = move.depth == 0 ? "-" : std::to_string(move.depth);
    alignRight(depthLabel);
    ImGui::TextUnformatted(depthLabel.c_str());
    
    ImGui::NextColumn();
    stream << std::fixed << std::setprecision(2) << (move.timeMs / 1000.0) << "s";
    std::string timeLabel = stream.str();
    alignRight(timeLabel);
    ImGui::TextUnformatted(timeLabel.c_str());

    ImGui::NextColumn();
    std::string scoreLabel = move.scoreCp ? std::to_string(*move.scoreCp).c_str() : "-";
    alignRight(scoreLabel);
    ImGui::TextUnformatted(scoreLabel.c_str());
    ImGui::NextColumn();
    ImGui::TextUnformatted(move.pv.c_str()); 
    ImGui::NextColumn();
}
