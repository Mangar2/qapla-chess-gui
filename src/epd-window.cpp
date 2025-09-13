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

#include <imgui.h>

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;

EpdWindow::EpdWindow()
{
}

EpdWindow::~EpdWindow() = default;

void EpdWindow::drawButtons()
{
    constexpr float space = 3.0f;
    constexpr float topOffset = 5.0f;
    constexpr float bottomOffset = 8.0f;
    constexpr float leftOffset = 20.0f;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = {25.0f, 25.0f};
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    for (const std::string button : {"Run", "Stop", "Clear"})
    {
        ImGui::SetCursorScreenPos(pos);
        auto state = QaplaButton::ButtonState::Normal;
        if (QaplaButton::drawIconButton(
                button, button, buttonSize, state, [&button, state](ImDrawList *drawList, ImVec2 topLeft, ImVec2 size)
                {
            if (button == "Run")
            {
                QaplaButton::drawPlay(drawList, topLeft, size, state);
            }
            if (button == "Stop")
            {
                QaplaButton::drawStop(drawList, topLeft, size, state);
            }
            if (button == "Clear")
            {
                QaplaButton::drawClear(drawList, topLeft, size, state);
            } }))
        {
            try
            {
                auto &boardData = InteractiveBoardWindow::instance();
                if (button == "Run")
                {
                    boardData.clearPool();
                    boardData.epdData().analyse();
                    SnackbarManager::instance().showSuccess("Epd analysis started");
                }
                else if (button == "Stop")
                {
                    boardData.stopPool();
                }
                else if (button == "Clear")
                {
                    boardData.clearPool();
                    boardData.epdData().clear();
                }
            }
            catch (const std::exception &e)
            {
                SnackbarManager::instance().showError(std::string("Fehler: ") + e.what());
            }
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}

void EpdWindow::drawInput()
{

    constexpr float inputWidth = 200.0f;
    constexpr int maxConcurrency = 32;
    constexpr int maxSeenPlies = 32;

    auto &boardData = InteractiveBoardWindow::instance();
    ImGui::Indent(10.0f);
    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::sliderInt<uint32_t>("Concurrency",
                                           boardData.epdData().config().concurrency, 1, maxConcurrency))
    {
        boardData.setPoolConcurrency(boardData.epdData().config().concurrency, true, true);
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_Selected))
    {
        ImGui::Indent(10.0f);

        std::string filePath = boardData.epdData().config().filepath;
        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::inputInt<uint32_t>("Seen plies",
                                        boardData.epdData().config().seenPlies, 1, maxSeenPlies);

        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::inputInt<uint64_t>("Max time (s)",
                                        boardData.epdData().config().maxTimeInS, 1, 3600 * 24 * 365, 1, 100);

        ImGui::SetNextItemWidth(inputWidth);
        ImGuiControls::inputInt<uint64_t>("Min time (s)",
                                        boardData.epdData().config().minTimeInS, 1, 3600 * 24 * 365, 1, 100);

        ImGui::Spacing();
        ImGuiControls::existingFileInput("Epd or RAW position file:",
                                boardData.epdData().config().filepath, inputWidth * 2.0f);
        ImGui::Spacing();
        ImGui::Unindent(10.0f);
    }
    ImGui::Unindent(10.0f);
}

void EpdWindow::draw()
{
    auto &boardData = InteractiveBoardWindow::instance();
    drawButtons();
    drawInput();
    ImVec2 size = ImGui::GetContentRegionAvail();
    ImGui::Indent(10.0f);
    auto clickedRow = boardData.epdData().drawTable(size);
    ImGui::Unindent(10.0f);
    if (clickedRow)
    {
        auto fen = boardData.epdData().getFen(*clickedRow);
        if (fen)
        {
            boardData.setPosition(false, *fen);
        }
    }
}
