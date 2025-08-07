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
#include "imgui-table.h"
#include "qapla-tester/move-record.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/string-helper.h"

#include "imgui.h"

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;

static void alignRight(const std::string& content) {
    float colWidth = ImGui::GetColumnWidth();
    float textWidth = ImGui::CalcTextSize(content.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + colWidth - textWidth - 10);
}

/**
 * Renders a vertical list of readonly text display fields styled like input boxes,
 * but without using actual ImGui::InputText widgets.
 *
 * @param lines Vector of text lines to display, each as its own styled box.
 */
static void renderReadonlyTextBoxes(const std::vector<std::string>& lines) {
    constexpr float boxPaddingX = 4.0f;
    constexpr float boxPaddingY = 2.0f;
    constexpr float boxRounding = 2.0f;
    constexpr ImU32 boxBgColor = IM_COL32(50, 52, 60, 255);
    constexpr ImU32 boxBorderColor = IM_COL32(90, 90, 100, 255);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float width = ImGui::CalcItemWidth();
    float boxHeight = ImGui::GetFrameHeight();
    float spacingY = ImGui::GetStyle().ItemSpacing.y;

    for (size_t i = 0; i < lines.size(); ++i) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImVec2(width, boxHeight);

        drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), boxBgColor, boxRounding);
        drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), boxBorderColor, boxRounding);

        ImGui::SetCursorScreenPos(ImVec2(pos.x + boxPaddingX, pos.y + boxPaddingY));
        ImGui::TextUnformatted(lines[i].c_str());

        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y + spacingY));
    }
}

static void textAlignRight(const std::string& content) {
    float region = ImGui::GetContentRegionAvail().x;
    float textWidth = ImGui::CalcTextSize(content.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + region - textWidth);
    ImGui::TextUnformatted(content.c_str());
}

static void drawEngineInfo(const EngineRecord& record) {
    ImGui::PushID("EngineInfo");
    std::string name = record.config.getName();
	std::string status = EngineRecord::to_string(record.status);
	std::string memory = record.memoryUsageB ? 
        ", " + std::to_string(*record.memoryUsageB / (1024 * 1024)) + " MB" : "";
	renderReadonlyTextBoxes({ name, status + memory });
    ImGui::PopID();
}

static void drawPVTableLine(const SearchInfo& searchInfo) {
    ImGui::TableNextRow();
    
    ImGui::TableSetColumnIndex(0);
    textAlignRight(searchInfo.depth ? std::to_string(*searchInfo.depth).c_str(): "-");

    ImGui::TableSetColumnIndex(1);
    textAlignRight(searchInfo.timeMs ? 
        formatMs(*searchInfo.timeMs, *searchInfo.timeMs < 60000 ? 1 : 0) : "-");

    ImGui::TableSetColumnIndex(2);
    textAlignRight(searchInfo.nodes ? std::format("{:L}", *searchInfo.nodes).c_str() : "-");

    ImGui::TableSetColumnIndex(3);
    std::string nps = "-";
    if (searchInfo.nps) {
        nps = std::format("{:L}", *searchInfo.nps);
    }
    else if (searchInfo.timeMs && searchInfo.nodes && *searchInfo.timeMs != 0) {
        nps = std::format("{:L}", static_cast<int64_t>(
            static_cast<double>(*searchInfo.nodes) * 1000.0 / static_cast<double>(*searchInfo.timeMs)));
    }
    textAlignRight(nps.c_str());

    ImGui::TableSetColumnIndex(4);
    textAlignRight(searchInfo.tbhits ? std::format("{:L}", *searchInfo.tbhits).c_str() : "-");

    ImGui::TableSetColumnIndex(5);
    if (searchInfo.scoreMate)  {
        textAlignRight(std::format("{}M{}", *searchInfo.scoreMate < 0 ? "-" : "", 
            std::abs(*searchInfo.scoreMate)));
    }
    else if (searchInfo.scoreCp) {
        textAlignRight(std::format("{:.2f}", *searchInfo.scoreCp / 100.0f));
    }
    else {
        textAlignRight("-");
    }

    if (searchInfo.pv.size() > 0) {
        std::string pv = "";
        for (auto& move : searchInfo.pv) {
            if (!pv.empty()) pv += ' ';
            pv += move;
		}
        ImGui::TableSetColumnIndex(6);
        ImGui::TextUnformatted(pv.c_str());
    }

}


EngineWindow::EngineWindow(std::shared_ptr<BoardData> boardData)
    : boardData_(std::move(boardData))
{
    table_ = std::make_unique<ImGuiTable>("EngineTable",
        ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
        std::vector<ImGuiTable::ColumnDef>{
            {"Depth", ImGuiTableColumnFlags_WidthFixed, 50.0f, true},
            { "Time", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
            { "Nodes", ImGuiTableColumnFlags_WidthFixed, 80.0f, true },
            { "NPS", ImGuiTableColumnFlags_WidthFixed, 60.0f, true },
            { "Tb hits", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
            { "Value", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
            { "Primary variant", ImGuiTableColumnFlags_WidthFixed }
    });
}

static void drawPVTable(const ImVec2& size, std::vector<SearchInfo> searchInfos) {
    constexpr ImGuiTableFlags flags =
        ImGuiTableFlags_RowBg
        | ImGuiTableFlags_SizingFixedFit
        | ImGuiTableFlags_ScrollX
        | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("EngineTable", 7, flags, size)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Depth", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Nodes", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("NPS", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Tb hits", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Primary variant", ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        uint32_t index = 0;
        for (std::string text : { "Depth", "Time", "Nodes", "NPS", "Tb hits", "Score" }) {
            ImGui::TableSetColumnIndex(index);
            textAlignRight(text);
            index++;
        }
        ImGui::TableSetColumnIndex(6);
        ImGui::TextUnformatted("Primary variant");
        for (size_t i = searchInfos.size(); i > 0; --i) {
			auto& info = searchInfos[i - 1];
            if (info.pv.size() > 0) {
                ImGui::PushID(("PVLine" + std::to_string(i)).c_str());
                drawPVTableLine(info);
                ImGui::PopID();
            }
		}
        ImGui::EndTable();
    }
}

void EngineWindow::draw() {
    constexpr float cMinRowHeight = 80.0f;
    constexpr float cEngineInfoWidth = 160.0f;
    constexpr float cMinTableWidth = 200.0f;
    constexpr float cSectionSpacing = 4.0f;
    constexpr float cCornerRounding = 6.0f;
    constexpr ImU32 cEvenBg = IM_COL32(40, 44, 52, 255);  
    constexpr ImU32 cOddBg = IM_COL32(30, 34, 40, 255);   
    constexpr ImU32 cBorder = IM_COL32(70, 80, 100, 255);

    const auto engineRecords = boardData_->engineRecords();
    if (engineRecords.empty()) {
        return;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();

    const float tableMinWidth = std::max(cMinTableWidth, avail.x - cEngineInfoWidth - cSectionSpacing);
    const float rowHeight = std::max(cMinRowHeight, avail.y / static_cast<float>(engineRecords.size()));

    ImVec2 start = ImGui::GetCursorScreenPos();
    for (size_t i = 0; i < engineRecords.size(); ++i) {
        ImGui::PushID(std::to_string(i).c_str());
        const ImU32 bg = (i % 2 == 0) ? cEvenBg : cOddBg;
        ImVec2 min = ImVec2(start.x, start.y + i * rowHeight);
        ImVec2 max = ImVec2(min.x + cEngineInfoWidth + tableMinWidth + cSectionSpacing, min.y + rowHeight);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(min, max, bg, cCornerRounding);
        drawList->AddRect(min, max, cBorder, cCornerRounding);

        ImGui::SetCursorScreenPos(ImVec2(min.x, min.y));
        ImGui::Dummy(ImVec2(cEngineInfoWidth, rowHeight));
        ImGui::SetCursorScreenPos(ImVec2(min.x, min.y + 5));
        ImGui::PushItemWidth(cEngineInfoWidth - 10.0f);
        ImGui::Indent(5.0f);
		drawEngineInfo(engineRecords[i]);
		ImGui::Unindent(5.0f);
		ImGui::PopItemWidth();
        
        ImGui::SetCursorScreenPos(ImVec2(min.x + cEngineInfoWidth, min.y));
        ImVec2 spacerMin = ImVec2(min.x + cEngineInfoWidth, min.y);
        ImVec2 spacerMax = ImVec2(min.x + cEngineInfoWidth + cSectionSpacing, min.y + rowHeight);
        drawList->AddRectFilled(spacerMin, spacerMax, IM_COL32(60, 60, 70, 120), 3.0f);

		ImVec2 tableMin = ImVec2(min.x + cEngineInfoWidth + cSectionSpacing, min.y);
        ImGui::SetCursorScreenPos(tableMin);

        int64_t indexModifier = 0;
        if (engineRecords.size() > 1) {
            indexModifier = !boardData_->gameRecord().isWhiteToMove() == (i == 0) ? 0 : 1;
        }

        std::vector<SearchInfo> searchInfos;
		auto& history = boardData_->gameRecord().history();
		int64_t nextMoveIndex = static_cast<int64_t>(boardData_->gameRecord().nextMoveIndex());
		int64_t index = nextMoveIndex - 1 - indexModifier;
        if (index >= 0 && static_cast<size_t>(index) < history.size()) {
			searchInfos = history[static_cast<size_t>(index)].info;
		}
        drawPVTable(ImVec2(max.x - tableMin.x, rowHeight), searchInfos);
        ImGui::Dummy(ImVec2(tableMinWidth, rowHeight));
        ImGui::PopID();
    }
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
