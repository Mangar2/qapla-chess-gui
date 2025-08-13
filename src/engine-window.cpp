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
#include "imgui-button.h"
#include "imgui-popup.h"
#include "engine-setup-window.h"
#include "qapla-tester/move-record.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/engine-event.h"

#include "imgui.h"

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;

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

static void drawEngineInfo(const EngineRecord& record) {
    ImGui::PushID("EngineInfo");
    std::string name = record.config.getName();
	std::string status = EngineRecord::to_string(record.status);
	std::string memory = record.memoryUsageB ? 
        ", " + std::to_string(*record.memoryUsageB / (1024 * 1024)) + " MB" : "";
	renderReadonlyTextBoxes({ name, status + memory });
    ImGui::PopID();
}


EngineWindow::EngineWindow(std::shared_ptr<BoardData> boardData)
    : setupWindow_(std::make_unique<ImGuiPopup<EngineSetupWindow>>(
        ImGuiPopup<EngineSetupWindow>::Config{ .title = "Select Engines" })),
    boardData_(std::move(boardData))
{
}

EngineWindow::~EngineWindow() = default;

void EngineWindow::addTables(size_t size) {
    for (size_t i = tables_.size(); i < size; ++i) {
		displayedMoveNo_.push_back(0);
        tables_.emplace_back(std::make_unique<ImGuiTable>(std::format("EngineTable{}", i),
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                {"Depth", ImGuiTableColumnFlags_WidthFixed, 50.0f, true},
                { "Time", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Nodes", ImGuiTableColumnFlags_WidthFixed, 80.0f, true },
                { "NPS", ImGuiTableColumnFlags_WidthFixed, 60.0f, true },
                { "Tb hits", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Value", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Primary variant", ImGuiTableColumnFlags_WidthFixed }
        }));
	}
}

void EngineWindow::setTable(size_t index, const MoveRecord& moveRecord) {
    
    if (index >= tables_.size()) {
        return; 
    }
    auto& table = tables_[index];
	auto& searchInfos = moveRecord.info;
	auto& moveNo = moveRecord.halfmoveNo_;
    if (searchInfos.size() == table->size() && moveNo == displayedMoveNo_[index]) {
        return; 
    }
	displayedMoveNo_[index] = moveNo;
    table->clear();
    bool last = true;
    for (size_t i = searchInfos.size(); i > 0; --i) {
        auto& info = searchInfos[i - 1];
        if (info.pv.empty() && !last) continue;
        last = false;
        std::string nps = "-";
        std::string score = "-";
        std::string pv = "";
        if (info.nps) {
            nps = std::format("{:L}", *info.nps);
        }
        else if (info.timeMs && info.nodes && *info.timeMs != 0) {
            nps = std::format("{:L}", static_cast<int64_t>(
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
            info.timeMs ? formatMs(*info.timeMs, *info.timeMs < 60000 ? 1 : 0) : "-",
            info.nodes ? std::format("{:L}", *info.nodes) : "-",
            nps,
            info.tbhits ? std::format("{:L}", *info.tbhits) : "-",
            score,
            pv
        };
        table->push(row);
    }
}

void EngineWindow::setTable(size_t index) {
	addTables(boardData_->engineRecords().size());

    if (index >= tables_.size() || index >= boardData_->engineRecords().size()) {
        return;
    }

	auto& engineRecord = boardData_->engineRecords()[index];

    std::vector<SearchInfo> searchInfos;
    auto& history = boardData_->gameRecord().history();

	// Only display search info with matching engine ID in range of nextMoveIndex
    auto nextMoveIndex = boardData_->gameRecord().nextMoveIndex();
    auto& curMoveRecord = boardData_->engineRecords()[index].curMoveRecord;
    for (int i = 0; i < 2; i++) {
        int curIndex = static_cast<int>(nextMoveIndex) - i - 1;
        if (curMoveRecord->halfmoveNo_ == nextMoveIndex + 1) {
			setTable(index, *curMoveRecord);
            continue;
        }
        if (curIndex < 0 || curIndex >= history.size()) {
            // No matching move record found
			tables_[index]->clear();
            return;
        }
        auto& moveRecord = history[static_cast<size_t>(curIndex)];
        if (moveRecord.engineId_ == engineRecord.identifier) {
            setTable(index, moveRecord);
            return;
        }
    }
}

void EngineWindow::drawButtons(size_t index) {
    constexpr float space = 3.0f;
    constexpr float topOffset = 5.0f;
    constexpr float bottomOffset = 8.0f;
    constexpr float leftOffset = 20.0f;
    std::vector<std::string> buttons{ "Restart", "Stop", "Config" };
    ImVec2 boardPos = ImGui::GetCursorScreenPos();
	boardPos.x = std::round(boardPos.x);
	boardPos.y = std::round(boardPos.y);

    constexpr ImVec2 buttonSize = { 25.0f, 25.0f };
    auto totalSize = buttonSize; 
    for (const auto& button : buttons) {
        auto total = QaplaButton::calcIconButtonTotalSize(buttonSize, button.c_str());
		totalSize.x = std::max(totalSize.x, total.x);
		totalSize.y = std::max(totalSize.y, total.y);
    }
	totalSize.x = std::round(totalSize.x);
	totalSize.y = std::round(totalSize.y);
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    for (const auto& button : buttons) {
        ImGui::SetCursorScreenPos(pos);
        if (QaplaButton::drawIconButton(button, button, buttonSize, false,
            [&button](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
                if (button == "Restart") {
                    QaplaButton::drawRestart(drawList, topLeft, size);
                }
                if (button == "Stop") {
                    QaplaButton::drawStop(drawList, topLeft, size);
                }
                if (button == "Config") {
                    QaplaButton::drawConfig(drawList, topLeft, size);
                }
            }))
        {
            try {
                if (button == "Restart") {
					boardData_->restartEngine(index);
                }
                else if (button == "Stop") {
                    boardData_->stopEngine(index);
                }
                else if (button == "Config") {
                    setupWindow_->open();
                }
            }
            catch (...) {

            }
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}

void EngineWindow::drawEngineSelectionPopup() {
	setupWindow_->draw();

   if (auto confirmed = setupWindow_->confirmed()) {
        if (*confirmed) {
			boardData_->setEngines(setupWindow_->content().getActiveEngines());
        }
        setupWindow_->resetConfirmation();
    }
}

void EngineWindow::draw() {
    constexpr float cMinRowHeight = 80.0f;
    constexpr float cEngineInfoWidth = 160.0f;
    constexpr float cMinTableWidth = 200.0f;
    constexpr float cSectionSpacing = 4.0f;
    constexpr float cCornerRounding = 0.0f;
    const ImU32 cEvenBg = ImGui::GetColorU32(ImGuiCol_TableRowBg);
    const ImU32 cOddBg = ImGui::GetColorU32(ImGuiCol_TableRowBg);
    const ImU32 cBorder = ImGui::GetColorU32(ImGuiCol_TableBorderStrong);

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
        drawButtons(i);
        drawEngineSelectionPopup();
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

        setTable(i);
        tables_[i]->draw(ImVec2(max.x - tableMin.x, rowHeight));
        ImGui::Dummy(ImVec2(tableMinWidth, rowHeight));
        ImGui::PopID();
    }
}

