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

#include "epd-window.h"
#include "imgui-table.h"
#include "imgui-button.h"

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


EpdWindow::EpdWindow(std::shared_ptr<BoardData> boardData)
    : boardData_(std::move(boardData))
{
}

EpdWindow::~EpdWindow() = default;

void EpdWindow::initTable() {
    table_ = std::make_unique<ImGuiTable>("EpdTable",
        ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
        std::vector<ImGuiTable::ColumnDef>{
            { "Name", ImGuiTableColumnFlags_WidthFixed, 100.0f, true},
            { "Request", ImGuiTableColumnFlags_WidthFixed, 100.0f, true },
            { "Result", ImGuiTableColumnFlags_WidthFixed, 80.0f, true }
        }
    );
}

void EpdWindow::drawButtons() {
    constexpr float space = 3.0f;
    constexpr float topOffset = 5.0f;
    constexpr float bottomOffset = 8.0f;
    constexpr float leftOffset = 20.0f;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = { 25.0f, 25.0f };
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    for (const std::string button : { "Run" }) {
        ImGui::SetCursorScreenPos(pos);
        if (QaplaButton::drawIconButton(button, button, buttonSize,
            [&button](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size, bool hover) {
                if (button == "Run") {
                    QaplaButton::drawArrow(drawList, topLeft, size, hover);
                }
            }))
        {
            try {
                boardData_->epdData().analyse();
            } 
            catch (...) {

            }
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}

void EpdWindow::draw() {
    drawButtons();
    ImVec2 size = ImGui::GetContentRegionAvail();
    boardData_->epdData().drawTable(size);
}

