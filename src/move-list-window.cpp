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

#include "move-list-window.h"
#include "qapla-tester/game-record.h"
#include "imgui.h"

#include <sstream>

using namespace QaplaWindows;

MoveListWindow::MoveListWindow(std::shared_ptr<const GameRecord> record) {
    gameRecord_ = std::move(record);
}

static void alignRight(const std::string& content) {
    float region = ImGui::GetContentRegionAvail().x;
    float textWidth = ImGui::CalcTextSize(content.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + region - textWidth);
}

void MoveListWindow::draw() {
    if (!gameRecord_) {
        ImGui::TextUnformatted("Internal Error");
        return;
    }
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
    constexpr ImGuiTableFlags flags =
        ImGuiTableFlags_RowBg
        | ImGuiTableFlags_SizingFixedFit
        | ImGuiTableFlags_ScrollX
        | ImGuiTableFlags_ScrollY;

    ImVec2 avail = ImGui::GetContentRegionAvail();

    if (ImGui::BeginTable("MoveListTable", 5, flags, ImVec2(avail.x, avail.y))) {
        ImGui::TableSetupColumn("Move", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Depth", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Eval", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("PV", ImGuiTableColumnFlags_WidthFixed);
        
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Move");

        ImGui::TableSetColumnIndex(1);
        alignRight("Depth");
        ImGui::TextUnformatted("Depth");

        ImGui::TableSetColumnIndex(2);
        alignRight("Time");
        ImGui::TextUnformatted("Time");

        ImGui::TableSetColumnIndex(3);
        alignRight("Eval");
        ImGui::TextUnformatted("Eval");

        ImGui::TableSetColumnIndex(4);
        ImGui::TextUnformatted("PV");

        int moveNumber = 1;
        const auto& moves = gameRecord_->history();

        for (size_t i = 0; i < moves.size(); i += 2) {
            renderMoveLine(std::to_string(moveNumber) + ".", moves[i]);
            if (i + 1 < moves.size()) {
                renderMoveLine("...", moves[i + 1]);
            }
            moveNumber++;
        }

        ImGui::EndTable();
    }

}

void MoveListWindow::renderMoveLine(const std::string& label, const MoveRecord& move) {
    ImGui::TableNextRow();

    // Move + SAN
    ImGui::TableSetColumnIndex(0);
    std::string moveLabel = label + move.san;
    if (!label.empty() && label[0] == '.') {
        alignRight(moveLabel);
    }
    ImGui::TextUnformatted(moveLabel.c_str());

    // Depth
    ImGui::TableSetColumnIndex(1);
    std::string depthLabel = move.depth == 0 ? "-" : std::to_string(move.depth);
    alignRight(depthLabel);
    ImGui::TextUnformatted(depthLabel.c_str());

    // Time
    ImGui::TableSetColumnIndex(2);
    std::ostringstream timeStream;
    timeStream << std::fixed << std::setprecision(2) << (move.timeMs / 1000.0) << "s";
    std::string timeLabel = timeStream.str();
    alignRight(timeLabel);
    ImGui::TextUnformatted(timeLabel.c_str());

    // Eval
    ImGui::TableSetColumnIndex(3);
    std::string scoreLabel = move.scoreCp ? std::to_string(*move.scoreCp) : "-";
    alignRight(scoreLabel);
    ImGui::TextUnformatted(scoreLabel.c_str());

    // PV
    ImGui::TableSetColumnIndex(4);
    ImGui::TextUnformatted("Hello World");//move.pv.c_str());
}
