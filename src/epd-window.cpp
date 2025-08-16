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
#include "snackbar.h"
#include "os-dialogs.h"
#include "imgui-controls.h"

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

void EpdWindow::drawButtons() {
    constexpr float space = 3.0f;
    constexpr float topOffset = 5.0f;
    constexpr float bottomOffset = 8.0f;
    constexpr float leftOffset = 20.0f;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = { 25.0f, 25.0f };
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    for (const std::string button : { "Run", "Stop", "Clear" }) {
        ImGui::SetCursorScreenPos(pos);
        if (QaplaButton::drawIconButton(button, button, buttonSize, false,
            [&button](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
                if (button == "Run") {
                    QaplaButton::drawPlay(drawList, topLeft, size);
                }
                if (button == "Stop") {
                    QaplaButton::drawStop(drawList, topLeft, size);
				}
                if (button == "Clear") {
                    QaplaButton::drawText("X", drawList, topLeft, size);
                }
            }))
        {
            try {
                if (button == "Run") {
                    boardData_->clearPool();
                    boardData_->epdData().analyse();
                } 
                else if (button == "Stop") {
                    boardData_->stopPool();
			    } 
                else if (button == "Clear") {
                    boardData_->clearPool();
                    boardData_->epdData().clear();
                }
            } 
            catch (const std::exception& e) {
                SnackbarManager::instance().showSnackbar(std::string("Fehler: ") + e.what(), 5.0f);
            }
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}

void EpdWindow::drawInput() {
    
    if (!ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_Selected)) return;
	ImGui::Indent(10.0f);
	constexpr float inputWidth = 200.0f;
    constexpr int maxConcurrency = 32;
	constexpr int maxSeenPlies = 32;
    int concurrency = static_cast<int>(boardData_->epdData().config().concurrency);
	int maxTimeInS = static_cast<int>(boardData_->epdData().config().maxTimeInS);
	int minTimeInS = static_cast<int>(boardData_->epdData().config().minTimeInS);
	int seenPlies = static_cast<int>(boardData_->epdData().config().seenPlies);
    std::string filePath = boardData_->epdData().config().filepath;
    //ImGui::DragInt("Concurrency", &concurrency, 1, 1, maxConcurrency);
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::SliderInt("Concurrency", &concurrency, 1, maxConcurrency);
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::SliderInt("Seen plies", &seenPlies, 1, maxSeenPlies);
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::InputInt("Max time (s)", &maxTimeInS);
    ImGui::SetNextItemWidth(inputWidth);
	ImGui::InputInt("Min time (s)", &minTimeInS);
    if (ImGui::Button("Select")) {
        try {
            auto selectedFiles = OsDialogs::openFileDialog();
            if (!selectedFiles.empty()) {
                boardData_->epdData().config().filepath = selectedFiles[0]; // Use the first selected file
            }
        }
        catch (const std::exception& e) {
            SnackbarManager::instance().showSnackbar(std::string("Error: ") + e.what(), 5.0f);
        }
    }
    ImGui::SetNextItemWidth(inputWidth * 2);
    ImGui::SameLine();
    if (auto path = ImGuiControls::inputText("Epd file", filePath)) {
        boardData_->epdData().config().filepath = *path;
    }


    if (concurrency != static_cast<int>(boardData_->epdData().config().concurrency)) {
        boardData_->setPoolConcurrency(concurrency, true, true);
    }
	boardData_->epdData().config().concurrency = static_cast<uint32_t>(concurrency);
    boardData_->epdData().config().maxTimeInS = static_cast<uint64_t>(maxTimeInS);
	boardData_->epdData().config().minTimeInS = static_cast<uint64_t>(minTimeInS);
	boardData_->epdData().config().seenPlies = static_cast<uint32_t>(seenPlies);
	
    ImGui::Unindent(10.0f);
}

void EpdWindow::draw() {
    drawButtons();
    drawInput();
    ImVec2 size = ImGui::GetContentRegionAvail();
    auto clickedRow = boardData_->epdData().drawTable(size);
    if (clickedRow) {
        auto fen = boardData_->epdData().getFen(*clickedRow);
        if (fen) {
            boardData_->setPosition(false, *fen);
        }
    }
}

