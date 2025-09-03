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

#include "imgui-engine-list.h"
#include "font.h"
#include "imgui-table.h"
#include "imgui-button.h"
#include "imgui-separator.h"
#include "qapla-tester/engine-record.h"
#include "qapla-tester/move-record.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/engine-event.h"
#include "qapla-engine/types.h"

#include <imgui.h>

#include <sstream>
#include <string>
#include <format>
#include <memory>
#include <algorithm>

using namespace QaplaWindows;

/**
 * Renders a vertical list of readonly text display fields styled like input boxes,
 * but without using actual ImGui::InputText widgets.
 *
 * @param lines Vector of text lines to display, each as its own styled box.
 */
static void renderReadonlyTextBoxes(const std::vector<std::string>& lines, size_t index) {
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
		drawList->PushClipRect(pos, ImVec2(pos.x + size.x - 4.0f, pos.y + size.y), true);
        auto textTopLeft = ImVec2(pos.x + boxPaddingX, pos.y + boxPaddingY);
        ImGui::SetCursorScreenPos(textTopLeft);
        if (i == 0 && index <= 1) {
            font::drawPiece(drawList, index == 0 ? QaplaBasics::WHITE_KING : QaplaBasics::BLACK_KING);
            ImGui::SetCursorScreenPos(ImVec2(textTopLeft.x + 20, textTopLeft.y));
        }
        ImGui::TextUnformatted(lines[i].c_str());
		drawList->PopClipRect();
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y + spacingY));
    }
}

static void drawEngineInfo(const EngineRecord& record, size_t index) {
    ImGui::Indent(5.0f);
    ImGui::PushID("EngineInfo");
    std::string name = record.config.getName();
	std::string status = EngineRecord::to_string(record.status);
	std::string memory = record.memoryUsageB ? 
        ", " + std::to_string(*record.memoryUsageB / (1024 * 1024)) + " MB" : "";
	renderReadonlyTextBoxes({ name, status + memory }, index);
    ImGui::PopID();
    ImGui::Unindent(5.0f);
}


ImGuiEngineList::ImGuiEngineList()
{
}

ImGuiEngineList::ImGuiEngineList(ImGuiEngineList&&) noexcept = default;
ImGuiEngineList& ImGuiEngineList::operator=(ImGuiEngineList&&) noexcept = default;

ImGuiEngineList::~ImGuiEngineList() = default;

void ImGuiEngineList::addTables(size_t size) {
    for (size_t i = tables_.size(); i < size; ++i) {
		displayedMoveNo_.push_back(0);
		infoCnt_.push_back(0);
        tables_.emplace_back(std::make_unique<ImGuiTable>(std::format("EngineTable{}", i),
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit,
            std::vector<ImGuiTable::ColumnDef>{
                {"Depth", ImGuiTableColumnFlags_WidthFixed, 50.0f, true},
                { "Time", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Nodes", ImGuiTableColumnFlags_WidthFixed, 80.0f, true },
                { "NPS", ImGuiTableColumnFlags_WidthFixed, 60.0f, true },
                { "Tb hits", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Value", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Primary variant", ImGuiTableColumnFlags_WidthFixed, 1660.0f }
        }));
	}
}

std::vector<std::string> ImGuiEngineList::mkTableLine(ImGuiTable* table, const SearchInfo& info) {
        std::string npsStr = "-";
        std::string score = "-";
        std::string pv = "";
        if (info.nps) {
            npsStr = std::format("{:L}", *info.nps);
        }
        else if (info.timeMs && info.nodes && *info.timeMs != 0) {
            npsStr = std::format("{:L}", static_cast<int64_t>(
                static_cast<double>(*info.nodes) * 1000.0 / static_cast<double>(*info.timeMs)));
        }
        if (info.scoreMate) {
            score = std::format("{}M{}", *info.scoreMate < 0 ? "-" : "", std::abs(*info.scoreMate));
        }
        else if (info.scoreCp) {
            score = std::format("{:.2f}", *info.scoreCp / 100.0f);
        }
        if (score != "-" && table->size() == 1 && table->getField(0, 5)  == "-") {
            table->setField(0, 5, score);
		}
        if (!info.pv.empty()) {
            for (auto& move : info.pv) {
                if (!pv.empty()) pv += ' ';
                pv += move;
            }
        } else if (info.currMove) {
            pv = *info.currMove;
        } 
        std::vector<std::string> row = {
            info.depth ? std::to_string(*info.depth) : "-",
            info.timeMs ? formatMs(*info.timeMs, 0) : "-",
            info.nodes ? std::format("{:L}", *info.nodes) : "-",
            npsStr,
            info.tbhits ? std::format("{:L}", *info.tbhits) : "-",
            score,
            pv
        };
        return row;
}

void ImGuiEngineList::setTable(size_t index, const MoveRecord& moveRecord) {
    
    if (index >= tables_.size()) {
        return; 
    }
    
    auto& table = tables_[index];
	auto& searchInfos = moveRecord.info;
	auto& moveNo = moveRecord.halfmoveNo_;
    if (moveRecord.infoUpdateCount == infoCnt_[index] && moveNo == displayedMoveNo_[index]) {
        return; 
    }

    table->clear();

	infoCnt_[index] = moveRecord.infoUpdateCount;
	displayedMoveNo_[index] = moveNo;

    bool last = true;
    for (size_t i = searchInfos.size(); i > 0; --i) {
        if (table->size() >= searchInfos.size()) break;
        auto& info = searchInfos[i - 1];
        if (info.pv.empty() && !last) continue;
        last = false;
        auto row = mkTableLine(table.get(), info);
        table->push(row);
    }
}

std::string ImGuiEngineList::drawButtons(size_t index) {
    constexpr float space = 3.0f;
    constexpr float topOffset = 5.0f;
    constexpr float bottomOffset = 8.0f;
    constexpr float leftOffset = 20.0f;
    std::vector<std::string> buttons{ "Restart", "Stop" };
    ImVec2 topLeft = ImGui::GetCursorScreenPos();
	topLeft.x = std::round(topLeft.x);
	topLeft.y = std::round(topLeft.y);
    auto curPos = ImVec2(topLeft.x + leftOffset, topLeft.y + topOffset);

    constexpr ImVec2 buttonSize = { 25.0f, 25.0f };

    auto totalSize = QaplaButton::calcIconButtonsTotalSize(buttonSize, buttons);

    std::string command;
    for (const auto& button : buttons) {
        ImGui::SetCursorScreenPos(curPos);
        if (QaplaButton::drawIconButton(button, button, buttonSize, false,
            [&button](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
                if (button == "Restart") {
                    QaplaButton::drawRestart(drawList, topLeft, size);
                }
                if (button == "Stop") {
                    QaplaButton::drawStop(drawList, topLeft, size);
                }
            }))
        {
            command = button;
        }
        curPos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x, topLeft.y + totalSize.y + topOffset + bottomOffset));
    return command;
}

std::string ImGuiEngineList::drawEngineSpace(size_t index, ImVec2 size) {

    std::string command;
    constexpr float cEngineInfoWidth = 160.0f;
    constexpr float cSectionSpacing = 4.0f;
    
    const bool isSmall = size.y < 100.0f;
    
    const ImU32 bgColor = ImGui::GetColorU32(ImGuiCol_TableRowBg);
    const ImU32 cBorder = ImGui::GetColorU32(ImGuiCol_TableBorderStrong);

    ImVec2 topLeft = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImGui::PushID(std::to_string(index).c_str());
    ImVec2 max = ImVec2(topLeft.x + cEngineInfoWidth + size.x + cSectionSpacing, topLeft.y + size.y);

    drawList->AddRectFilled(topLeft, max, bgColor);
    ImGuiSeparator::Horizontal();

    command = drawEngineArea(topLeft, drawList, max, cEngineInfoWidth, index, isSmall);

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x + cEngineInfoWidth, topLeft.y));
    ImGuiSeparator::Vertical();

    drawEngineTable(topLeft, cEngineInfoWidth, cSectionSpacing, index, max, size);
    
    ImGui::SetCursorScreenPos(topLeft);
    ImGui::Dummy(ImVec2(size.x, size.y - 3.0f));
    ImGui::PopID();
    return command;
}

std::string QaplaWindows::ImGuiEngineList::drawEngineArea(const ImVec2 &topLeft, ImDrawList *drawList, 
    const ImVec2 &max, float cEngineInfoWidth, size_t index, bool isSmall)
{
    std::string command;
    ImGui::SetCursorScreenPos(topLeft);
    drawList->PushClipRect(topLeft, max);
    ImGui::SetCursorScreenPos(ImVec2(topLeft.x, topLeft.y + 5.0f));
    ImGui::PushItemWidth(cEngineInfoWidth - 10.0f);
    bool hasEngine = (index < engineRecords_.size());
    if (allowInput_ && (!isSmall || !hasEngine)) command = drawButtons(index);
    if (hasEngine) drawEngineInfo(engineRecords_[index], index);
    ImGui::PopItemWidth();
    drawList->PopClipRect();
    return command;
}

void QaplaWindows::ImGuiEngineList::drawEngineTable(const ImVec2 &topLeft, float cEngineInfoWidth, float cSectionSpacing, 
    size_t index, const ImVec2 &max, const ImVec2 &size)
{
    ImVec2 tableMin = ImVec2(topLeft.x + cEngineInfoWidth + cSectionSpacing, topLeft.y);
    ImGui::SetCursorScreenPos(tableMin);
    if (index < tables_.size())
    {
        auto tableSize = ImVec2(max.x - tableMin.x, size.y);

        if (ImGui::BeginChild("TableScroll", tableSize, ImGuiChildFlags_AutoResizeX,
            ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            tables_[index]->draw(ImVec2(2000.0f, tableSize.y));
        }
        ImGui::EndChild();
    }
}

std::pair<uint32_t, std::string> ImGuiEngineList::draw() {
    const float cMinRowHeight = 50.0f;
    constexpr float cEngineInfoWidth = 160.0f;
    constexpr float cMinTableWidth = 200.0f;
    constexpr float cSectionSpacing = 4.0f;

	const auto records = engineRecords_.size();
    addTables(records);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float tableMinWidth = std::max(cMinTableWidth, avail.x - cEngineInfoWidth - cSectionSpacing);
    const uint32_t rowHeight = static_cast<uint32_t>(
        std::max(cMinRowHeight, avail.y / static_cast<float>(records)));
    uint32_t index = 0;
    std::string command;
    for (size_t i = 0; i < records; ++i) {
		auto c = drawEngineSpace(i, ImVec2(tableMinWidth, static_cast<float>(rowHeight)));
        if (!c.empty()) {
            index = i;
            command = std::move(c); 
        }
    }
    return { index, command };
}

